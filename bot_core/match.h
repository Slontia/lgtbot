// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cassert>

#include <map>
#include <set>
#include <bitset>
#include <memory>
#include <thread>
#include <variant>
#include <vector>

#include "utility/msg_checker.h"

#include "bot_core/match_base.h"
#include "bot_core/msg_sender.h"
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
class MatchChildClient;
struct PostFrame;
struct PlayerStateFrame;
struct GameOverFrame;

class Match : public MatchBase, public std::enable_shared_from_this<Match>
{
  public:
    using VariantID = std::variant<UserID, ComputerID>;
    enum State { NOT_STARTED = 'N', IS_STARTED = 'S', IS_OVER = 'O' };
    static const uint32_t kAvgScoreOffset = 10;

    struct InitOptions
    {
        uint32_t bench_computers_to_player_num_{0};
        bool is_formal_{true};
        std::vector<std::string> applied_options_log_;
    };

    Match(BotCtx& bot, const MatchID id, GameHandle& game_handle, InitOptions init_options,
            const UserID host_uid, const std::optional<GroupID> gid);
    ~Match();

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
    virtual const char* GameName() const override { return game_handle_.Info().name_; }

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
    auto UserNum() const { std::lock_guard<std::mutex> l(mutex_); return users_.size(); }

    VariantID ConvertPid(const PlayerID pid) const;

    ErrCode Terminate(const bool is_force);

    const GameHandle& game_handle() const { return game_handle_; }
    std::optional<GroupID> gid() const { return gid_; }
    UserID HostUserId() const { std::lock_guard<std::mutex> l(mutex_); return host_uid_; }
    const State state() const { return state_; }
    MatchManager& match_manager() { return bot_.match_manager(); }

    void BriefInfo(std::string& out) const;

    void ReleaseGameChildIfOver();

    friend class MatchChildClient;

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

    uint64_t MaxPlayerNum_() const { return game_handle_.CachedMaxPlayer(); }
    uint32_t Multiple_() const { return game_handle_.CachedMultiple(); }

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

    void BriefInfo_(std::string& out) const;
    void Help_(MsgSenderBase& reply, const bool text_mode);
    std::string OptionInfo_() const;
    void ApplyChildPlayerState(const PlayerID pid, const std::string& state);
    void ApplyChildGameOverFromScores(const struct GameOverFrame& frame);
    void ApplyChildPost_(const struct PostFrame& frame);
    void StartReadThread_();
    void StopReadThread_();
    void KickForConfigChange_();
    void Unbind_();
    void UnbindMatchSide_();
    // Signal the game child to stop and unbind the match.
    // Must be called under mutex_. Returns the moved-out game child for the caller to
    // destroy outside the lock (to avoid deadlocking with the read thread callbacks).
    std::unique_ptr<MatchChildClient> PrepareTerminate_();

    // Finish a terminate initiated by PrepareTerminate_: join the read thread and
    // let the child be destroyed. Must be called WITHOUT holding mutex_.
    void FinishTerminate_(std::unique_ptr<MatchChildClient> child);

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

    // options
    struct RuntimeOptions
    {
        struct ResourceHolder
        {
            std::string resource_dir_;
            std::string saved_image_dir_;
        };

        ResourceHolder resource_holder_;
        lgtbot::game::GenericOptions generic_options_;
    };
    RuntimeOptions options_;

    std::unique_ptr<MatchChildClient> game_child_;
    std::thread read_thread_;
    std::vector<std::string> applied_options_log_;

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
