// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <cassert>
#include <concepts>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "utility/msg_checker.h"
#include "utility/lock_wrapper.h"

#include "bot_core/match/match_common.h"
#include "bot_core/msg_sender.h"
#include "bot_core/game_handle.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/match/match_child_client.h"
#include "bot_core/match/match_lobby.h"
#include "bot_core/match/match_running.h"

namespace lgtbot {
namespace core {
namespace match {

class Match : public std::enable_shared_from_this<Match>
{
  public:
    // Wraps a raw pointer to the game child process. Bound at most once, from within
    // data_.lock(), at the moment state transitions to IS_STARTED. After binding the
    // pointer stays valid for the lifetime of the enclosing Match: game_child_ inside
    // Internal is never destroyed prior to Match destruction. operator() is idempotent
    // and thread-safe; it forwards to MatchChildClient::Cancel which sends SIGTERM to
    // the child without waiting, unblocking any concurrent IPC held under data_.lock().
    class GameChildTerminator
    {
      public:
        GameChildTerminator() noexcept = default;

        GameChildTerminator(const GameChildTerminator&) = delete;
        GameChildTerminator& operator=(const GameChildTerminator&) = delete;

        void Bind(MatchChildClient* child) noexcept
        {
            child_.store(child, std::memory_order_release);
        }

        void operator()() const noexcept
        {
            if (auto* const c = child_.load(std::memory_order_acquire)) {
                c->Cancel();
            }
        }

      private:
        std::atomic<MatchChildClient*> child_{nullptr};
    };

    class Internal
    {
      public:
        Internal(Match& match,
                const UserID host_uid, MatchInitOptions init_options,
                std::unique_ptr<MatchChildClient> game_child)
            : match_(match)
            , users_([&match, host_uid]() {
                std::map<UserID, MatchParticipantUser> m;
                m.emplace(host_uid, MatchParticipantUser(host_uid, match.ctx_.bot.MakeMsgSender(host_uid)));
                return m;
            }())
            , game_child_(std::move(game_child))
            , phase_(std::in_place_type<MatchLobby>, match.ctx_, &match.messaging_, users_, host_uid, std::move(init_options))
        {}

        // Raw pointer to the eagerly-spawned child; used by Match to bind the terminator
        // during construction. The pointer is stable for the lifetime of Internal.
        MatchChildClient* GameChild() const { return game_child_.get(); }

        struct PhaseResult {
            ErrCode rc = EC_OK;
            bool is_over = false;
        };

        void BriefInfo(std::string& out) const;

        ErrCode SetBenchTo(const UserID uid, HostMsgSenderBase& reply,
                           const uint64_t bench_computers_to_player_num);
        ErrCode SetFormal(const UserID uid, HostMsgSenderBase& reply, const bool is_formal);
        ErrCode Join(const UserID uid, HostMsgSenderBase& reply);

        // Dispatches the request to MatchLobby or MatchRunning using the eagerly-spawned child.
        PhaseResult ExecuteRequest(const UserID uid, const std::optional<GroupID> gid,
                                   const std::string& msg, MsgSender& reply);

        // Merged game-start: validates host, prepares plan, sends Start IPC and either
        // commits MatchRunning or rolls back.
        PhaseResult ExecuteGameStart(const UserID uid, HostMsgSenderBase& reply);

        void ExecuteAlert(const uint64_t remaining_sec,
                          const std::shared_ptr<std::atomic<bool>>& tio);

        // Commits the MatchLobby -&gt; MatchRunning transition by reading MatchLobby's cached snapshot
        // (via its const accessors) and constructing MatchRunning in place. Idempotent:
        // no-op when phase_ is already MatchRunning. Called both (a) lazily by the
        // get_running callback on the first push frame arriving during SendStart,
        // and (b) explicitly by ExecuteGameStart when SendStart returned STAGE_OK
        // without producing any push frames.
        void EnsureRunningPhase();

        PhaseResult ExecuteLeave(const UserID uid, HostMsgSenderBase& reply, const bool force);

        PhaseResult ExecuteUserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel);

        PhaseResult ExecuteTerminate(const bool is_force);

        PhaseResult ExecuteTimeout(const std::shared_ptr<std::atomic<bool>>& tio);

        template <std::invocable<> F>
        void Help(HostMsgSenderBase& reply, bool text_mode, F&& lobby_help);

        template <typename F>
        decltype(auto) VisitPhase(F&& f)
        {
            return std::visit(std::forward<F>(f), phase_);
        }

        template <typename F>
        decltype(auto) VisitPhase(F&& f) const
        {
            return std::visit(std::forward<F>(f), phase_);
        }

        bool IsLobby() const { return std::holds_alternative<MatchLobby>(phase_); }

      private:
        void CleanupRunning_();

        Match& match_;
        std::map<UserID, MatchParticipantUser> users_;
        std::unique_ptr<MatchChildClient> game_child_;
        std::variant<MatchLobby, MatchRunning> phase_;
    };

    using VariantID = MatchVariantID;
    using State = MatchState;
    using InitOptions = MatchInitOptions;
    static constexpr State NOT_STARTED = MATCH_NOT_STARTED;
    static constexpr State IS_STARTED = MATCH_IS_STARTED;
    static constexpr State IS_OVER = MATCH_IS_OVER;
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, InitOptions init_options,
            const UserID host_uid, const std::optional<GroupID> gid,
            std::unique_ptr<MatchChildClient> game_child);
    ~Match();

    uint64_t MatchId() const { return ctx_.mid; }
    const char* GameName() const { return ctx_.game_handle.Info().name_; }

    HostMsgSenderBase& BoardcastMsgSender();
    HostMsgSenderBase& TellMsgSender(const PlayerID pid);
    HostMsgSenderBase& GroupMsgSender();

    const char* PlayerName(const PlayerID& pid);
    const char* PlayerAvatar(const PlayerID& pid, const int32_t size);

    ErrCode SetBenchTo(const UserID uid, HostMsgSenderBase& reply, const uint64_t bench_computers_to_player_num);
    ErrCode SetFormal(const UserID uid, HostMsgSenderBase& reply, const bool is_formal);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply);
    ErrCode GameStart(const UserID uid, HostMsgSenderBase& reply);
    ErrCode Join(const UserID uid, HostMsgSenderBase& reply);
    ErrCode Leave(const UserID uid, HostMsgSenderBase& reply, const bool force);
    ErrCode UserInterrupt(const UserID uid, HostMsgSenderBase& reply, const bool cancel);

    HostMsgSenderBase::MsgSenderGuard Boardcast() { return BoardcastMsgSender()(); }
    HostMsgSenderBase::MsgSenderGuard BoardcastAtAll();
    HostMsgSenderBase::MsgSenderGuard Tell(const PlayerID pid) { return TellMsgSender(pid)(); }

    void ShowInfo(HostMsgSenderBase& reply) const;

    bool SwitchHost();

    size_t UserNum() const;

    VariantID ConvertPid(const PlayerID pid) const;

    ErrCode Terminate(const bool is_force);

    const GameHandle& game_handle() const { return ctx_.game_handle; }
    std::optional<GroupID> gid() const { return ctx_.gid; }
    UserID HostUserId() const;
    State state() const { return state_.load(std::memory_order_acquire); }
    MatchManager& match_manager();
    bool IsPrivate() const { return !ctx_.gid.has_value(); }

    void BriefInfo(std::string& out) const;

    friend class MatchRunning;
    friend class MatchTimerFactory;

   private:
    auto MakeUnbindCallback_() { return [this](const UserID u) { match_manager().UnbindMatch(u); }; }
    void Unbind_();
    void Help_(HostMsgSenderBase& reply, const bool text_mode);
    void FetchHelp_(HostMsgSenderBase& reply, const bool text_mode);
    void HandleGameTimeout_(const std::shared_ptr<std::atomic<bool>>& tio);

    const Command<void(HostMsgSenderBase&)> help_cmd_{
        Command<void(HostMsgSenderBase&)>("查看游戏帮助", std::bind_front(&Match::Help_, this), VoidChecker("帮助"),
                OptionalDefaultChecker<BoolChecker>(false, "文字", "图片"))
    };

    MatchContext ctx_;
    MatchMessaging messaging_;
    MatchHelpServices help_;
    std::optional<MsgSender> group_sender_;
    mutable std::unique_ptr<HostMsgSenderBase> private_broadcast_scratch_;

    mutable mutex_protect_wrapper<Internal> data_;

    // Bound from within data_.lock(); read without holding data_. See class-level comment.
    GameChildTerminator game_child_terminator_;

    std::atomic<MatchState> state_{MATCH_NOT_STARTED};
};

} // namespace match
} // namespace core
} // namespace lgtbot
