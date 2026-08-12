// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <cassert>
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

struct MatchData
{
    std::map<UserID, MatchParticipantUser> users;
    std::vector<MatchPlayer> players;
    std::unique_ptr<MatchChildClient> game_child;
    std::variant<Lobby, Running> phase;

    MatchData(const MatchContext& ctx, MatchMessaging* messaging,
            const UserID host_uid, MatchInitOptions init_options)
        : users([&ctx, host_uid]() {
            std::map<UserID, MatchParticipantUser> m;
            m.emplace(host_uid, MatchParticipantUser(host_uid, ctx.bot.MakeMsgSender(host_uid)));
            return m;
        }())
        , phase(std::in_place_type<Lobby>, ctx, messaging, users, players, host_uid, std::move(init_options))
    {}
};

class Match : public std::enable_shared_from_this<Match>
{
  public:
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

    MsgSenderBase& BoardcastMsgSender();
    MsgSenderBase& TellMsgSender(const PlayerID pid);
    MsgSenderBase& GroupMsgSender();

    const char* PlayerName(const PlayerID& pid);
    const char* PlayerAvatar(const PlayerID& pid, const int32_t size);

    ErrCode SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num);
    ErrCode SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply);
    ErrCode GameStart(const UserID uid, MsgSenderBase& reply);
    ErrCode Join(const UserID uid, MsgSenderBase& reply);
    ErrCode Leave(const UserID uid, MsgSenderBase& reply, const bool force);
    ErrCode UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel);

    MsgSenderBase::MsgSenderGuard Boardcast() { return BoardcastMsgSender()(); }
    MsgSenderBase::MsgSenderGuard BoardcastAtAll();
    MsgSenderBase::MsgSenderGuard Tell(const PlayerID pid) { return TellMsgSender(pid)(); }

    void ShowInfo(MsgSenderBase& reply) const;

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

    void BindMsgSenderMatch_();

   private:
    void Unbind_();
    void Help_(MsgSenderBase& reply, const bool text_mode);
    void FetchHelp_(MsgSenderBase& reply, const bool text_mode);
    ErrCode EnsureLobbyChild_(MsgSenderBase& reply);
    void CommitRunning_(LobbyStartSnapshot snapshot);
    [[nodiscard]] bool LobbyStartAborted_();
    void RollbackLobbyStart_(MatchData& data);
    void RollbackLobbyStart_();
    void CleanupRunning_(MatchData& data);
    void CleanupRunningUsers_(MatchData& data);
    void ReleaseGameChild_(MatchData& data);

    const Command<void(MsgSenderBase&)> help_cmd_{
        Command<void(MsgSenderBase&)>("查看游戏帮助", std::bind_front(&Match::Help_, this), VoidChecker("帮助"),
                OptionalDefaultChecker<BoolChecker>(false, "文字", "图片"))
    };

    MatchContext ctx_;
    MatchMessaging messaging_;
    MatchHelpServices help_;
    std::optional<MsgSender> group_sender_;
    mutable std::unique_ptr<MsgSenderBase> private_broadcast_scratch_;

    mutable mutex_protect_wrapper<MatchData, MatchPhaseMutex> data_;

    std::atomic<MatchState> state_{MATCH_NOT_STARTED};
};
