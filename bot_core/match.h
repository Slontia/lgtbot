#pragma once
#include <assert.h>

#include <map>
#include <set>

#include "util.h"
#include "utility/msg_sender.h"
#include "utility/spinlock.h"
#include "utility/timer.h"

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

class Match : public std::enable_shared_from_this<Match>
{
   public:
    enum State { IN_CONFIGURING = 'C', NOT_STARTED = 'N', IS_STARTED = 'S' };
    static const uint32_t kAvgScoreOffset = 10;

    Match(BotCtx& bot, const MatchID id, const GameHandle& game_handle, const UserID host_uid,
          const std::optional<GroupID> gid, const bool skip_config);
    ~Match();

    ErrCode SetComNum(const uint64_t com_num, const replier_t reply);

    ErrCode Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg, const replier_t reply);
    ErrCode GameConfigOver(const replier_t reply);
    ErrCode GameStart(const bool is_public, const replier_t reply);
    ErrCode Join(const UserID uid, const replier_t reply);
    ErrCode Leave(const UserID uid, const replier_t reply, const bool force);
    ErrCode LeaveMidway(const UserID uid, const bool is_public);
    MsgSenderWrapper<MsgSenderForBot> Boardcast() const;
    MsgSenderWrapper<MsgSenderForBot> Tell(const uint64_t pid) const;
    template <bool IS_AT, typename Sender>
    void PrintPlayer(Sender& sender, const uint64_t pid)
    {
        sender << "[" << pid << "号]";
        if (pid >= pid2uid_.size()) {
            sender << "机器人" << pid - pid2uid_.size() << "号";
        } else if (IS_AT) {
            sender << AtMsg(pid2uid_[pid]);
        } else if (gid().has_value()) {
            sender << GroupUserMsg(pid2uid_[pid], *gid());
        } else {
            sender << UserMsg(pid2uid_[pid]);
        }
    }
    void GamePrepare();
    void GameOver(const int64_t scores[]);
    void StartTimer(const uint64_t sec);
    void StopTimer();
    std::string OptionInfo() const;

    bool SwitchHost();

    bool Has(const UserID uid) const;
    bool IsPrivate() const { return !gid_.has_value();  + com_num_; }
    auto PlayerNum() const { return pid2uid_.size() + com_num_; }

    std::variant<UserID, ComputerID> ConvertPid(const uint64_t pid) const;

    const GameHandle& game_handle() const { return game_handle_; }
    MatchID mid() const { return mid_; }
    std::optional<GroupID> gid() const { return gid_; }
    UserID host_uid() const { return host_uid_; }
    const std::set<UserID>& ready_uid_set() const { return ready_uid_set_; }
    const State state() const { return state_; }
    MatchManager& match_manager() { return bot_.match_manager(); }

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
        case State::IN_CONFIGURING:
            return "配置中";
        case State::NOT_STARTED:
            return "未开始";
        case State::IS_STARTED:
            return "已开始";
        }
    }
    std::vector<ScoreInfo> CalScores_(const int64_t scores[]) const;
    bool SatisfyMaxPlayer_() const;

    // bot
    BotCtx& bot_;

    // basic info
    const MatchID mid_;
    const GameHandle& game_handle_;
    UserID host_uid_;
    const std::optional<GroupID> gid_;
    State state_;

    // game
    std::unique_ptr<GameBase, const std::function<void(GameBase* const)>> game_;

    // player info , player_num == com_num_ + pid2uid_.size() == com_num_ + uid2pid_.size() * player_num_each_user_
    std::set<UserID> ready_uid_set_; // players is now in game, exclude exited players
    std::map<UserID, uint64_t> uid2pid_; // players user currently use
    std::vector<UserID> pid2uid_; // all players, exclude computers
    uint64_t player_num_each_user_;
    uint64_t com_num_;

    // time info
    std::unique_ptr<Timer, std::function<void(Timer*)>> timer_;
    std::chrono::time_point<std::chrono::system_clock> start_time_;
    std::chrono::time_point<std::chrono::system_clock> end_time_;

    const uint16_t multiple_;
};
