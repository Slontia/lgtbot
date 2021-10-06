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

#define INVALID_LOBBY (QQ)0

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

class Match : public MatchBase, public std::enable_shared_from_this<Match>
{
  public:
    using VariantID = std::variant<UserID, ComputerID>;
    enum State { NOT_STARTED = 'N', IS_STARTED = 'S', IS_OVER = 'O' };
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID id, const GameHandle& game_handle, const UserID host_uid,
          const std::optional<GroupID> gid);
    ~Match();

    ErrCode SetBenchTo(const UserID uid, MsgSenderBase& reply, const uint64_t com_num);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, MsgSender& reply);
    ErrCode GameConfigOver(MsgSenderBase& reply);
    ErrCode GameStart(const UserID uid, const bool is_public, MsgSenderBase& reply);
    ErrCode Join(const UserID uid, MsgSenderBase& reply);
    ErrCode Leave(const UserID uid, MsgSenderBase& reply, const bool force);
    ErrCode LeaveMidway(const UserID uid, const bool is_public);
    virtual MsgSenderBase& BoardcastMsgSender() override;
    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) override;
    MsgSenderBase::MsgSenderGuard Boardcast() { return BoardcastMsgSender()(); }
    MsgSenderBase::MsgSenderGuard Tell(const PlayerID pid) { return TellMsgSender(pid)(); }
    virtual void StartTimer(const uint64_t sec) override;
    virtual void StopTimer() override;
    void ShowInfo(const std::optional<GroupID>&, MsgSenderBase& reply) const;

    bool SwitchHost();

    bool Has(const UserID uid) const;
    bool IsPrivate() const { return !gid_.has_value(); }
    auto PlayerNum() const { return players_.size(); }

    VariantID ConvertPid(const PlayerID pid) const;

    void Terminate();

    const GameHandle& game_handle() const { return game_handle_; }
    MatchID mid() const { return mid_; }
    std::optional<GroupID> gid() const { return gid_; }
    UserID host_uid() const { return host_uid_; }
    const auto& users() const { return users_; }
    const State state() const { return state_; }
    MatchManager& match_manager() { return bot_.match_manager(); }

    const uint64_t user_controlled_player_num() const { return users_.size() * player_num_each_user_; }
    const uint64_t com_num() const { return std::max(0L, static_cast<int64_t>(bench_to_player_num_ - user_controlled_player_num())); }

    struct ScoreInfo {
        UserID uid_;
        int64_t game_score_;
        double zero_sum_match_score_;
        uint64_t poss_match_score_;
    };

   private:
    std::string State2String()
    {
        switch (state_) {
        case State::NOT_STARTED:
            return "未开始";
        case State::IS_STARTED:
            return "已开始";
        case State::IS_OVER:
            return "已结束";
        }
    }
    std::vector<ScoreInfo> CalScores_(const std::vector<int64_t>& scores) const;
    void OnGameOver_();
    void Help_(MsgSenderBase& reply, const bool text_mode);
    void Routine_();
    std::string OptionInfo_() const;
    void KickForConfigChange_();
    void Unbind_();
    void Terminate_();

    mutable std::mutex mutex_;

    // bot
    BotCtx& bot_;

    // basic info
    const MatchID mid_;
    const GameHandle& game_handle_;
    UserID host_uid_;
    const std::optional<GroupID> gid_;
    State state_;

    // time info
    std::shared_ptr<bool> timer_is_over_; // must before match because atom stage will call StopTimer
    std::unique_ptr<Timer> timer_;
    std::chrono::time_point<std::chrono::system_clock> start_time_;
    std::chrono::time_point<std::chrono::system_clock> end_time_;

    // game
    GameHandle::game_options_ptr options_;
    GameHandle::main_stage_ptr main_stage_;

    // player info
    struct ParticipantUser
    {
        ParticipantUser(const UserID uid)
            : uid_(uid)
            , sender_(uid)
            , is_left_during_game_(false)
            , leave_when_config_changed_(true)
        {}
        const UserID uid_;
        std::vector<PlayerID> pids_;
        MsgSender sender_;
        bool leave_when_config_changed_;
        bool is_left_during_game_;
    };
    std::map<UserID, ParticipantUser> users_;
    std::variant<MsgSender, MsgSenderBatch> boardcast_sender_;

    // other options
    uint64_t bench_to_player_num_;
    uint64_t player_num_each_user_;
    uint16_t multiple_;

    // player info (fill when game ready to start)
    std::vector<VariantID> players_; // all players, include computers

    const Command<void(MsgSenderBase&)> help_cmd_;
};
