// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cassert>

#include <map>
#include <set>
#include <bitset>
#include <variant>

#include "utility/msg_checker.h"

#include "bot_core/match_base.h"
#include "bot_core/msg_sender.h"
#include "bot_core/timer.h"
#include "bot_core/game_handle.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/db_manager.h"

#define INVALID_MATCH (MatchID)0

inline bool match_is_valid(MatchID id) { return id != INVALID_MATCH; }

typedef enum { PRIVATE_MATCH, GROUP_MATCH, DISCUSS_MATCH } MatchType;

class GameBase;
class Match;
class PrivateMatch;
class GroupMatch;
class DiscussMatch;
class MatchManager;
class GameHandle;

template <typename ...Ts>
class Overload : public Ts...
{
  public:
    Overload(Ts&& ...ts) : Ts(std::forward<Ts>(ts))... {}
    using Ts::operator()...;
};

struct ParticipantUser
{
    enum class State { ACTIVE, LEFT };
    explicit ParticipantUser(const UserID uid, const bool is_ai)
        : uid_(uid)
        , is_ai_(is_ai)
        , sender_(uid)
        , state_(State::ACTIVE)
        , leave_when_config_changed_(true)
        , want_interrupt_(false)
    {}
    UserID uid_;
    bool is_ai_;
    std::vector<PlayerID> pids_;
    MsgSender sender_;
    State state_;
    bool leave_when_config_changed_;
    bool want_interrupt_;
};

class Match : public MatchBase, public std::enable_shared_from_this<Match>
{
  public:
    using VariantID = std::variant<UserID, ComputerID>;
    enum State { NOT_STARTED = 'N', IS_STARTED = 'S', IS_OVER = 'O' };
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID id, GameHandle& game_handle, const UserID host_uid,
          const std::optional<GroupID> gid);
    ~Match();

    virtual MsgSenderBase& BoardcastMsgSender() override;
    virtual MsgSenderBase& BoardcastAiInfoMsgSender() override;
    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override;
    virtual MsgSenderBase& GroupMsgSender() override;

    virtual const char* PlayerName(const PlayerID& pid) override;
    virtual const char* PlayerAvatar(const PlayerID& pid, const int32_t size) override;

    virtual void StartTimer(const uint64_t sec, void* p, void(*cb)(void*, uint64_t)) override;
    virtual void StopTimer() override;

    virtual void Eliminate(const PlayerID pid) override;
    virtual void Hook(const PlayerID pid) override;
    virtual void Activate(const PlayerID pid) override;

    virtual bool IsInDeduction() const override { return is_in_deduction_; }
    virtual uint64_t MatchId() const override { return mid_; }
    virtual const char* GameName() const override { return game_handle_.name_.c_str(); }

    ErrCode SetBenchTo(const UserID uid, MsgSenderBase& reply, const std::optional<uint64_t> com_num);
    ErrCode SetMultiple(const UserID uid, MsgSenderBase& reply, const uint32_t multiple);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply);
    ErrCode GameConfigOver(MsgSenderBase& reply);
    ErrCode GameStart(const UserID uid, const bool is_public, MsgSenderBase& reply);
    ErrCode Join(const UserID uid, MsgSenderBase& reply);
    ErrCode Leave(const UserID uid, MsgSenderBase& reply, const bool force);
    ErrCode LeaveMidway(const UserID uid, const bool is_public);
    ErrCode UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel);

    MsgSenderBase::MsgSenderGuard Boardcast() { return BoardcastMsgSender()(); }
    MsgSenderBase::MsgSenderGuard BoardcastAtAll();
    MsgSenderBase::MsgSenderGuard Tell(const PlayerID pid) { return TellMsgSender(pid)(); }
    MsgSenderBase::MsgSenderGuard BoardcastAiInfo() { return BoardcastAiInfoMsgSender()(); }

    void ShowInfo(MsgSenderBase& reply) const;

    bool SwitchHost();

    bool IsPrivate() const { return !gid_.has_value(); }
    auto PlayerNum() const { return players_.size(); }

    VariantID ConvertPid(const PlayerID pid) const;

    ErrCode Terminate(const bool is_force);

    const GameHandle& game_handle() const { return game_handle_; }
    std::optional<GroupID> gid() const { return gid_; }
    UserID host_uid() const { return host_uid_; }
    const State state() const { return state_; }
    MatchManager& match_manager() { return bot_.match_manager(); }

    const uint64_t user_controlled_player_num() const { return users_.size() * player_num_each_user_; }

    std::string BriefInfo() const;

   private:
    ErrCode CheckMultipleAllowed_(const UserID uid, MsgSenderBase& reply, const uint32_t multiple) const;

    template <typename Logger>
    Logger& MatchLog(Logger&& logger) const
    {
        logger << "[mid=" << MatchId() << "] ";
        if (gid_.has_value()) {
            logger << "[gid=" << *gid_ << "] ";
        } else {
            logger << "[no gid] ";
        }
        logger << "[game=" << GameName() << "] [host_uid=" << host_uid_ << "] ";
        return logger;
    }

    void OnGameOver_();
    void Help_(MsgSenderBase& reply, const bool text_mode);
    void Routine_();
    std::string OptionInfo_() const;
    void KickForConfigChange_();
    void Unbind_();
    void Terminate_();
    template <typename Fn>
    bool AllControlledPlayerState_(const ParticipantUser& user, Fn&& fn) const;
    bool Has_(const UserID uid) const;
    const char* HostUserName_() const;
    uint64_t ComputerNum_() const;
    void EmplaceUser_(const UserID uid);

    mutable std::mutex mutex_;

    // bot
    BotCtx& bot_;

    // basic info
    const MatchID mid_;
    GameHandle& game_handle_;
    UserID host_uid_;
    const std::optional<GroupID> gid_;
    std::atomic<State> state_;

    // time info
    std::shared_ptr<bool> timer_is_over_; // must before match because atom stage will call StopTimer
    std::unique_ptr<Timer> timer_;

    // game
    GameHandle::game_options_ptr options_;
    GameHandle::main_stage_ptr main_stage_;

    // user info
    std::map<UserID, ParticipantUser> users_;

    // message senders
    class MsgSenderBatchHandler
    {
      public:
        MsgSenderBatchHandler(Match& match, const bool ai_only) : match_(match) {};

        template <typename Fn>
        void operator()(Fn&& fn)
        {
            for (auto& [_, user_info] : match_.users_) {
                if (user_info.state_ != ParticipantUser::State::LEFT && (!ai_only_ || user_info.is_ai_)) {
                    fn(user_info.sender_);
                }
            }
        }

      private:
        Match& match_;
        bool ai_only_;
    };
    MsgSenderBatch<MsgSenderBatchHandler> boardcast_private_sender_;
    MsgSenderBatch<MsgSenderBatchHandler> boardcast_ai_info_private_sender_;
    std::optional<MsgSender> group_sender_;

    // other options
    uint64_t bench_to_player_num_;
    uint64_t player_num_each_user_;
    uint16_t multiple_;

    // player info (fill when game ready to start)
    struct Player
    {
        enum class State { ACTIVE, ELIMINATED, HOOKED };
        Player(const VariantID& id) : id_(id), state_(State::ACTIVE) {}
        VariantID id_;
        State state_;
    };
    std::vector<Player> players_; // all players, include computers

    const Command<void(MsgSenderBase&)> help_cmd_;

#ifdef TEST_BOT
  public:
    std::mutex before_handle_timeout_mutex_;
    std::condition_variable before_handle_timeout_cv_;
    bool before_handle_timeout_;
#endif

    bool is_in_deduction_;
};
