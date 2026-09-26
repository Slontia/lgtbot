// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match_child_client.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "bot_core/bot_core.h"
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
                                  const MatchChildClient::RuntimeOptions* options = nullptr)
{
    std::ostringstream oss;
    oss << "runner=" << runner_exe.string() << ", game=" << game_library.string();
    if (options) {
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

void AppendMsgItem(MsgSenderBase::MsgSenderGuard& g, const lgtbot::ipc::MsgItem& item)
{
    switch (item.content_case()) {
    case lgtbot::ipc::MsgItem::kText:         g << item.text(); break;
    case lgtbot::ipc::MsgItem::kAtPlayerId:   g << At(PlayerID{item.at_player_id()}); break;
    case lgtbot::ipc::MsgItem::kUserId:       g << Name(UserID{item.user_id()}); break;
    case lgtbot::ipc::MsgItem::kImagePath:    g << Image{item.image_path()}; break;
    case lgtbot::ipc::MsgItem::kMarkdown:
        g << Markdown{item.markdown().text(), item.markdown().width()};
        break;
    default:
        WarnLog() << "MatchChildClient: unknown MsgItem content_case=" << static_cast<int>(item.content_case());
        break;
    }
}

void ReplyRespToMsgSender(MsgSenderBase& reply, const lgtbot::ipc::ReplyResp& resp)
{
    auto g = reply();
    for (const auto& item : resp.items()) {
        AppendMsgItem(g, item);
    }
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

lgtbot::ipc::ReplyResp MakeGameOverReply()
{
    lgtbot::ipc::ReplyResp reply;
    reply.add_items()->set_text(kGameOverReplyText);
    return reply;
}

std::optional<ChildFrame> ParseFrame(const std::string& raw)
{
    lgtbot::ipc::GameResponse resp;
    if (!resp.ParseFromString(raw)) {
        ErrorLog() << "MatchChildClient: failed to parse GameResponse, " << FormatRawPayload(raw);
        return std::nullopt;
    }

    const uint64_t ipc_id = resp.ipc_id();
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
        return ReplyFrame{ipc_id, resp.reply()};
    case lgtbot::ipc::GameResponse::kResult:
        return ResultFrame{ipc_id, resp.result().stage()};
    default:
        ErrorLog() << "MatchChildClient: unknown GameResponse resp_case=" << static_cast<int>(resp.resp_case())
                   << " response=" << resp.DebugString();
        return std::nullopt;
    }
}

} // namespace

MatchChildClient::PendingRequests::~PendingRequests()
{
    const auto game_over = MakeGameOverReply();
    for (auto& [id, entry] : entries_) {
        (void)id;
        ReplyRespToMsgSender(entry.reply_sender, game_over);
        entry.on_result(lgtbot::ipc::ResultResp::STAGE_FAILED);
    }
    entries_.clear();
}

void MatchChildClient::PendingRequests::Emplace(const uint64_t ipc_id, Entry entry)
{
    entries_.emplace(ipc_id, std::move(entry));
}

std::optional<MatchChildClient::PendingRequests::EntryIt> MatchChildClient::PendingRequests::FindEntry_(
        const uint64_t ipc_id)
{
    const auto it = entries_.find(ipc_id);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it;
}

void MatchChildClient::PendingRequests::DispatchReply(const uint64_t ipc_id,
                                                      const lgtbot::ipc::ReplyResp& reply)
{
    const auto it = FindEntry_(ipc_id);
    if (!it) {
        ErrorLog() << "MatchChildClient: Reply for unknown ipc_id=" << ipc_id;
        return;
    }
    ReplyRespToMsgSender((*it)->second.reply_sender, reply);
}

void MatchChildClient::PendingRequests::DispatchResult(const uint64_t ipc_id, const IpcStage stage)
{
    const auto it = FindEntry_(ipc_id);
    if (!it) {
        ErrorLog() << "MatchChildClient: Result for unknown ipc_id=" << ipc_id
                   << " stage=" << static_cast<int>(stage);
        return;
    }
    auto entry = std::move((*it)->second);
    entries_.erase(*it);
    entry.on_result(stage);
}

MatchChildClient::MatchChildClient(Subprocess proc, const ChildIpcPushHandler& dispatch_push,
                                   const ChildIpcEofHandler& on_eof)
    : proc_(std::move(proc)),
      read_thread_([this, dispatch_push, on_eof](const std::stop_token stop) {
          RunReadLoop_(stop, dispatch_push, on_eof);
      })
{
}

std::unique_ptr<MatchChildClient> MakeMatchChildClient(const std::filesystem::path runner_exe,
                                                     const std::filesystem::path game_library,
                                                     const MatchChildClient::RuntimeOptions& options,
                                                     const ChildIpcPushHandler& dispatch_push,
                                                     const ChildIpcEofHandler& on_eof)
{
    const std::string client_ctx = FormatMakeClientContext(runner_exe, game_library, &options);

    std::vector<std::string> argv{runner_exe.string(), game_library.string()};
    Subprocess proc(std::move(argv));
    if (!proc.Ok()) {
        ErrorLog() << "MatchChildClient: spawn failed, " << client_ctx;
        return nullptr;
    }
    auto client = std::unique_ptr<MatchChildClient>(new MatchChildClient(std::move(proc), dispatch_push, on_eof));
    auto init_fut = client->SendInit_(options);
    if (!init_fut) {
        ErrorLog() << "MatchChildClient: SendInit failed to write request, " << client_ctx;
        return nullptr;
    }
    const auto init_stage = init_fut->get();
    if (init_stage != lgtbot::ipc::ResultResp::STAGE_OK) {
        ErrorLog() << "MatchChildClient: SendInit failed, stage=" << static_cast<int>(init_stage) << ", " << client_ctx;
        return nullptr;
    }
    return client;
}

MatchChildClient::~MatchChildClient()
{
    // proc_ is destroyed after read_thread_ joins; shut down the child here so Read() can exit.
    proc_.Close();
}

uint64_t MatchChildClient::AllocIpcId_()
{
    return next_ipc_id_.fetch_add(1, std::memory_order_relaxed);
}

bool MatchChildClient::WriteProto_(lgtbot::ipc::GameRequest req)
{
    const std::lock_guard request_lock(request_mutex_);
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
        ErrorLog() << "MatchChildClient: Write failed, ipc_id=" << req.ipc_id()
                   << " request=" << req.DebugString();
        return false;
    }
    return true;
}

void MatchChildClient::RunReadLoop_(std::stop_token stop, const ChildIpcPushHandler& dispatch_push,
                                    const ChildIpcEofHandler& on_eof)
{
    while (!stop.stop_requested()) {
        std::string raw;
        if (!proc_.Read(raw)) {
            const bool unexpected = !stop.stop_requested();
            if (unexpected) {
                ErrorLog() << "MatchChildClient: Read failed unexpectedly";
            }
            if (on_eof) {
                on_eof(unexpected);
            }
            break;
        }
        auto frame_opt = ParseFrame(raw);
        if (!frame_opt) {
            continue;
        }

        std::visit(Overload{
            [&](PostFrame&& f) { dispatch_push(PushFrame{std::move(f)}); },
            [&](PlayerStateFrame&& f) { dispatch_push(PushFrame{std::move(f)}); },
            [&](GameOverFrame&& f) { dispatch_push(PushFrame{std::move(f)}); },
            [&](ReplyFrame&& f) { pending_.lock()->DispatchReply(f.ipc_id, f.reply); },
            [&](ResultFrame&& f) { pending_.lock()->DispatchResult(f.ipc_id, f.stage); },
        }, std::move(*frame_opt));
    }
}

template<typename T, typename Handler>
std::optional<std::future<T>> MatchChildClient::SendIpc_(lgtbot::ipc::GameRequest&& req, MsgSenderBase& reply_sender,
                                                         Handler handler)
{
    const uint64_t ipc_id = AllocIpcId_();
    req.set_ipc_id(ipc_id);
    auto promise = std::make_shared<std::promise<T>>();
    auto fut = promise->get_future();
    PendingRequests::Entry entry{
        reply_sender,
        [promise, handler = std::move(handler)](const IpcStage stage) { promise->set_value(handler(stage)); },
    };
    if (!WriteProto_(std::move(req))) {
        return std::nullopt;
    }
    pending_.lock()->Emplace(ipc_id, std::move(entry));
    return std::move(fut);
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::SendIpcStage_(lgtbot::ipc::GameRequest&& req,
                                                                                      MsgSenderBase& reply_sender)
{
    return SendIpc_<IpcStage>(std::move(req), reply_sender, [](const IpcStage stage) { return stage; });
}

std::optional<std::future<ErrCode>> MatchChildClient::SendIpcErrCode_(lgtbot::ipc::GameRequest&& req,
                                                                     MsgSenderBase& reply_sender)
{
    return SendIpc_<ErrCode>(std::move(req), reply_sender, StageToErr);
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::SendInit_(const RuntimeOptions& options)
{
    lgtbot::ipc::GameRequest req;
    auto* init = req.mutable_init();
    init->set_resource_dir(options.resource_holder_.resource_dir_);
    init->set_saved_image_dir(options.resource_holder_.saved_image_dir_);
    init->set_public_timer_alert(options.public_timer_alert_);
    init->set_bench(options.generic_options_.bench_computers_to_player_num_);
    init->set_is_formal(options.generic_options_.is_formal_);
    return SendIpcStage_(std::move(req), EmptyMsgSender::Get());
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::SendSetOption(const std::string& text)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_set_option()->set_text(text);
    return SendIpcStage_(std::move(req), EmptyMsgSender::Get());
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::SendApplyInitOptions(const std::string& args)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_apply_init_options()->set_args(args);
    return SendIpcStage_(std::move(req), EmptyMsgSender::Get());
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::SendStart(const uint64_t match_id,
                                                                                 const uint32_t user_num,
                                                                                 const std::vector<lgtbot::ipc::PlayerInfo>& players)
{
    lgtbot::ipc::GameRequest req;
    auto* start = req.mutable_start();
    start->set_match_id(match_id);
    start->set_user_num(user_num);
    for (const auto& p : players) {
        *start->add_players() = p;
    }
    return SendIpcStage_(std::move(req), EmptyMsgSender::Get());
}

std::optional<std::future<ErrCode>> MatchChildClient::SendExecute(const PlayerID player_id, const bool is_public,
                                                                  const std::string& text, MsgSender& reply)
{
    lgtbot::ipc::GameRequest req;
    auto* exec = req.mutable_execute();
    exec->set_text(text);
    exec->set_player_id(player_id.Get());
    exec->set_is_public(is_public);
    return SendIpcErrCode_(std::move(req), reply);
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::SendLeave(const PlayerID player_id)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_leave()->set_player_id(player_id.Get());
    return SendIpcStage_(std::move(req), EmptyMsgSender::Get());
}

std::optional<std::future<MatchChildClient::IpcStage>> MatchChildClient::FetchHelp(const bool text_mode,
                                                                                   MsgSenderBase& reply_sender)
{
    lgtbot::ipc::GameRequest req;
    req.mutable_help()->set_text_mode(text_mode);
    return SendIpcStage_(std::move(req), reply_sender);
}
