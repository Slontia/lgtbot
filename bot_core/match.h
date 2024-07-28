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

template <typename ...Ts>
class Overload : public Ts...
{
  public:
    Overload(Ts&& ...ts) : Ts(std::forward<Ts>(ts))... {}
    using Ts::operator()...;
};

class Match : public MatchBase, public std::enable_shared_from_this<Match>
{
  public:
    using VariantID = std::variant<UserID, ComputerID>;
    enum State { NOT_STARTED = 'N', IS_STARTED = 'S', IS_OVER = 'O' };
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID id, GameHandle& game_handle, GameHandle::Options options,
            const UserID host_uid, const std::optional<GroupID> gid);
    ~Match() = default;

    virtual MsgSenderBase& BoardcastMsgSender() override;
    virtual MsgSenderBase& BoardcastAiInfoMsgSender() override;
    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override;
    virtual MsgSenderBase& GroupMsgSender() override;

    virtual const char* PlayerName(const PlayerID& pid) override;
    virtual const char* PlayerAvatar(const PlayerID& pid, const int32_t size) override;

    virtual void StartTimer(const uint64_t sec, void* alert_arg, void(*alert_cb)(void*, uint64_t)) override;
    virtual void StopTimer() override;

    virtual void Eliminate(const PlayerID pid) override;
    virtual void Hook(const PlayerID pid) override;
    virtual void Activate(const PlayerID pid) override;

    virtual bool IsInDeduction() const override { return is_in_deduction_; }
    virtual uint64_t MatchId() const override { return mid_; }
    virtual const char* GameName() const override { return game_handle_.Info().name_.c_str(); }

    ErrCode SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t bench_computers_to_player_num);
    ErrCode SetFormal(const UserID uid, MsgSenderBase& reply, const bool is_formal);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply);
    ErrCode GameConfigOver(MsgSenderBase& reply);
    ErrCode GameStart(const UserID uid, MsgSenderBase& reply);
    ErrCode Join(const UserID uid, MsgSenderBase& reply);
    ErrCode Leave(const UserID uid, MsgSenderBase& reply, const bool force);
    ErrCode UserInterrupt(const UserID uid, MsgSenderBase& reply, const bool cancel);

    MsgSenderBase::MsgSenderGuard Boardcast() { return BoardcastMsgSender()(); }
    MsgSenderBase::MsgSenderGuard BoardcastAtAll();
    MsgSenderBase::MsgSenderGuard Tell(const PlayerID pid) { return TellMsgSender(pid)(); }
    MsgSenderBase::MsgSenderGuard BoardcastAiInfo() { return BoardcastAiInfoMsgSender()(); }

    void ShowInfo(MsgSenderBase& reply) const;

    bool SwitchHost();

    bool IsPrivate() const { return !gid_.has_value(); }
    auto UserNum() const { return std::lock_guard(mutex_), users_.size(); }

    VariantID ConvertPid(const PlayerID pid) const;

    ErrCode Terminate(const bool is_force);

    const GameHandle& game_handle() const { return game_handle_; }
    std::optional<GroupID> gid() const { return gid_; }
    UserID HostUserId() const { return std::lock_guard(mutex_), host_uid_; }
    const State state() const { return state_; }
    MatchManager& match_manager() { return bot_.match_manager(); }

    std::string BriefInfo() const;

   private:
    struct ParticipantUser
    {
        enum class State { ACTIVE, LEFT };
        explicit ParticipantUser(Match& match, const UserID uid, const bool is_ai)
            : uid_(uid)
            , is_ai_(is_ai)
            , sender_(match.bot_.MakeMsgSender(uid, &match))
        {}
        UserID uid_{""};
        bool is_ai_{false};
        PlayerID pid_{UINT32_MAX};
        MsgSender sender_;
        State state_{State::ACTIVE};
        bool leave_when_config_changed_{true};
        bool want_interrupt_{false};
    };

    uint32_t MaxPlayerNum_() const { return game_handle_.Info().max_player_num_fn_(options_.game_options_.get()); }
    uint32_t Multiple_() const { return game_handle_.Info().multiple_fn_(options_.game_options_.get()); }

    template <typename Logger>
    Logger& MatchLog_(Logger&& logger) const
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

    std::string BriefInfo_() const;
    void OnGameOver_();
    void Help_(MsgSenderBase& reply, const bool text_mode);
    void Routine_();
    std::string OptionInfo_() const;
    void KickForConfigChange_();
    void Unbind_();
    void Terminate_();
    bool Has_(const UserID uid) const;
    std::string HostUserName_() const;
    uint32_t PlayerNum_() const;
    uint32_t ComputerNum_() const;
    void EmplaceUser_(const UserID uid);

    mutable std::mutex mutex_;

    // bot
    BotCtx& bot_;

    // basic info
    const MatchID mid_;
    GameHandle& game_handle_;
    UserID host_uid_;
    const std::optional<GroupID> gid_;
    std::atomic<State> state_{State::NOT_STARTED};

    struct TimerController
    {
      public:
        void Start(Match& match, uint64_t sec, void* alert_arg, void(*alert_cb)(void*, uint64_t));

        void Stop(const Match& match);

      private:
        std::shared_ptr<bool> timer_is_over_; // must before match because atom stage will call StopTimer
        std::unique_ptr<Timer> timer_;

#ifdef TEST_BOT
      public:
        std::mutex before_handle_timeout_mutex_;
        std::condition_variable before_handle_timeout_cv_;
        bool before_handle_timeout_{false};
#endif
    };

#ifdef TEST_BOT
  public:
#endif

    TimerController timer_cntl_;

#ifdef TEST_BOT
  private:
#endif

    // options
    struct Options
    {
        struct ResourceHolder
        {
            std::string resource_dir_;
            std::string saved_image_dir_;
        };

        ResourceHolder resource_holder_;
        GameHandle::game_options_ptr game_options_;
        lgtbot::game::GenericOptions generic_options_;
    };
    Options options_;

    // game
    GameHandle::main_stage_ptr main_stage_{nullptr, [](const lgtbot::game::MainStageBase*) {}};

    // user info
    std::map<UserID, ParticipantUser> users_;

    // message senders
    class MsgSenderBatchHandler
    {
      public:
        MsgSenderBatchHandler(Match& match, const bool ai_only) : match_(match), ai_only_(ai_only) {};

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
    MsgSenderBatch<MsgSenderBatchHandler> boardcast_private_sender_{MsgSenderBatchHandler(*this, false)};
    MsgSenderBatch<MsgSenderBatchHandler> boardcast_ai_info_private_sender_{MsgSenderBatchHandler(*this, true)};
    std::optional<MsgSender> group_sender_;

    // player info (fill when game ready to start)
    struct Player
    {
        enum class State { ACTIVE, ELIMINATED, HOOKED };
        Player(const VariantID& id) : id_(id), state_(State::ACTIVE) {}
        VariantID id_;
        State state_;
    };
    std::vector<Player> players_; // is filled when game starts

    const Command<void(MsgSenderBase&)> help_cmd_{
        Command<void(MsgSenderBase&)>("查看游戏帮助", std::bind_front(&Match::Help_, this), VoidChecker("帮助"),
                OptionalDefaultChecker<BoolChecker>(false, "文字", "图片"))
    };

    bool is_in_deduction_{false};
};
