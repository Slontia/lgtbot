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

#include "bot_core/match_env.h"
#include "bot_core/match_phase_guard.h"
#include "bot_core/match_types.h"
#include "bot_core/msg_sender.h"
#include "bot_core/game_handle.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/match_child_client.h"
#include "bot_core/match_lobby.h"
#include "bot_core/match_running.h"

class Match : public std::enable_shared_from_this<Match>
{
  public:
    class Internal
    {
      public:
        Internal(const MatchContext& ctx, MatchMessaging* messaging,
                const UserID host_uid, MatchInitOptions init_options)
            : users_([&ctx, host_uid]() {
                std::map<UserID, MatchParticipantUser> m;
                m.emplace(host_uid, MatchParticipantUser(host_uid, ctx.bot.MakeMsgSender(host_uid)));
                return m;
            }())
            , phase_(std::in_place_type<Lobby>, ctx, messaging, users_, players_, host_uid, std::move(init_options))
        {}

        struct PhaseResult {
            ErrCode rc = EC_OK;
            bool is_over = false;
            std::unique_ptr<MatchChildClient> child;
        };

        struct LobbyOptions {
            std::vector<std::string> applied_log;
            MatchChildClient::RuntimeOptions runtime;
        };

        bool HasGameChild() const { return game_child_ != nullptr; }

        void BriefInfo(std::string& out) const;

        ErrCode SetBenchTo(UserID uid, HostMsgSenderBase& reply,
                           uint64_t bench_computers_to_player_num, bool is_starting);
        ErrCode SetFormal(UserID uid, HostMsgSenderBase& reply,
                          bool is_formal, bool is_starting);
        ErrCode Join(UserID uid, HostMsgSenderBase& reply, bool is_starting);

        ErrCode ExecuteLobbyRequest(UserID uid, std::optional<GroupID> gid,
                                    const std::string& msg, MsgSender& reply,
                                    std::weak_ptr<class Match> self);

        std::optional<LobbyOptions> ReadLobbyOptions() const;
        ErrCode InstallGameChild(std::unique_ptr<MatchChildClient> child,
                                 const std::vector<std::string>& applied_log,
                                 HostMsgSenderBase& reply);

        std::optional<LobbyGameStartPlan> BeginGameStart(UserID uid, HostMsgSenderBase& reply,
                                                         std::weak_ptr<const class Match> self,
                                                         const std::atomic<MatchState>& state,
                                                         ErrCode& err_out);
        std::optional<LobbyRunningHandoff> TryExtractRunningHandoff(
            std::atomic<MatchState>& state,
            std::unique_ptr<MatchChildClient>& aborted_child_out);
        void CommitRunning(LobbyStartSnapshot snapshot,
                           std::optional<MsgSender> group_sender,
                           const MatchContext& ctx, MatchMessaging* messaging,
                           MatchHelpServices* help,
                           std::weak_ptr<class Match> self,
                           std::atomic<MatchState>& state);
        PhaseResult ExecuteSendStart(MatchID mid, uint32_t user_num,
                                     const std::vector<lgtbot::ipc::PlayerInfo>& players);
        void BroadcastGameStartedImpl_(MatchID mid,
                                       const std::vector<lgtbot::ipc::PlayerInfo>& players);
        std::unique_ptr<MatchChildClient> RollbackLobbyStart(std::atomic<MatchState>& state);

        void ExecuteAlert(uint64_t remaining_sec,
                          const std::shared_ptr<std::atomic<bool>>& tio);

        template <std::invocable<UserID> F>
        std::unique_ptr<MatchChildClient> CleanupRunning(std::atomic<MatchState>& state,
                                                          F&& unbind_user);

        template <std::invocable<UserID> F>
        PhaseResult ExecuteRunningRequest(UserID uid, std::optional<GroupID> gid,
                                          const std::string& msg, MsgSender& reply,
                                          std::weak_ptr<const class Match> self,
                                          std::atomic<MatchState>& state, F&& unbind_user);

        template <std::invocable<UserID> F>
        PhaseResult ExecuteLeave(UserID uid, HostMsgSenderBase& reply, bool force,
                                 std::atomic<MatchState>& state, F&& unbind_user);

        template <std::invocable<UserID> F>
        PhaseResult ExecuteUserInterrupt(UserID uid, HostMsgSenderBase& reply, bool cancel,
                                         std::atomic<MatchState>& state, F&& unbind_user);

        template <std::invocable<UserID> F>
        PhaseResult ExecuteTerminate(bool is_force, std::atomic<MatchState>& state,
                                     F&& unbind_user);

        template <std::invocable<UserID> F>
        PhaseResult BroadcastGameStarted(MatchID mid,
                                         const std::vector<lgtbot::ipc::PlayerInfo>& players,
                                         std::atomic<MatchState>& state, F&& unbind_user);

        template <std::invocable<UserID> F>
        PhaseResult ExecuteTimeout(const std::shared_ptr<std::atomic<bool>>& tio,
                                   std::atomic<MatchState>& state, F&& unbind_user);

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

        bool IsLobby() const { return std::holds_alternative<Lobby>(phase_); }

      private:
        std::map<UserID, MatchParticipantUser> users_;
        std::vector<MatchPlayer> players_;
        std::unique_ptr<MatchChildClient> game_child_;
        std::variant<Lobby, Running> phase_;
    };

    using VariantID = MatchVariantID;
    using State = MatchState;
    using InitOptions = MatchInitOptions;
    static constexpr State NOT_STARTED = MATCH_NOT_STARTED;
    static constexpr State IS_STARTING = MATCH_IS_STARTING;
    static constexpr State IS_STARTED = MATCH_IS_STARTED;
    static constexpr State IS_OVER = MATCH_IS_OVER;
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID mid, GameHandle& game_handle, InitOptions init_options,
            const UserID host_uid, const std::optional<GroupID> gid);
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

    friend class Running;

   private:
    void Unbind_();
    void Help_(HostMsgSenderBase& reply, const bool text_mode);
    void FetchHelp_(HostMsgSenderBase& reply, const bool text_mode);
    ErrCode EnsureLobbyChild_(HostMsgSenderBase& reply);
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

    mutable mutex_protect_wrapper<Internal, MatchPhaseMutex> data_;

    std::atomic<MatchState> state_{MATCH_NOT_STARTED};
};
