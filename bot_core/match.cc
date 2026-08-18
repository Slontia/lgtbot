// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match.h"

#include <cassert>
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

void Match::Internal::BriefInfo(std::string& out) const
{
    PhaseCommon(phase_).BriefInfo(out);
}

ErrCode Match::Internal::SetBenchTo(UserID uid, HostMsgSenderBase& reply,
                               uint64_t bench_computers_to_player_num, bool is_starting)
{
    auto* p = std::get_if<Lobby>(&phase_);
    if (!p || is_starting) {
        reply() << "[错误] 设置失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->SetBenchTo(uid, reply, bench_computers_to_player_num);
}

ErrCode Match::Internal::SetFormal(UserID uid, HostMsgSenderBase& reply,
                              bool is_formal, bool is_starting)
{
    auto* p = std::get_if<Lobby>(&phase_);
    if (!p || is_starting) {
        reply() << "[错误] 设置失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->SetFormal(uid, reply, is_formal);
}

ErrCode Match::Internal::Join(UserID uid, HostMsgSenderBase& reply, bool is_starting)
{
    auto* p = std::get_if<Lobby>(&phase_);
    if (!p || is_starting) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->Join(uid, reply);
}

ErrCode Match::Internal::ExecuteLobbyRequest(UserID uid, std::optional<GroupID> gid,
                                        const std::string& msg, MsgSender& reply,
                                        std::weak_ptr<Match> self)
{
    auto* lobby = std::get_if<Lobby>(&phase_);
    if (!lobby || !game_child_) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    return lobby->Request(uid, gid, msg, reply, self, *game_child_);
}

std::optional<Match::Internal::LobbyOptions> Match::Internal::ReadLobbyOptions() const
{
    const auto* lobby = std::get_if<Lobby>(&phase_);
    if (!lobby) {
        return std::nullopt;
    }
    return LobbyOptions{lobby->AppliedOptionsLog(), lobby->ChildRuntimeOptions()};
}

ErrCode Match::Internal::InstallGameChild(std::unique_ptr<MatchChildClient> child,
                                     const std::vector<std::string>& applied_log,
                                     HostMsgSenderBase& reply)
{
    game_child_ = std::move(child);
    for (const auto& line : applied_log) {
        const auto stage = game_child_->SendSetOption(line);
        if (!stage || *stage != lgtbot::ipc::ResultResp::STAGE_OK) {
            game_child_.reset();
            reply() << "[错误] 建立失败：无法同步游戏设置到子进程";
            return EC_MATCH_UNEXPECTED_CONFIG;
        }
    }
    return EC_OK;
}

std::optional<LobbyGameStartPlan> Match::Internal::BeginGameStart(UserID uid, HostMsgSenderBase& reply,
                                                             std::weak_ptr<const Match> self,
                                                             const std::atomic<MatchState>& state,
                                                             ErrCode& err_out)
{
    if (state.load(std::memory_order_acquire) != MATCH_NOT_STARTED) {
        reply() << "[错误] 开始失败：游戏已经开始";
        err_out = EC_MATCH_ALREADY_BEGIN;
        return std::nullopt;
    }
    return std::move(std::get<Lobby>(phase_)).BeginGameStart(uid, reply, self, err_out);
}

std::optional<LobbyRunningHandoff> Match::Internal::TryExtractRunningHandoff(
    std::atomic<MatchState>& state,
    std::unique_ptr<MatchChildClient>& aborted_child_out)
{
    if (state.load(std::memory_order_acquire) != MATCH_IS_STARTING) {
        std::move(std::get<Lobby>(phase_)).RollbackPreparedStart();
        state.store(MATCH_NOT_STARTED, std::memory_order_release);
        aborted_child_out = std::move(game_child_);
        return std::nullopt;
    }
    return std::move(std::get<Lobby>(phase_)).IntoRunning();
}

void Match::Internal::CommitRunning(LobbyStartSnapshot snapshot,
                               std::optional<MsgSender> group_sender,
                               const MatchContext& ctx, MatchMessaging* messaging,
                               MatchHelpServices* help,
                               std::weak_ptr<Match> self,
                               std::atomic<MatchState>& state)
{
    phase_.emplace<Running>(ctx, messaging, help, users_, players_,
                            std::move(snapshot), game_child_.get(), std::move(group_sender));
    state.store(MATCH_IS_STARTED, std::memory_order_release);
    std::get<Running>(phase_).BindMatch(std::move(self));
}

Match::Internal::PhaseResult Match::Internal::ExecuteSendStart(MatchID mid, uint32_t user_num,
                                                    const std::vector<lgtbot::ipc::PlayerInfo>& players)
{
    auto& running = std::get<Running>(phase_);
    auto on_push = [&running](const PushFrame& f) { running.ApplyChildPushFrame(f); };
    const auto stage = game_child_->SendStart(mid.Get(), user_num, players, on_push, &running);
    if (!stage) {
        if (!running.is_over()) {
            running.HandleChildEof();
        }
        return {EC_MATCH_UNEXPECTED_CONFIG, true, nullptr};
    }
    if (*stage != lgtbot::ipc::ResultResp::STAGE_OK) {
        running.HandleChildEof();
        return {EC_MATCH_UNEXPECTED_CONFIG, true, nullptr};
    }
    return {EC_OK, running.is_over(), nullptr};
}

void Match::Internal::BroadcastGameStartedImpl_(MatchID mid,
                                           const std::vector<lgtbot::ipc::PlayerInfo>& players)
{
    auto& running = std::get<Running>(phase_);
    running.BoardcastAtAll()
        << "游戏开始，您可以使用「帮助」命令（不带" META_COMMAND_SIGN "号），查看可执行命令";
    nlohmann::json players_json_array = nlohmann::json::array();
    for (const auto& pi : players) {
        if (pi.computer()) {
            players_json_array.push_back(nlohmann::json{{"computer_id", pi.computer_id()}});
        } else {
            players_json_array.push_back(nlohmann::json{{"display_name", pi.display_name()}});
        }
    }
    running.Boardcast()
        << nlohmann::json{{"match_id", mid.Get()},
                          {"state", "started"},
                          {"players", std::move(players_json_array)}}.dump();
}

std::unique_ptr<MatchChildClient> Match::Internal::RollbackLobbyStart(std::atomic<MatchState>& state)
{
    std::move(std::get<Lobby>(phase_)).RollbackPreparedStart();
    state.store(MATCH_NOT_STARTED, std::memory_order_release);
    return std::move(game_child_);
}

void Match::Internal::ExecuteAlert(uint64_t remaining_sec,
                              const std::shared_ptr<std::atomic<bool>>& tio)
{
    if (tio->load(std::memory_order_acquire)) {
        return;
    }
    auto& running = std::get<Running>(phase_);
    if (running.is_over()) {
        return;
    }
    auto on_push = [&running](const PushFrame& f) { running.ApplyChildPushFrame(f); };
    (void)game_child_->SendAlert(remaining_sec, on_push, &running);
}

template <std::invocable<UserID> F>
std::unique_ptr<MatchChildClient> Match::Internal::CleanupRunning(
    std::atomic<MatchState>& state, F&& unbind_user)
{
    state.store(MATCH_IS_OVER, std::memory_order_release);
    for (auto& [uid, user] : users_) {
        if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
            unbind_user(uid);
        }
    }
    return std::move(game_child_);
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteRunningRequest(
    UserID uid, std::optional<GroupID> gid,
    const std::string& msg, MsgSender& reply,
    std::weak_ptr<const Match> self,
    std::atomic<MatchState>& state, F&& unbind_user)
{
    auto& running = std::get<Running>(phase_);
    const auto rc = running.ExecuteRequest(uid, gid, msg, reply, self);
    if (running.is_over()) {
        return {rc, true, CleanupRunning(state, std::forward<F>(unbind_user))};
    }
    return {rc, false, nullptr};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteLeave(
    UserID uid, HostMsgSenderBase& reply, bool force,
    std::atomic<MatchState>& state, F&& unbind_user)
{
    if (auto* running = std::get_if<Running>(&phase_)) {
        const auto rc = running->LeaveBeforeChild(uid, reply, force);
        if (rc != EC_OK) {
            return {rc, false, nullptr};
        }
        unbind_user(uid);
        if (running->is_over()) {
            return {EC_OK, true, CleanupRunning(state, std::forward<F>(unbind_user))};
        }
        return {EC_OK, false, nullptr};
    }
    const auto rc = std::get<Lobby>(phase_).Leave(uid, reply, force);
    if (rc != EC_OK) {
        return {rc, false, nullptr};
    }
    unbind_user(uid);
    if (users_.empty()) {
        return {EC_OK, true, nullptr};
    }
    return {EC_OK, false, nullptr};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteUserInterrupt(
    UserID uid, HostMsgSenderBase& reply, bool cancel,
    std::atomic<MatchState>& state, F&& unbind_user)
{
    if (auto* running = std::get_if<Running>(&phase_)) {
        const auto rc = running->UserInterrupt(uid, reply, cancel);
        if (running->is_over()) {
            return {rc, true, CleanupRunning(state, std::forward<F>(unbind_user))};
        }
        return {rc, false, nullptr};
    }
    const auto rc = std::get<Lobby>(phase_).UserInterrupt(uid, reply, cancel);
    return {rc, false, nullptr};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteTerminate(
    bool is_force, std::atomic<MatchState>& state, F&& unbind_user)
{
    if (auto* running = std::get_if<Running>(&phase_)) {
        const auto rc = running->Terminate(is_force);
        if (running->is_over()) {
            return {rc, true, CleanupRunning(state, std::forward<F>(unbind_user))};
        }
        return {rc, false, nullptr};
    }
    const auto rc = std::get<Lobby>(phase_).Terminate(is_force);
    auto expected = MATCH_IS_STARTING;
    state.compare_exchange_strong(expected, MATCH_NOT_STARTED, std::memory_order_acq_rel);
    for (auto& [uid, user] : users_) {
        if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
            unbind_user(uid);
        }
    }
    return {rc, true, nullptr};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::BroadcastGameStarted(
    MatchID mid,
    const std::vector<lgtbot::ipc::PlayerInfo>& players,
    std::atomic<MatchState>& state, F&& unbind_user)
{
    auto* running = std::get_if<Running>(&phase_);
    if (!running) {
        return {EC_OK, false, nullptr};
    }
    if (!running->is_over()) {
        BroadcastGameStartedImpl_(mid, players);
    }
    if (running->is_over()) {
        return {EC_OK, true, CleanupRunning(state, std::forward<F>(unbind_user))};
    }
    return {EC_OK, false, nullptr};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteTimeout(
    const std::shared_ptr<std::atomic<bool>>& tio,
    std::atomic<MatchState>& state, F&& unbind_user)
{
    if (tio->load(std::memory_order_acquire)) {
        return {EC_OK, false, nullptr};
    }
    auto& running = std::get<Running>(phase_);
    if (running.is_over()) {
        return {EC_OK, false, nullptr};
    }
    auto on_push = [&running](const PushFrame& f) { running.ApplyChildPushFrame(f); };
    (void)game_child_->SendTimeout(on_push, &running);
    if (running.is_over()) {
        return {EC_OK, true, CleanupRunning(state, std::forward<F>(unbind_user))};
    }
    return {EC_OK, false, nullptr};
}

template <std::invocable<> F>
void Match::Internal::Help(HostMsgSenderBase& reply, bool text_mode, F&& lobby_help)
{
    if (auto* r = std::get_if<Running>(&phase_)) {
        r->FetchHelp(reply, text_mode);
        return;
    }
    std::forward<F>(lobby_help)();
}

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
    help_.fetch_lobby_help = [this](HostMsgSenderBase& reply, const bool text_mode) {
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

MatchManager& Match::match_manager()
{
    return ctx_.bot.match_manager();
}

HostMsgSenderBase& Match::BoardcastMsgSender()
{
    return data_.lock()->VisitPhase([](auto& p) -> decltype(auto) { return p.BoardcastMsgSender(); });
}

HostMsgSenderBase& Match::TellMsgSender(const PlayerID pid)
{
    return data_.lock()->VisitPhase([&](auto& p) -> decltype(auto) { return p.TellMsgSender(pid); });
}

HostMsgSenderBase& Match::GroupMsgSender()
{
    return data_.lock()->VisitPhase([](auto& p) -> decltype(auto) { return p.GroupMsgSender(); });
}

const char* Match::PlayerName(const PlayerID& pid)
{
    return data_.lock()->VisitPhase([&](auto& p) { return p.PlayerName(pid); });
}

const char* Match::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    return data_.lock()->VisitPhase([&](auto& p) { return p.PlayerAvatar(pid, size); });
}

HostMsgSenderBase::MsgSenderGuard Match::BoardcastAtAll()
{
    return data_.lock()->VisitPhase([](auto& p) { return p.BoardcastAtAll(); });
}

size_t Match::UserNum() const
{
    return data_.lock()->VisitPhase([](const auto& p) { return p.UserNum(); });
}

Match::VariantID Match::ConvertPid(const PlayerID pid) const
{
    return data_.lock()->VisitPhase([&](const auto& p) { return p.ConvertPid(pid); });
}

void Match::BriefInfo(std::string& out) const
{
    data_.lock()->BriefInfo(out);
}

ErrCode Match::SetBenchTo(const UserID uid, HostMsgSenderBase& reply,
                           const uint64_t bench_computers_to_player_num)
{
    return data_.lock()->SetBenchTo(uid, reply, bench_computers_to_player_num,
                                    state_.load(std::memory_order_acquire) == MATCH_IS_STARTING);
}

ErrCode Match::SetFormal(const UserID uid, HostMsgSenderBase& reply, const bool is_formal)
{
    return data_.lock()->SetFormal(uid, reply, is_formal,
                                   state_.load(std::memory_order_acquire) == MATCH_IS_STARTING);
}

ErrCode Match::Join(const UserID uid, HostMsgSenderBase& reply)
{
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
    const bool is_starting = state_.load(std::memory_order_acquire) == MATCH_IS_STARTING;
    const auto rc = data_.lock()->Join(uid, reply, is_starting);
    if (rc != EC_OK) {
        match_manager().UnbindMatch(uid);
    }
    return rc;
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid,
                        const std::string& msg, MsgSender& reply)
{
    if (!data_.lock()->HasGameChild()) {
        if (const auto rc = EnsureLobbyChild_(reply); rc != EC_OK) {
            return rc;
        }
    }
    if (data_.lock()->IsLobby()) {
        return data_.lock()->ExecuteLobbyRequest(uid, gid, msg, reply, weak_from_this());
    }
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteRunningRequest(uid, gid, msg, reply,
                                                      weak_from_this(), state_, unbind);
    if (result.is_over) {
        Unbind_();
    }
    if (result.rc == EC_GAME_REQUEST_NOT_FOUND) {
        reply() << "[错误] 未预料的游戏指令，您可以通过「帮助」（不带" META_COMMAND_SIGN
                   "号）查看所有支持的游戏指令\n"
                   "若您想执行元指令，请尝试在请求前加「" META_COMMAND_SIGN "」，或通过「" META_COMMAND_SIGN
                   "帮助」查看所有支持的元指令";
    }
    return result.rc;
}

ErrCode Match::Leave(const UserID uid, HostMsgSenderBase& reply, const bool force)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteLeave(uid, reply, force, state_, unbind);
    if (result.is_over) {
        Unbind_();
    }
    return result.rc;
}

ErrCode Match::UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteUserInterrupt(uid, reply, cancel, state_, unbind);
    if (result.is_over) {
        Unbind_();
    }
    return result.rc;
}

void Match::ShowInfo(HostMsgSenderBase& reply) const
{
    data_.lock()->VisitPhase([&](auto& p) { p.ShowInfo(reply, weak_from_this()); });
}

bool Match::SwitchHost()
{
    return data_.lock()->VisitPhase([](auto& p) { return p.SwitchHost(); });
}

UserID Match::HostUserId() const
{
    return data_.lock()->VisitPhase([](const auto& p) { return p.HostUserId(); });
}

ErrCode Match::Terminate(const bool is_force)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteTerminate(is_force, state_, unbind);
    if (result.is_over) {
        Unbind_();
    }
    return result.rc;
}

ErrCode Match::GameStart(const UserID uid, HostMsgSenderBase& reply)
{
    ErrCode err_out = EC_OK;
    auto plan = data_.lock()->BeginGameStart(uid, reply, weak_from_this(), state_, err_out);
    if (!plan) {
        return err_out;
    }
    state_.store(MATCH_IS_STARTING, std::memory_order_release);
    auto players_for_child = std::move(plan->players_for_child);
    const uint32_t user_num = plan->user_num;

    if (!data_.lock()->HasGameChild()) {
        if (const auto rc = EnsureLobbyChild_(reply); rc != EC_OK) {
            return rc;
        }
    }

#ifdef TEST_BOT
    if (g_match_test_after_game_child_started) {
        g_match_test_after_game_child_started();
    }
#endif

    std::unique_ptr<MatchChildClient> aborted_child;
    auto handoff = data_.lock()->TryExtractRunningHandoff(state_, aborted_child);
    if (!handoff) {
        reply() << "[错误] 开始失败：游戏已被中断";
        return EC_MATCH_ALREADY_BEGIN;
    }

    data_.lock()->CommitRunning(std::move(handoff->snapshot), std::move(group_sender_),
                                ctx_, &messaging_, &help_, weak_from_this(), state_);

    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };

    auto start_result = data_.lock()->ExecuteSendStart(ctx_.mid, user_num, players_for_child);
    if (start_result.rc != EC_OK) {
        auto child = data_.lock()->CleanupRunning(state_, unbind);
        Unbind_();
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    if (start_result.is_over) {
        auto child = data_.lock()->CleanupRunning(state_, unbind);
        Unbind_();
        return EC_OK;
    }

    auto post = data_.lock()->BroadcastGameStarted(ctx_.mid, players_for_child, state_, unbind);
    if (post.is_over) {
        Unbind_();
    }
    return EC_OK;
}

ErrCode Match::EnsureLobbyChild_(HostMsgSenderBase& reply)
{
    const auto opts = data_.lock()->ReadLobbyOptions();
    if (!opts) {
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    auto new_child = MakeMatchChildClient(ResolveRunnerExe(),
            GameLibraryPath(ctx_.bot, ctx_.game_handle), opts->runtime);
    if (!new_child) {
        reply() << "[错误] 建立失败：无法启动游戏子进程";
        return EC_MATCH_UNEXPECTED_CONFIG;
    }
    return data_.lock()->InstallGameChild(std::move(new_child), opts->applied_log, reply);
}

void Match::HandleGameTimeout_(const std::shared_ptr<std::atomic<bool>>& tio)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteTimeout(tio, state_, unbind);
    if (result.is_over) {
        Unbind_();
    }
}

void Match::Help_(HostMsgSenderBase& reply, const bool text_mode)
{
    data_.lock()->Help(reply, text_mode, [this, &reply, text_mode] {
        FetchHelp_(reply, text_mode);
    });
}

void Match::FetchHelp_(HostMsgSenderBase& reply, const bool text_mode)
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
