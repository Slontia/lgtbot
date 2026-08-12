// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match.h"

#include <cassert>
#include <algorithm>
#include <ranges>
#include <random>
#include <utility>

#ifdef TEST_BOT
#include <functional>
extern std::function<void()> g_match_test_after_game_child_started;
#endif

#include "utility/log.h"
#include "utility/overloaded.h"
#include "utility/msg_checker.h"
#include "bot_core/match_manager.h"
#include "bot_core/match_internal.h"
#include "bot_core/options.h"
#include "match_process/match_ipc.pb.h"
#include "nlohmann/json.hpp"

namespace {

MatchPhaseCommon& PhaseCommon(std::variant<Lobby, Running>& phase)
{
if (auto* r = std::get_if<Running>(&phase)) {
    return *r;
}
    return std::get<Lobby>(phase);
}

const MatchPhaseCommon& PhaseCommon(const std::variant<Lobby, Running>& phase)
{
if (auto* r = std::get_if<Running>(&phase)) {
    return *r;
}
    return std::get<Lobby>(phase);
}

} // namespace

Match::Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, InitOptions init_options,
        const UserID host_uid, const std::optional<GroupID> gid)
    : ctx_{bot, mid, game_handle, gid}
    , group_sender_{}
    , data_{ctx_, &messaging_, host_uid, std::move(init_options)}
{
    if (gid.has_value()) {
        group_sender_.emplace(bot.MakeMsgSender(*gid));
    } else {
        group_sender_.reset();
    }
    messaging_.group_sender = &group_sender_;
    messaging_.private_broadcast_scratch = &private_broadcast_scratch_;

    help_.try_help_command = [this](MsgReader& reader, MsgSender& reply) -> bool {
        return help_cmd_.CallIfValid(reader, reply);
    };
    help_.help_command_info = [this](const bool with_example, const bool with_html_color) -> std::string {
        return help_cmd_.Info(with_example, with_html_color);
    };
    help_.fetch_lobby_help = [this](MsgSenderBase& reply, const bool text_mode) {
        FetchHelp_(reply, text_mode);
    };
}

Match::~Match() = default;

void Match::Unbind_()
{
    match_manager().UnbindMatch(ctx_.mid);
    if (ctx_.gid.has_value()) {
        match_manager().UnbindMatch(*ctx_.gid);
    }
}

void Match::CleanupRunningUsers_(MatchData& data)
{
    state_.store(MATCH_IS_OVER, std::memory_order_release);
    for (auto& [uid, user] : data.users) {
        if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
            match_manager().UnbindMatch(uid);
        }
    }
}

void Match::ReleaseGameChild_(MatchData& data)
{
    data.game_child.reset();
}

void Match::CleanupRunning_(MatchData& data)
{
    CleanupRunningUsers_(data);
    ReleaseGameChild_(data);
}

MatchManager& Match::match_manager()
{
    return ctx_.bot.match_manager();
}

MsgSenderBase& Match::BoardcastMsgSender()
{
    return PhaseCommon(data_.lock()->phase).BoardcastMsgSender();
}

MsgSenderBase& Match::TellMsgSender(const PlayerID pid)
{
    return PhaseCommon(data_.lock()->phase).TellMsgSender(pid);
}

MsgSenderBase& Match::GroupMsgSender()
{
    return PhaseCommon(data_.lock()->phase).GroupMsgSender();
}

const char* Match::PlayerName(const PlayerID& pid)
{
    return PhaseCommon(data_.lock()->phase).PlayerName(pid);
}

const char* Match::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    return PhaseCommon(data_.lock()->phase).PlayerAvatar(pid, size);
}

MsgSenderBase::MsgSenderGuard Match::BoardcastAtAll()
{
    return PhaseCommon(data_.lock()->phase).BoardcastAtAll();
}

size_t Match::UserNum() const
{
    return PhaseCommon(data_.lock()->phase).UserNum();
}

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    return PhaseCommon(data_.lock()->phase).ConvertPid(pid);
}

void Match::BriefInfo(std::string& out) const
{
    auto&& g = data_.lock();
    PhaseCommon(g->phase).BriefInfo(out);
}





ErrCode Match::SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num)
{
    auto&& g = data_.lock();
    auto* p = std::get_if<Lobby>(&g->phase);
    if (!p || state_.load(std::memory_order_acquire) == MATCH_IS_STARTING) {
        reply() << "[错误] 设置失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->SetBenchTo(uid, reply, bench_computers_to_player_num);
}

ErrCode Match::SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal)
{
    auto&& g = data_.lock();
    auto* p = std::get_if<Lobby>(&g->phase);
    if (!p || state_.load(std::memory_order_acquire) == MATCH_IS_STARTING) {
        reply() << "[错误] 设置失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->SetFormal(uid, reply, is_formal);
}

ErrCode Match::Join(const UserID uid, MsgSenderBase& reply)
{
    auto&& g = data_.lock();
    auto* p = std::get_if<Lobby>(&g->phase);
    if (!p || state_.load(std::memory_order_acquire) == MATCH_IS_STARTING) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    const auto self = shared_from_this();
    if (!match_manager().BindMatch(uid, self)) {
        const auto existing = match_manager().GetMatch(uid);
        if (!existing || existing.get() != this) {
            reply() << "[错误] 加入失败：您已加入其他游戏，您可通过私信裁判「" META_COMMAND_SIGN
                       "游戏信息」查看该游戏信息";
            return EC_MATCH_USER_ALREADY_IN_OTHER_MATCH;
        }
        reply() << "[错误] 加入失败：您已加入该游戏";
        return EC_MATCH_USER_ALREADY_IN_MATCH;
    }
    auto rc = p->Join(uid, reply);
    if (rc != EC_OK) {
        match_manager().UnbindMatch(uid);
    }
    return rc;
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply)
{
    ErrCode err_out = EC_OK;
    std::optional<std::future<ErrCode>> exec_fut;
    {
        auto&& g = data_.lock();
        if (auto* lobby = std::get_if<Lobby>(&g->phase)) {
            if (!g->game_child) {
                g = {};
                if (const auto rc = EnsureLobbyChild_(reply); rc != EC_OK) {
                    return rc;
                }
                g = data_.lock();
            }
            return lobby->Request(uid, gid, msg, reply, weak_from_this(), *g->game_child);
        }
        exec_fut = std::get<Running>(g->phase).BeginExecuteRequest(uid, gid, msg, reply, err_out, weak_from_this());
    }
    if (!exec_fut) {
        return err_out;
    }
    ErrCode rc = std::move(*exec_fut).get();
    DrainPendingChildIpc_();
    {
        auto&& g = data_.lock();
        if (auto* running = std::get_if<Running>(&g->phase)) {
            rc = running->FinishExecuteRequest(rc);
        }
    }
    if (rc == EC_GAME_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的游戏指令，您可以通过「帮助」（不带" META_COMMAND_SIGN
                   "号）查看所有支持的游戏指令\n"
                   "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN
                   "帮助」查看所有支持的元指令";
    }
    ReleaseGameChildIfOver();
    return rc;
}

ErrCode Match::Leave(const UserID uid, MsgSenderBase& reply, const bool force)
{
    ErrCode rc = EC_OK;
    std::optional<std::future<MatchChildClient::IpcStage>> child_leave_out;
    {
        auto&& g = data_.lock();
        if (auto* running = std::get_if<Running>(&g->phase)) {
            rc = running->LeaveBeforeChild(uid, reply, force, child_leave_out);
        } else {
            rc = std::get<Lobby>(g->phase).Leave(uid, reply, force);
        }
        if (rc != EC_OK) {
            return rc;
        }
        match_manager().UnbindMatch(uid);
        std::visit(overloaded{
            [&](Running& phase) {
                if (phase.is_over()) {
                    CleanupRunning_(*g);
                }
            },
            [&](Lobby&) {
                if (g->users.empty()) {
                    Unbind_();
                }
            },
        }, g->phase);
    }
    if (child_leave_out) {
        (void)std::move(*child_leave_out).get();
    }
    DrainPendingChildIpc_();
    ReleaseGameChildIfOver();
    return rc;
}

ErrCode Match::UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel)
{
    auto&& g = data_.lock();
    auto rc = PhaseCommon(g->phase).UserInterrupt(uid, reply, cancel);
    std::visit(overloaded{
        [&](Running& phase) {
            if (phase.is_over()) {
                CleanupRunning_(*g);
                g = {};
                Unbind_();
            }
        },
        [](Lobby&) {},
    }, g->phase);
    return rc;
}

void Match::ShowInfo(MsgSenderBase& reply) const
{
    PhaseCommon(data_.lock()->phase).ShowInfo(reply, weak_from_this());
}

bool Match::SwitchHost()
{
    return PhaseCommon(data_.lock()->phase).SwitchHost();
}

UserID Match::HostUserId() const
{
    return PhaseCommon(data_.lock()->phase).HostUserId();
}

ErrCode Match::Terminate(const bool is_force)
{
    auto&& g = data_.lock();
    auto rc = PhaseCommon(g->phase).Terminate(is_force);
    std::visit(overloaded{
        [&](Running& phase) {
            if (phase.is_over()) {
                CleanupRunning_(*g);
                g = {};
                Unbind_();
            }
        },
        [&](Lobby&) {
            auto expected = MATCH_IS_STARTING;
            state_.compare_exchange_strong(expected, MATCH_NOT_STARTED, std::memory_order_acq_rel);
            for (auto& [uid, user] : g->users) {
                if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
                    match_manager().UnbindMatch(uid);
                }
            }
            g = {};
            Unbind_();
        },
    }, g->phase);
    return rc;
}

ErrCode Match::GameStart(const UserID uid, MsgSenderBase& reply)
{
    std::vector<lgtbot::ipc::PlayerInfo> players_for_child;
    uint32_t user_num = 0;
    ErrCode err_out = EC_OK;

    {
        auto&& g = data_.lock();
        if (state_.load(std::memory_order_acquire) != MATCH_NOT_STARTED) {
            reply() << "[错误] 开始失败：游戏已经开始";
            return EC_MATCH_ALREADY_BEGIN;
        }

        auto plan = std::move(std::get<Lobby>(g->phase))
                            .BeginGameStart(uid, reply, weak_from_this(), err_out);
        if (!plan.has_value()) {
            return err_out;
        }

        state_.store(MATCH_IS_STARTING, std::memory_order_release);
        players_for_child = std::move(plan->players_for_child);
        user_num = plan->user_num;
    }

    if (![&]() -> bool {
            auto&& g = data_.lock();
            return g->game_child != nullptr;
        }())
    {
        if (const auto rc = EnsureLobbyChild_(reply); rc != EC_OK) {
            return rc;
        }
    }

#ifdef TEST_BOT
    {
        auto&& g = data_.lock();
        if (g_match_test_after_game_child_started) {
            g = {};
            g_match_test_after_game_child_started();
            g = data_.lock();
        }
    }
#endif

    {
        auto&& g = data_.lock();
        if (LobbyStartAborted_()) {
            std::unique_ptr<MatchChildClient> child = std::move(g->game_child);
            RollbackLobbyStart_(*g);
            g = {};
            reply() << "[错误] 开始失败：游戏已被中断";
            return EC_MATCH_ALREADY_BEGIN;
        }
    }

    LobbyStartSnapshot snapshot;
    {
        auto&& g = data_.lock();
        if (LobbyStartAborted_()) {
            std::unique_ptr<MatchChildClient> child = std::move(g->game_child);
            RollbackLobbyStart_(*g);
            g = {};
            reply() << "[错误] 开始失败：游戏已被中断";
            return EC_MATCH_ALREADY_BEGIN;
        }

        auto handoff = std::move(std::get<Lobby>(g->phase)).IntoRunning();
        assert(handoff.has_value());
        snapshot = std::move(handoff->snapshot);
    }

    CommitRunning_(std::move(snapshot));

    bool start_ok = false;
    {
        MatchChildClient* child_ptr = nullptr;
        {
            auto&& g = data_.lock();
            child_ptr = g->game_child.get();
        }
        auto start_fut = child_ptr->SendStart(ctx_.mid.Get(), user_num, players_for_child);
        start_ok = start_fut && std::move(*start_fut).get() == lgtbot::ipc::ResultResp::STAGE_OK;
    }
    DrainPendingChildIpc_();
    if (!start_ok) {
        RollbackLobbyStart_();
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    ReleaseGameChildIfOver();

    {
        auto&& g = data_.lock();
        if (auto* running = std::get_if<Running>(&g->phase)) {
            if (!running->is_over()) {
                running->BoardcastAtAll()
                    << "游戏开始，您可以使用「帮助」命令（不带" META_COMMAND_SIGN "号），查看可执行命令";

                nlohmann::json players_json_array = nlohmann::json::array();
                for (const auto& pi : players_for_child) {
                    if (pi.computer()) {
                        players_json_array.push_back(nlohmann::json{{"computer_id", pi.computer_id()}});
                    } else {
                        players_json_array.push_back(nlohmann::json{{"display_name", pi.display_name()}});
                    }
                }
                running->Boardcast()
                    << nlohmann::json{{"match_id", ctx_.mid.Get()},
                                      {"state", "started"},
                                      {"players", std::move(players_json_array)}}.dump();
            }
        }
    }

    return EC_OK;
}

ErrCode Match::EnsureLobbyChild_(MsgSenderBase& reply)
{
    const auto self = shared_from_this();
    const ChildIpcPushHandler push_handler = [self](const PushFrame& frame) {
        self->ApplyChildIpcFromReadThread_(frame);
    };
    const ChildIpcEofHandler eof_handler = [self](const bool unexpected) {
        self->ApplyChildEofFromReadThread_(unexpected);
    };
    auto g = data_.lock();
    const auto* lobby = std::get_if<Lobby>(&g->phase);
    if (!lobby) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    const auto applied_log = lobby->AppliedOptionsLog();
    const auto child_runtime_options = lobby->ChildRuntimeOptions();
    g = {};
    auto new_child = MakeMatchChildClient(ResolveRunnerExe(),
            GameLibraryPath(ctx_.bot, ctx_.game_handle), child_runtime_options,
            push_handler, eof_handler);
    if (!new_child) {
        reply() << "[错误] 建立失败：无法启动游戏子进程";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    g = data_.lock();
    g->game_child = std::move(new_child);
    for (const auto& line : applied_log) {
        auto opt_fut = g->game_child->SendSetOption(line);
        if (!opt_fut || std::move(*opt_fut).get() != lgtbot::ipc::ResultResp::STAGE_OK) {
            g->game_child.reset();
            reply() << "[错误] 建立失败：无法同步游戏设置到子进程";
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
    }
    return EC_OK;
}

void Match::CommitRunning_(LobbyStartSnapshot snapshot)
{
    auto&& g = data_.lock();
    auto gs = std::move(group_sender_);
    g->phase.template emplace<Running>(ctx_, &messaging_, &help_, g->users,
            g->players, std::move(snapshot), g->game_child.get(), std::move(gs));
    state_.store(MATCH_IS_STARTED, std::memory_order_release);
    std::get<Running>(g->phase).BindMsgSenderMatch(weak_from_this());
}

void Match::RollbackLobbyStart_(MatchData& data)
{
    std::move(std::get<Lobby>(data.phase)).RollbackPreparedStart();
    state_.store(MATCH_NOT_STARTED, std::memory_order_release);
}

void Match::RollbackLobbyStart_()
{
    std::unique_ptr<MatchChildClient> child;
    auto&& g = data_.lock();
    RollbackLobbyStart_(*g);
    child = std::move(g->game_child);
    g = {};
}

bool Match::LobbyStartAborted_()
{
    return state_.load(std::memory_order_acquire) != MATCH_IS_STARTING;
}

void Match::DrainPendingChildIpc_()
{
    std::vector<std::future<void>> futs;
    {
        std::lock_guard<std::mutex> lk(pending_ipc_mutex_);
        futs = std::move(pending_ipc_futures_);
    }
    for (auto& fut : futs) {
        fut.wait();
    }
}

void Match::ApplyChildIpcFromReadThreadImpl_(const PushFrame frame)
{
    bool finalize_over = false;
    {
        auto&& g = data_.lock();
        if (!std::holds_alternative<Running>(g->phase)) {
            return;
        }
        auto& running = std::get<Running>(g->phase);
        g = {};
        running.ApplyChildIpcFromReadThread(frame);
        finalize_over = running.is_over();
    }
    if (finalize_over) {
        auto&& g = data_.lock();
        CleanupRunningUsers_(*g);
        g = {};
        Unbind_();
    }
}

void Match::ApplyChildIpcFromReadThread_(const PushFrame& frame)
{
    const auto self = shared_from_this();
    auto fut = ctx_.bot.PostCleanup([self, frame] { self->ApplyChildIpcFromReadThreadImpl_(frame); });
    std::lock_guard<std::mutex> lk(pending_ipc_mutex_);
    pending_ipc_futures_.push_back(std::move(fut));
}

void Match::ApplyChildEofFromReadThread_(const bool unexpected)
{
    const auto self = shared_from_this();
    auto fut = ctx_.bot.PostCleanup([self, unexpected] { self->ApplyChildEofFromReadThreadImpl_(unexpected); });
    std::lock_guard<std::mutex> lk(pending_ipc_mutex_);
    pending_ipc_futures_.push_back(std::move(fut));
}

void Match::ApplyChildEofFromReadThreadImpl_(const bool unexpected)
{
    bool finalize_over = false;
    {
        auto&& g = data_.lock();
        if (!std::holds_alternative<Running>(g->phase)) {
            return;
        }
        auto& running = std::get<Running>(g->phase);
        g = {};
        running.ApplyChildEofFromReadThread(unexpected);
        finalize_over = running.is_over();
    }
    if (finalize_over) {
        auto&& g = data_.lock();
        CleanupRunningUsers_(*g);
        g = {};
        Unbind_();
    }
}

void Match::Help_(MsgSenderBase& reply, const bool text_mode)
{
    std::string remote;
    HelpTextCollector help_collector(remote);
    std::optional<std::future<MatchChildClient::IpcStage>> help_fut;
    {
        auto&& g = data_.lock();
        if (auto* r = std::get_if<Running>(&g->phase)) {
            help_fut = r->BeginFetchHelp(text_mode, help_collector);
            if (!help_fut) {
                reply() << "[错误] 无法从游戏进程获取帮助信息";
                return;
            }
        } else {
            FetchHelp_(reply, text_mode);
            return;
        }
    }
    const auto stage = std::move(*help_fut).get();
    DrainPendingChildIpc_();
    auto&& g = data_.lock();
    if (auto* r = std::get_if<Running>(&g->phase)) {
        r->FinishFetchHelp(reply, text_mode, remote, stage);
    }
}

void Match::FetchHelp_(MsgSenderBase& reply, const bool text_mode)
{
    const std::string remote_opts = ctx_.game_handle.ConfigClient().QueryOptionInfo(text_mode);
    std::string outstr = "## 当前可使用的游戏命令";
    outstr += "\n\n### 查看信息";
    outstr += "\n1. " + help_cmd_.Info(true /* with_example */, !text_mode /* with_html_color */);
    if (!remote_opts.empty()) {
        outstr += "\n\n### 配置选项";
        outstr += "\n" + remote_opts;
    }
    if (text_mode) {
        reply() << outstr;
    } else {
        reply() << Markdown(outstr);
    }
}

void Match::BindMsgSenderMatch_()
{
    const auto wk = weak_from_this();
    auto&& g = data_.lock();
    for (auto& [_, user_info] : g->users) {
        user_info.sender_.SetMatch(wk);
    }
    if (group_sender_.has_value()) {
        group_sender_->SetMatch(wk);
    }
}

void Match::ReleaseGameChildIfOver()
{
    std::unique_ptr<MatchChildClient> child;
    auto&& g = data_.lock();
    if (const auto* running = std::get_if<Running>(&g->phase); running && running->is_over()) {
        child = std::move(g->game_child);
    }
    g = {};
}

