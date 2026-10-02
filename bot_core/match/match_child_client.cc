// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match/match_child_client.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <thread>
#include <utility>

#include "bot_core/bot_core.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "bot_core/match/match_running.h"
#include "match_process/ipc_frame.h"
#include "match_process/match_ipc.pb.h"
#include "bot_core/subprocess.h"
#include "utility/log.h"
#include "utility/overloaded.h"
#include "utility/utils.h"

#ifndef MATCH_GAME_RUNNER_PATH
#define MATCH_GAME_RUNNER_PATH "match_game_runner"
#endif

namespace lgtbot {
namespace core {
namespace match {

namespace {

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

const PushHandler kNoPush{[](const PushFrame&) {}};

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
    const auto init_stage = client->SendInit_(options);
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
        lgtbot::ipc::GameRequest req, ChildMessageHandler& handler, const PushHandler& on_push)
{
    if (!WriteProto_(std::move(req))) {
        return std::nullopt;
    }
    for (;;) {
        std::string raw;
        if (!proc_.Read(raw)) {
            ErrorLog() << "MatchChildClient: Read failed unexpectedly";
            lgtbot::ipc::MsgItem item;
            item.set_text("游戏由于无法解析的子进程响应而异常结束");
            AppendMsgItem(handler, item);
            return std::nullopt;
        }
        lgtbot::ipc::GameResponse resp;
        if (!resp.ParseFromString(raw)) {
            ErrorLog() << "MatchChildClient: failed to parse GameResponse, " << FormatRawPayload(raw);
            continue;
        }
        switch (resp.resp_case()) {
        case lgtbot::ipc::GameResponse::kPost:
            on_push(resp.post());
            break;
        case lgtbot::ipc::GameResponse::kPlayerState:
            on_push(resp.player_state());
            break;
        case lgtbot::ipc::GameResponse::kGameOver:
            on_push(resp.game_over());
            break;
        case lgtbot::ipc::GameResponse::kTimerStart:
            on_push(resp.timer_start());
            break;
        case lgtbot::ipc::GameResponse::kTimerStop:
            on_push(resp.timer_stop());
            break;
        case lgtbot::ipc::GameResponse::kReply:
            for (const auto& item : resp.reply().items()) {
                AppendMsgItem(handler, item);
            }
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
    return SendRequestAndRead_(std::move(req), DoNothingHandler::Get(), kNoPush);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendSetOption(const std::string& text)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_set_option()->set_text(text);
    return SendRequestAndRead_(std::move(req), DoNothingHandler::Get(), kNoPush);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendStart(const uint64_t match_id,
        const uint32_t user_num, const std::vector<lgtbot::ipc::PlayerInfo>& players,
        const uint32_t bench, const bool is_formal, const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    auto* start = req.mutable_start();
    start->set_match_id(match_id);
    start->set_user_num(user_num);
    for (const auto& p : players) {
        *start->add_players() = p;
    }
    start->set_bench(bench);
    start->set_is_formal(is_formal);
    return SendRequestAndRead_(std::move(req), DoNothingHandler::Get(), on_push);
}

std::optional<ErrCode> MatchChildClient::SendExecute(const PlayerID player_id, const bool is_public,
                                                     const std::string& text,
                                                     ChildMessageHandler& reply_handler,
                                                     const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    auto* exec = req.mutable_execute();
    exec->set_text(text);
    exec->set_player_id(player_id.Get());
    exec->set_is_public(is_public);
    const auto stage = SendRequestAndRead_(std::move(req), reply_handler, on_push);
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
    return SendRequestAndRead_(std::move(req), DoNothingHandler::Get(), on_push);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendTimeout(const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_timeout();
    return SendRequestAndRead_(std::move(req), DoNothingHandler::Get(), on_push);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::SendAlert(const uint64_t remaining_sec,
                                                                       const PushHandler& on_push)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_alert()->set_remaining_sec(remaining_sec);
    return SendRequestAndRead_(std::move(req), DoNothingHandler::Get(), on_push);
}

std::optional<MatchChildClient::IpcStage> MatchChildClient::FetchHelp(const bool text_mode,
                                                                      std::string& text_out)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_help()->set_text_mode(text_mode);
    ExtractTextHandler handler(text_out);
    return SendRequestAndRead_(std::move(req), handler, kNoPush);
}

DoNothingHandler& DoNothingHandler::Get()
{
    static DoNothingHandler instance;
    return instance;
}

ChildMessageReplyHandler::ChildMessageReplyHandler(HostMsgSenderBase& sender, const MatchRunning* running)
    : sender_(sender)
    , running_(running)
{}

HostMsgSenderBase::MsgSenderGuard& ChildMessageReplyHandler::Guard()
{
    if (!g_) {
        g_.emplace(sender_());
    }
    return *g_;
}

void ChildMessageReplyHandler::HandleText(const std::string& text)
{
    Guard() << text;
}

void ChildMessageReplyHandler::HandleAtPlayerId(const PlayerID pid)
{
    Guard() << "[" << pid.Get() << "号";
    if (running_) {
        Guard() << "：";
        std::visit(overloaded{
            [this](const UserID id) { Guard() << At(id); },
            [this](const ComputerID id) { Guard() << "机器人" << id.Get() << "号"; },
        }, running_->ConvertPid(pid));
    }
    Guard() << "]";
}

void ChildMessageReplyHandler::HandleUserId(const UserID uid)
{
    Guard() << At(uid);
}

void ChildMessageReplyHandler::HandleImagePath(const std::string& path)
{
    Guard() << Image{path};
}

void ChildMessageReplyHandler::HandleMarkdown(const std::string& text, const uint32_t width)
{
    Guard() << Markdown(text, width);
}

ExtractTextHandler::ExtractTextHandler(std::string& out)
    : out_(out)
{}

void ExtractTextHandler::HandleText(const std::string& text)
{
    if (out_.empty()) {
        out_ = text;
    }
}

void AppendMsgItem(ChildMessageHandler& handler, const lgtbot::ipc::MsgItem& item)
{
    switch (item.content_case()) {
    case lgtbot::ipc::MsgItem::kText:
        handler.HandleText(item.text());
        break;
    case lgtbot::ipc::MsgItem::kAtPlayerId:
        handler.HandleAtPlayerId(PlayerID{item.at_player_id()});
        break;
    case lgtbot::ipc::MsgItem::kUserId:
        handler.HandleUserId(UserID{item.user_id()});
        break;
    case lgtbot::ipc::MsgItem::kImagePath:
        handler.HandleImagePath(item.image_path());
        break;
    case lgtbot::ipc::MsgItem::kMarkdown:
        handler.HandleMarkdown(item.markdown().text(), item.markdown().width());
        break;
    default:
        ErrorLog() << "unknown message item " << static_cast<int>(item.content_case());
        break;
    }
}

std::filesystem::path ResolveRunnerExe()
{
    if (const char* const e = std::getenv("LGTBOT_MATCH_RUNNER")) {
        return e;
    }
    return std::filesystem::path(MATCH_GAME_RUNNER_PATH);
}

std::filesystem::path GameLibraryPath(const BotCtx& bot, const GameHandle& gh)
{
    const auto base = std::filesystem::absolute(bot.game_path()) / gh.Info().module_name_;
#if defined(_WIN32)
    return base / "libgame.dll";
#elif defined(__APPLE__)
    return base / "libgame.dylib";
#else
    return base / "libgame.so";
#endif
}

} // namespace match
} // namespace core
} // namespace lgtbot
