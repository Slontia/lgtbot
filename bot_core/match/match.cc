// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/match/match.h"

#include <cassert>
#include <utility>

#include "utility/log.h"
#include "utility/overloaded.h"
#include "utility/msg_checker.h"
#include "bot_core/match/match_manager.h"
#include "bot_core/options.h"
#include "match_process/match_ipc.pb.h"
#include "nlohmann/json.hpp"

namespace lgtbot {
namespace core {
namespace match {

namespace {

MatchPhaseBase& PhaseCommon(std::variant<MatchLobby, MatchRunning>& phase)
{
    if (auto* r = std::get_if<MatchRunning>(&phase)) {
        return *r;
    }
    return std::get<MatchLobby>(phase);
}

const MatchPhaseBase& PhaseCommon(const std::variant<MatchLobby, MatchRunning>& phase)
{
    if (auto* r = std::get_if<MatchRunning>(&phase)) {
        return *r;
    }
    return std::get<MatchLobby>(phase);
}

} // namespace

void Match::Internal::BriefInfo(std::string& out) const
{
    PhaseCommon(phase_).BriefInfo(out);
}

ErrCode Match::Internal::SetBenchTo(UserID uid, HostMsgSenderBase& reply,
                               uint64_t bench_computers_to_player_num)
{
    auto* p = std::get_if<MatchLobby>(&phase_);
    if (!p) {
        reply() << "[错误] 设置失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->SetBenchTo(uid, reply, bench_computers_to_player_num);
}

ErrCode Match::Internal::SetFormal(UserID uid, HostMsgSenderBase& reply, bool is_formal)
{
    auto* p = std::get_if<MatchLobby>(&phase_);
    if (!p) {
        reply() << "[错误] 设置失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->SetFormal(uid, reply, is_formal);
}

ErrCode Match::Internal::Join(UserID uid, HostMsgSenderBase& reply)
{
    auto* p = std::get_if<MatchLobby>(&phase_);
    if (!p) {
        reply() << "[错误] 加入失败：游戏已经开始";
        return EC_MATCH_ALREADY_BEGIN;
    }
    return p->Join(uid, reply);
}

template <std::invocable<UserID> Unbind>
Match::Internal::PhaseResult Match::Internal::ExecuteRequest(
    UserID uid, std::optional<GroupID> gid,
    const std::string& msg, MsgSender& reply,
    std::atomic<MatchState>& state,
    Match& match, Unbind&& unbind_user)
{
    {
        // Route the in-game help command before phase dispatch. The callback must not
        // re-acquire data_ (we already hold it here), so dispatch via Internal::Help
        // directly instead of Match::Help_ which locks again.
        MsgReader reader(msg);
        Command<void(HostMsgSenderBase&)> help_cmd("查看游戏帮助",
                [this, &match](HostMsgSenderBase& reply, const bool text_mode) {
                    Help(reply, text_mode, [&match, &reply, text_mode] { match.FetchHelp_(reply, text_mode); });
                },
                VoidChecker("帮助"), OptionalDefaultChecker<BoolChecker>(false, "文字", "图片"));
        if (help_cmd.CallIfValid(reader, reply)) {
            return {EC_GAME_REQUEST_OK, false};
        }
    }
    if (auto* const lobby = std::get_if<MatchLobby>(&phase_)) {
        return {lobby->Request(uid, gid, msg, reply, *game_child_), false};
    }
    auto& running = std::get<MatchRunning>(phase_);
    const auto rc = running.ExecuteRequest(uid, gid, msg, reply, MakeCallback(running));
    if (running.is_over()) {
        CleanupRunning(state, std::forward<Unbind>(unbind_user));
        return {rc, true};
    }
    return {rc, false};
}

template <std::invocable<UserID> Unbind>
Match::Internal::PhaseResult Match::Internal::ExecuteGameStart(
    const UserID uid, HostMsgSenderBase& reply,
    std::atomic<MatchState>& state,
    const MatchContext& ctx, MatchMessaging* messaging,
    MatchHelpServices* help,
    std::optional<MsgSender> group_sender,
    Match& match, Unbind&& unbind_user)
{
    auto* const lobby = std::get_if<MatchLobby>(&phase_);
    if (!lobby) {
        reply() << "[错误] 开始失败：游戏已经开始";
        return {EC_MATCH_ALREADY_BEGIN, false};
    }

    if (const auto rc = lobby->CheckHost(uid, reply); rc != EC_OK) {
        return {rc, false};
    }

    // Snapshot the IPC-facing player list before we touch phase_: the transition
    // committed by EnsureRunningPhase destroys the MatchLobby (and thus
    // its cache), but the broadcast at the bottom of this function still needs the
    // list. `players_for_child` is a copy; `MatchLobby::PlayerIds()` and RuntimeOptions
    // will be read again inside EnsureRunningPhase — cheap, and keeps this function
    // free of the moved-in plan bookkeeping the old ensure_running lambda required.
    auto players_for_child = lobby->PlayersForChild();
    const auto user_num = static_cast<uint32_t>(users_.size());
    const auto& generic = lobby->RuntimeOptions().generic_options_;
    const auto bench = generic.bench_computers_to_player_num_;
    const auto is_formal = generic.is_formal_;

    const auto get_running = [this, &match]() -> MatchRunning& {
        EnsureRunningPhase(match);
        return std::get<MatchRunning>(phase_);
    };
    const auto stage = game_child_->SendStart(ctx.mid.Get(), user_num,
                                              players_for_child, bench, is_formal,
                                              MakeCallback(get_running));

    if (!stage) {
        // Child died mid-Start. If MatchRunning was committed by a push frame, run
        // cleanup; otherwise the match dies in MatchLobby. The child pointer stays valid
        // (never reset) so the terminator bound at construction remains sound.
        if (MatchRunning* const running = std::get_if<MatchRunning>(&phase_); running && !running->is_over()) {
            running->HandleChildEof();
        }
        if (std::holds_alternative<MatchRunning>(phase_)) {
            CleanupRunning(state, std::forward<Unbind>(unbind_user));
            return {EC_UNEXPECTED_ERROR, true};
        }
        reply() << "[错误] 开始失败：游戏已被中断";
        return {EC_MATCH_ALREADY_BEGIN, false};
    }

    if (!std::holds_alternative<MatchRunning>(phase_) && *stage != lgtbot::ipc::ResultResp::STAGE_OK) {
        // Child rejected the Start (e.g., AdaptOptions returned false). No push
        // frames arrived, so phase_ is still MatchLobby. main_stage_ stays null child-side,
        // so a later SetOption + retry Start on the same child is valid.
        reply() << "[错误] 开始失败：不符合游戏参数的预期";
        return {EC_MATCH_UNEXPECTED_CONFIG, false};
    }

    // STAGE_OK: commit to MatchRunning if push frames did not already do so.
    EnsureRunningPhase(match);
    auto& running = std::get<MatchRunning>(phase_);
    if (running.is_over()) {
        CleanupRunning(state, std::forward<Unbind>(unbind_user));
        return {EC_OK, true};
    }
    running.BoardcastAtAll()
        << "游戏开始，您可以使用「帮助」命令（不带" META_COMMAND_SIGN "号），查看可执行命令";
    nlohmann::json players_json_array = nlohmann::json::array();
    for (const auto& pi : players_for_child) {
        players_json_array.push_back(pi.computer() ? nlohmann::json{{"computer_id", pi.computer_id()}}
                                                   : nlohmann::json{{"display_name", pi.display_name()}});
    }
    running.Boardcast()
        << nlohmann::json{{"match_id", ctx.mid.Get()},
                          {"state", "started"},
                          {"players", std::move(players_json_array)}}.dump();
    return {EC_OK, false};
}

void Match::Internal::ExecuteAlert(uint64_t remaining_sec,
                              const std::shared_ptr<std::atomic<bool>>& tio, Match& match)
{
    if (tio->load(std::memory_order_acquire)) {
        return;
    }
    auto& running = std::get<MatchRunning>(phase_);
    if (running.is_over()) {
        return;
    }
    (void)game_child_->SendAlert(remaining_sec, MakeCallback(running));
}

void Match::Internal::EnsureRunningPhase(Match& match)
{
    auto* const lobby = std::get_if<MatchLobby>(&phase_);
    if (!lobby) {
        return;   // already MatchRunning
    }
    // Snapshot MatchLobby state into locals BEFORE emplace. `variant::emplace` destroys
    // the currently held alternative BEFORE constructing the new one, so any
    // reference-returning accessor on `*lobby` passed as an argument (RuntimeOptions,
    // PlayerIds, ...) would be a dangling reference by the time MatchRunning's ctor
    // copies from it.
    const UserID host_uid = lobby->HostUserId();
    MatchRuntimeOptions options = lobby->RuntimeOptions();       // copy
    std::vector<MatchVariantID> player_ids = lobby->PlayerIds(); // copy

    phase_.emplace<MatchRunning>(match.ctx_, &match.messaging_, &match.help_, users_,
                            host_uid, std::move(options), std::move(player_ids),
                            game_child_.get(),
                            std::move(match.group_sender_),
                            MatchTimerFactory(match));
    match.state_.store(MATCH_IS_STARTED, std::memory_order_release);
}

template <std::invocable<UserID> F>
void Match::Internal::CleanupRunning(std::atomic<MatchState>& state, F&& unbind_user)
{
    state.store(MATCH_IS_OVER, std::memory_order_release);
    for (auto& [uid, user] : users_) {
        if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
            unbind_user(uid);
        }
    }
    if (game_child_) {
        game_child_->Cancel();
    }
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteLeave(
    UserID uid, HostMsgSenderBase& reply, bool force,
    std::atomic<MatchState>& state, Match& match, F&& unbind_user)
{
    if (auto* running = std::get_if<MatchRunning>(&phase_)) {
        const auto rc = running->LeaveBeforeChild(uid, reply, force, MakeCallback(*running));
        if (rc != EC_OK) {
            return {rc, false};
        }
        unbind_user(uid);
        if (running->is_over()) {
            CleanupRunning(state, std::forward<F>(unbind_user));
            return {EC_OK, true};
        }
        return {EC_OK, false};
    }
    const auto rc = std::get<MatchLobby>(phase_).Leave(uid, reply, force);
    if (rc != EC_OK) {
        return {rc, false};
    }
    unbind_user(uid);
    if (users_.empty()) {
        return {EC_OK, true};
    }
    return {EC_OK, false};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteUserInterrupt(
    UserID uid, HostMsgSenderBase& reply, bool cancel,
    std::atomic<MatchState>& state, F&& unbind_user)
{
    if (auto* running = std::get_if<MatchRunning>(&phase_)) {
        const auto rc = running->UserInterrupt(uid, reply, cancel);
        if (running->is_over()) {
            CleanupRunning(state, std::forward<F>(unbind_user));
            return {rc, true};
        }
        return {rc, false};
    }
    const auto rc = std::get<MatchLobby>(phase_).UserInterrupt(uid, reply, cancel);
    return {rc, false};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteTerminate(
    bool is_force, std::atomic<MatchState>& state, F&& unbind_user)
{
    if (auto* running = std::get_if<MatchRunning>(&phase_)) {
        const auto rc = running->Terminate(is_force);
        if (running->is_over()) {
            CleanupRunning(state, std::forward<F>(unbind_user));
            return {rc, true};
        }
        return {rc, false};
    }
    const auto rc = std::get<MatchLobby>(phase_).Terminate(is_force);
    for (auto& [uid, user] : users_) {
        if (user.presence_.load(std::memory_order_acquire) != UserPresence::LEFT) {
            unbind_user(uid);
        }
    }
    return {rc, true};
}

template <std::invocable<UserID> F>
Match::Internal::PhaseResult Match::Internal::ExecuteTimeout(
    const std::shared_ptr<std::atomic<bool>>& tio,
    std::atomic<MatchState>& state, Match& match, F&& unbind_user)
{
    if (tio->load(std::memory_order_acquire)) {
        return {EC_OK, false};
    }
    auto& running = std::get<MatchRunning>(phase_);
    if (running.is_over()) {
        return {EC_OK, false};
    }
    (void)game_child_->SendTimeout(MakeCallback(running));
    if (running.is_over()) {
        CleanupRunning(state, std::forward<F>(unbind_user));
        return {EC_OK, true};
    }
    return {EC_OK, false};
}

template <std::invocable<> F>
void Match::Internal::Help(HostMsgSenderBase& reply, bool text_mode, F&& lobby_help)
{
    if (auto* r = std::get_if<MatchRunning>(&phase_)) {
        r->FetchHelp(reply, text_mode);
        return;
    }
    std::forward<F>(lobby_help)();
}

Match::Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, InitOptions init_options,
        const UserID host_uid, const std::optional<GroupID> gid,
        std::unique_ptr<MatchChildClient> game_child)
    : ctx_{bot, mid, game_handle, gid}
    , group_sender_{}
    , data_{ctx_, &messaging_, host_uid, std::move(init_options), std::move(game_child)}
{
    // The child is a Match invariant now: spawned by match_manager before Match
    // creation, kept alive until ~Match. Binding the terminator once here means Cancel
    // (thus lock-free interrupt of any IPC held under data_.lock()) is available for
    // the entire Match lifetime — not just after IS_STARTED.
    game_child_terminator_.Bind(data_.lock()->GameChild());

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
    auto locked = data_.lock();
    return locked->VisitPhase([&](auto& p) -> decltype(auto) {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, MatchRunning>) {
            return p.TellMsgSender(pid);
        } else {
            // PlayerID is only meaningful in MatchRunning; before game start there is no
            // per-player addressing, so callers that reach here get a silent no-op.
            return static_cast<HostMsgSenderBase&>(HostEmptyMsgSender::Get());
        }
    });
}

HostMsgSenderBase& Match::GroupMsgSender()
{
    return data_.lock()->VisitPhase([](auto& p) -> decltype(auto) { return p.GroupMsgSender(); });
}

const char* Match::PlayerName(const PlayerID& pid)
{
    auto locked = data_.lock();
    return locked->VisitPhase([&](auto& p) -> const char* {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, MatchRunning>) {
            return p.PlayerName(pid);
        } else {
            return "";
        }
    });
}

const char* Match::PlayerAvatar(const PlayerID& pid, const int32_t size)
{
    auto locked = data_.lock();
    return locked->VisitPhase([&](auto& p) -> const char* {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, MatchRunning>) {
            return p.PlayerAvatar(pid, size);
        } else {
            return "";
        }
    });
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
    auto locked = data_.lock();
    return locked->VisitPhase([&](const auto& p) -> Match::VariantID {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, MatchRunning>) {
            return p.ConvertPid(pid);
        } else {
            return p.HostUserId();
        }
    });
}

void Match::BriefInfo(std::string& out) const
{
    data_.lock()->BriefInfo(out);
}

ErrCode Match::SetBenchTo(const UserID uid, HostMsgSenderBase& reply,
                           const uint64_t bench_computers_to_player_num)
{
    return data_.lock()->SetBenchTo(uid, reply, bench_computers_to_player_num);
}

ErrCode Match::SetFormal(const UserID uid, HostMsgSenderBase& reply, const bool is_formal)
{
    return data_.lock()->SetFormal(uid, reply, is_formal);
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
    const auto rc = data_.lock()->Join(uid, reply);
    if (rc != EC_OK) {
        match_manager().UnbindMatch(uid);
    }
    return rc;
}

ErrCode Match::Request(const UserID uid, const std::optional<GroupID> gid,
                        const std::string& msg, MsgSender& reply)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteRequest(uid, gid, msg, reply, state_, *this, unbind);
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
    auto result = data_.lock()->ExecuteLeave(uid, reply, force, state_, *this, unbind);
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
    data_.lock()->VisitPhase([&](auto& p) { p.ShowInfo(reply); });
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
    // Phase 1 (no lock): interrupt any in-flight IPC held under data_.lock() so the
    // holding thread exits its critical section and we can proceed to phase 2. If
    // terminator is unbound (state < IS_STARTED) this is a no-op; short lobby IPC
    // paths release the lock quickly on their own.
    game_child_terminator_();

    // Phase 2 (single data_.lock()): tear down state and detach users. Cleanup is
    // idempotent — if another thread's error path already ran CleanupRunning,
    // ExecuteTerminate observes MATCH_IS_OVER / no MatchRunning and returns EC_OK.
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteTerminate(is_force, state_, unbind);
    if (result.is_over) {
        Unbind_();
    }
    return result.rc;
}

ErrCode Match::GameStart(const UserID uid, HostMsgSenderBase& reply)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteGameStart(
                uid, reply, state_,
                ctx_, &messaging_, &help_, std::move(group_sender_), *this, unbind);
    if (result.is_over) {
        Unbind_();
        if (result.rc != EC_OK) {
            reply() << "[错误] 开始失败：非预期的错误";
        }
    }
    return result.rc;
}

void Match::HandleGameTimeout_(const std::shared_ptr<std::atomic<bool>>& tio)
{
    auto unbind = [this](UserID u) { match_manager().UnbindMatch(u); };
    auto result = data_.lock()->ExecuteTimeout(tio, state_, *this, unbind);
    if (result.is_over) {
        // Route mid/gid unbind through MatchManager's worker thread. This handler
        // runs on Timer::thread_ (the loop thread under the new sync-handle design),
        // so a synchronous Unbind_() here that dropped MatchManager's last
        // shared_ptr would cascade into ~Timer.join(current thread) — self deadlock.
        match_manager().ScheduleUnbind(shared_from_this());
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

} // namespace match
} // namespace core
} // namespace lgtbot
