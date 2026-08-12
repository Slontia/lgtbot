// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_child_client.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <utility>

#include "bot_core/bot_core.h"
#include "bot_core/match_internal.h"
#include "match_process/ipc_frame.h"
#include "match_process/match_ipc.pb.h"
#include "bot_core/subprocess.h"
#include "utility/log.h"
#include "utility/utils.h"

namespace {

constexpr const char* kGameOverReplyText = "游戏已经结束";
constexpr size_t kMaxRawPayloadLogBytes = 4096;

std::string FormatMakeClientContext(const std::filesystem::path& runner_exe,
                                    const std::filesystem::path& game_library,
                                    const MatchChildClient::RuntimeOptions* const options = nullptr)
{
    std::ostringstream oss;
    oss << "runner=" << runner_exe.string() << ", game=" << game_library.string();
    if (options != nullptr) {
        oss << ", resource_dir=" << options->resource_holder_.resource_dir_
            << ", saved_image_dir=" << options->resource_holder_.saved_image_dir_
            << ", public_timer_alert=" << Bool2Str(options->public_timer_alert_)
            << ", bench=" << options->generic_options_.bench_computers_to_player_num_
            << ", is_formal=" << Bool2Str(options->generic_options_.is_formal_);
    }
    return oss.str();
}

std::string FormatRawPayload(const std::string& raw)
{
    std::ostringstream oss;
    oss << "size=" << raw.size();
    if (raw.empty()) {
        return oss.str();
    }
    oss << " hex=" << std::hex << std::setfill('0');
    const size_t limit = std::min(raw.size(), kMaxRawPayloadLogBytes);
    for (size_t i = 0; i < limit; ++i) {
        if (i > 0) {
            oss << ' ';
        }
        oss << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(raw[i]));
    }
    if (raw.size() > limit) {
        oss << " ...(truncated)";
    }
    return oss.str();
}

ErrCode StageToErr(const lgtbot::ipc::ResultResp::Stage s)
{
    using S = lgtbot::ipc::ResultResp;
    switch (s) {
    case S::STAGE_OK:        return EC_GAME_REQUEST_OK;
    case S::STAGE_CHECKOUT:  return EC_GAME_REQUEST_CHECKOUT;
    case S::STAGE_FAILED:    return EC_GAME_REQUEST_FAILED;
    case S::STAGE_CONTINUE:  return EC_GAME_REQUEST_CONTINUE;
    case S::STAGE_NOT_FOUND: return EC_GAME_REQUEST_NOT_FOUND;
    default:
        ErrorLog() << "MatchChildClient: unknown ResultResp stage=" << static_cast<int>(s);
        return EC_GAME_REQUEST_UNKNOWN;
    }
}

void ReplyRespToMsgSender(MsgSenderBase& reply, const lgtbot::ipc::ReplyResp& resp)
{
    auto g = reply();
    for (const auto& item : resp.items()) {
        AppendMsgItem(g, item);
    }
}

lgtbot::ipc::ReplyResp MakeGameOverReply()
{
    lgtbot::ipc::ReplyResp reply;
    reply.add_items()->set_text(kGameOverReplyText);
    return reply;
}

} // namespace

MatchChildClient::MatchChildClient(Subprocess proc)
    : proc_(std::move(proc))
{}

MatchChildClient::~MatchChildClient() { proc_.Close(); }

std::unique_ptr<MatchChildClient> MakeMatchChildClient(const std::filesystem::path runner_exe,
                                                       const std::filesystem::path game_library,
                                                       const MatchChildClient::RuntimeOptions& options)
{
    const std::string client_ctx = FormatMakeClientContext(runner_exe, game_library, &options);

    std::vector<std::string> argv{runner_exe.string(), game_library.string()};
    Subprocess proc(std::move(argv));
    if (!proc.Ok()) {
        ErrorLog() << "MatchChildClient: spawn failed, " << client_ctx;
        return nullptr;
    }
    auto client = std::unique_ptr<MatchChildClient>(new MatchChildClient(std::move(proc)));
    auto init_stage = client->SendInit_(options);
    if (!init_stage || *init_stage != lgtbot::ipc::ResultResp::STAGE_OK) {
        ErrorLog() << "MatchChildClient: SendInit failed, stage="
                   << (init_stage ? static_cast<int>(*init_stage) : -1) << ", " << client_ctx;
        return nullptr;
    }
    return client;
}

bool MatchChildClient::WriteProto_(lgtbot::ipc::GameRequest req)
{
    const std::lock_guard lock(write_mutex_);
    if (!proc_.Ok()) {
        ErrorLog() << "MatchChildClient: WriteProto failed, stdin closed, request=" << req.DebugString();
        return false;
    }
    std::string buf;
    if (!req.SerializeToString(&buf)) {
        ErrorLog() << "MatchChildClient: GameRequest::SerializeToString failed, request=" << req.DebugString();
        return false;
    }
    if (!proc_.Write(buf)) {
        ErrorLog() << "MatchChildClient: Write failed, request=" << req.DebugString();
        return false;
    }
    return true;
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendRequestAndRead_(
        lgtbot::ipc::GameRequest req, MsgSenderBase& reply_sender, const PushHandler& on_push)
{
    if (!WriteProto_(std::move(req))) {
        return std::nullopt;
    }
    for (;;) {
        std::string raw;
        if (!proc_.Read(raw)) {
            ErrorLog() << "MatchChildClient: Read failed unexpectedly";
            ReplyRespToMsgSender(reply_sender, MakeGameOverReply());
            return std::nullopt;
        }
        lgtbot::ipc::GameResponse resp;
        if (!resp.ParseFromString(raw)) {
            ErrorLog() << "MatchChildClient: failed to parse GameResponse, " << FormatRawPayload(raw);
            continue;
        }
        switch (resp.resp_case()) {
        case lgtbot::ipc::GameResponse::kPost:
            if (on_push) {
                PostFrame f;
                f.post = resp.post();
                on_push(f);
            }
            break;
        case lgtbot::ipc::GameResponse::kPlayerState: {
            if (on_push) {
                const auto& ps = resp.player_state();
                on_push(PlayerStateFrame{PlayerID{ps.pid()}, ps.state()});
            }
            break;
        }
        case lgtbot::ipc::GameResponse::kGameOver:
            if (on_push) {
                GameOverFrame f;
                f.game_over = resp.game_over();
                on_push(f);
            }
            break;
        case lgtbot::ipc::GameResponse::kReply:
            ReplyRespToMsgSender(reply_sender, resp.reply());
            break;
        case lgtbot::ipc::GameResponse::kResult:
            return resp.result().stage();
        default:
            ErrorLog() << "MatchChildClient: unknown GameResponse resp_case="
                       << static_cast<int>(resp.resp_case()) << " response=" << resp.DebugString();
            break;
        }
    }
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendInit_(const RuntimeOptions& options)
{
    lgtbot::ipc::GameRequest req;
    auto* init = req.mutable_init();
    init->set_resource_dir(options.resource_holder_.resource_dir_);
    init->set_saved_image_dir(options.resource_holder_.saved_image_dir_);
    init->set_public_timer_alert(options.public_timer_alert_);
    init->set_bench(options.generic_options_.bench_computers_to_player_num_);
    init->set_is_formal(options.generic_options_.is_formal_);
    static const PushHandler kNoop = [](const PushFrame&) {};
    return SendRequestAndRead_(std::move(req), EmptyMsgSender::Get(), kNoop);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendSetOption(const std::string& text)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_set_option()->set_text(text);
    static const PushHandler kNoop = [](const PushFrame&) {};
    return SendRequestAndRead_(std::move(req), EmptyMsgSender::Get(), kNoop);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendStart(const uint64_t match_id,
        const uint32_t user_num, const std::vector<lgtbot::ipc::PlayerInfo>& players,
        const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    auto* start = req.mutable_start();
    start->set_match_id(match_id);
    start->set_user_num(user_num);
    for (const auto& p : players) {
        *start->add_players() = p;
    }
    return SendRequestAndRead_(std::move(req), EmptyMsgSender::Get(), on_push);
}

std::optional<ErrCode> MatchChildClient::SendExecute(const PlayerID player_id, const bool is_public,
                                                     const std::string& text, MsgSender& reply,
                                                     const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    auto* exec = req.mutable_execute();
    exec->set_text(text);
    exec->set_player_id(player_id.Get());
    exec->set_is_public(is_public);
    const auto stage = SendRequestAndRead_(std::move(req), reply, on_push);
    if (!stage) {
        return std::nullopt;
    }
    return StageToErr(*stage);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendLeave(const PlayerID player_id,
                                                                      const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_leave()->set_player_id(player_id.Get());
    static const PushHandler kNoop = [](const PushFrame&) {};
    return SendRequestAndRead_(std::move(req), EmptyMsgSender::Get(), kNoop);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::FetchHelp(const bool text_mode,
                                                                      MsgSenderBase& reply_sender)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_help()->set_text_mode(text_mode);
    static const PushHandler kNoop = [](const PushFrame&) {};
    return SendRequestAndRead_(std::move(req), reply_sender, kNoop);
}