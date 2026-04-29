// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_child_client.h"

#include "bot_core/bot_core.h"
#include "bot_core/msg_sender.h"
#include "match_process/ipc_frame.h"
#include "match_process/match_ipc.pb.h"
#include "bot_core/subprocess.h"
#include "utility/log.h"
#include "utility/utils.h"

namespace {

ErrCode StageToErr(const lgtbot::ipc::ResultResp::Stage s)
{
    using S = lgtbot::ipc::ResultResp;
    switch (s) {
    case S::STAGE_OK:        return EC_GAME_REQUEST_OK;
    case S::STAGE_CHECKOUT:  return EC_GAME_REQUEST_CHECKOUT;
    case S::STAGE_FAILED:    return EC_GAME_REQUEST_FAILED;
    case S::STAGE_CONTINUE:  return EC_GAME_REQUEST_CONTINUE;
    case S::STAGE_NOT_FOUND: return EC_GAME_REQUEST_NOT_FOUND;
    default:                 return EC_GAME_REQUEST_UNKNOWN;
    }
}

} // namespace

MatchChildClient::MatchChildClient(const std::filesystem::path runner_exe,
                                   const std::filesystem::path game_library)
{
    std::string spawn_err;
    std::vector<std::string> argv{runner_exe.string(), game_library.string()};
    proc_ = std::make_unique<Subprocess>(std::move(argv), spawn_err);
    if (!proc_->ok()) {
        ErrorLog() << "MatchChildClient spawn failed: " << spawn_err;
        proc_.reset();
        return;
    }
    child_in_  = proc_->child_stdin();
    child_out_ = proc_->child_stdout();
}

MatchChildClient::~MatchChildClient() = default;

bool MatchChildClient::WriteProto(const lgtbot::ipc::GameRequest& req)
{
    if (!child_in_) return false;
    std::string buf;
    if (!req.SerializeToString(&buf)) return false;
    return WriteFrame(child_in_, buf);
}

// static
std::optional<ChildFrame> MatchChildClient::ParseFrame(const std::string& raw)
{
    lgtbot::ipc::GameResponse resp;
    if (!resp.ParseFromString(raw)) return std::nullopt;

    switch (resp.resp_case()) {
    case lgtbot::ipc::GameResponse::kPost: {
        PostFrame f;
        f.post = resp.post();
        return f;
    }
    case lgtbot::ipc::GameResponse::kPlayerState: {
        const auto& ps = resp.player_state();
        return PlayerStateFrame{PlayerID{ps.pid()}, ps.state()};
    }
    case lgtbot::ipc::GameResponse::kGameOver:
        return GameOverFrame{resp.game_over()};
    case lgtbot::ipc::GameResponse::kReply:
        return ReplyFrame{resp.reply()};
    case lgtbot::ipc::GameResponse::kResult:
        return ResultFrame{resp.result().stage()};
    case lgtbot::ipc::GameResponse::kAck:
        return AckFrame{resp.ack().ok()};
    default:
        return std::nullopt;
    }
}

bool MatchChildClient::SendInit(const std::string& resource_dir, const std::string& saved_image_dir,
                                const bool public_timer_alert, const uint32_t bench, const uint32_t is_formal)
{
    lgtbot::ipc::GameRequest req;
    auto* init = req.mutable_init();
    init->set_resource_dir(resource_dir);
    init->set_saved_image_dir(saved_image_dir);
    init->set_public_timer_alert(public_timer_alert);
    init->set_bench(bench);
    init->set_is_formal(is_formal);
    if (!WriteProto(req)) return false;
    std::string raw;
    if (!ReadFrame(child_out_, raw)) return false;
    lgtbot::ipc::GameResponse resp;
    return resp.ParseFromString(raw) && resp.has_ack() && resp.ack().ok();
}

bool MatchChildClient::SendSetOption(const std::string& text)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_set_option()->set_text(text);
    if (!WriteProto(req)) return false;
    std::string raw;
    if (!ReadFrame(child_out_, raw)) return false;
    lgtbot::ipc::GameResponse resp;
    return resp.ParseFromString(raw) && resp.has_ack() && resp.ack().ok();
}

bool MatchChildClient::SendStart(const uint64_t match_id, const uint32_t user_num,
                                 const std::vector<lgtbot::ipc::PlayerInfo>& players,
                                 std::vector<PushFrame>& push_frames_out)
{
    lgtbot::ipc::GameRequest req;
    auto* start = req.mutable_start();
    start->set_match_id(match_id);
    start->set_user_num(user_num);
    for (const auto& p : players) {
        *start->add_players() = p;
    }
    if (!WriteProto(req)) return false;

    for (;;) {
        std::string raw;
        if (!ReadFrame(child_out_, raw)) return false;
        auto frame_opt = ParseFrame(raw);
        if (!frame_opt) continue;

        // Returns nullopt to continue, or the bool result to return.
        const auto result = std::visit(Overload{
            [&push_frames_out](PostFrame&& f)        -> std::optional<bool> { push_frames_out.push_back(std::move(f)); return std::nullopt; },
            [&push_frames_out](PlayerStateFrame&& f) -> std::optional<bool> { push_frames_out.push_back(std::move(f)); return std::nullopt; },
            [&push_frames_out](GameOverFrame&& f)    -> std::optional<bool> { push_frames_out.push_back(std::move(f)); return std::nullopt; },
            [](AckFrame&& f)                         -> std::optional<bool> { return f.ok; },
            [](auto&&)                               -> std::optional<bool> { return false; },
        }, std::move(*frame_opt));
        if (result.has_value()) {
            return *result;
        }
    }
}

void MatchChildClient::RunReadLoop(IMatchChildCallbacks& cbs)
{
    for (;;) {
        {
            std::lock_guard<std::mutex> lk(pending_mutex_);
            if (stopped_) break;
        }
        std::string raw;
        if (!ReadFrame(child_out_, raw)) {
            // EOF or error: unblock any WaitForResponse_ caller.
            bool unexpected;
            {
                std::lock_guard<std::mutex> lk(pending_mutex_);
                unexpected = !stopped_; // stopped_ == true means SignalStop() was called first
                stopped_ = true;
                if (!pending_ready_) {
                    pending_frame_ = std::nullopt;
                    pending_ready_ = true;
                }
                pending_cv_.notify_all();
            }
            cbs.OnEof(unexpected);
            break;
        }
        auto frame_opt = ParseFrame(raw);
        if (!frame_opt) continue;

        std::visit(Overload{
            [&](PostFrame&& f)        { cbs.OnPost(f); },
            [&](PlayerStateFrame&& f) { cbs.OnPlayerState(f); },
            [&](GameOverFrame&& f)    { cbs.OnGameOver(f); },
            [&](auto&& f)             { SetPendingFrame_(std::move(f)); },
        }, std::move(*frame_opt));
    }
}

void MatchChildClient::CloseInput()
{
    if (proc_) {
        proc_->close_stdin();
    }
    child_in_ = nullptr; // keep in sync so WriteProto returns false
}

void MatchChildClient::SignalStop()
{
    {
        std::lock_guard<std::mutex> lk(pending_mutex_);
        stopped_ = true;
        if (!pending_ready_) {
            pending_frame_ = std::nullopt;
            pending_ready_ = true;
        }
    }
    pending_cv_.notify_all();
    // Kill the child so the pipe EOF unblocks ReadFrame in the read thread.
    if (proc_) {
        proc_->request_stop();
    }
}

void MatchChildClient::SetPendingFrame_(ResponseFrame frame)
{
    {
        std::lock_guard<std::mutex> lk(pending_mutex_);
        pending_frame_ = std::move(frame);
        pending_ready_ = true;
    }
    pending_cv_.notify_one();
}

std::optional<ResponseFrame> MatchChildClient::WaitForResponse_()
{
    std::unique_lock<std::mutex> lk(pending_mutex_);
    pending_cv_.wait(lk, [this] { return pending_ready_; });
    pending_ready_ = false;
    return std::move(pending_frame_);
}

ErrCode MatchChildClient::SendExecute(const PlayerID player_id, const bool is_public,
                                      const std::string& text,
                                      std::function<void(const lgtbot::ipc::ReplyResp&)> reply_cb)
{
    lgtbot::ipc::GameRequest req;
    auto* exec = req.mutable_execute();
    exec->set_text(text);
    exec->set_player_id(player_id.Get());
    exec->set_is_public(is_public);
    std::string buf;
    if (!req.SerializeToString(&buf) || !WriteFrame(child_in_, buf)) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    for (;;) {
        auto resp_opt = WaitForResponse_();
        if (!resp_opt) return EC_MATCH_UNEXPECTED_CONFIG;
        const auto rc = std::visit(Overload{
            [](ResultFrame&& f)  -> std::optional<ErrCode> { return StageToErr(f.stage); },
            [&](ReplyFrame&& f)  -> std::optional<ErrCode> { if (reply_cb) reply_cb(f.reply); return std::nullopt; },
            [](auto&&)           -> std::optional<ErrCode> { return EC_MATCH_UNEXPECTED_CONFIG; },
        }, std::move(*resp_opt));
        if (rc.has_value()) return *rc;
    }
}

bool MatchChildClient::SendLeave(const PlayerID player_id)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_leave()->set_player_id(player_id.Get());
    std::string buf;
    if (!req.SerializeToString(&buf) || !WriteFrame(child_in_, buf)) {
        return false;
    }
    auto resp_opt = WaitForResponse_();
    if (!resp_opt) return false;
    return std::visit(Overload{
        [](AckFrame&& f) { return f.ok; },
        [](auto&&)       { return false; },
    }, std::move(*resp_opt));
}

bool MatchChildClient::FetchHelp(const bool text_mode, std::string& text_out)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_help()->set_text_mode(text_mode);
    std::string buf;
    if (!req.SerializeToString(&buf) || !WriteFrame(child_in_, buf)) {
        return false;
    }
    auto resp_opt = WaitForResponse_();
    if (!resp_opt) return false;
    return std::visit(Overload{
        [&text_out](ReplyFrame&& f) {
            if (f.reply.items_size() == 0) return false;
            text_out = f.reply.items(0).text();
            return true;
        },
        [](auto&&) { return false; },
    }, std::move(*resp_opt));
}
