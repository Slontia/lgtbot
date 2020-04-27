#pragma once
#include "spinlock.h"
#include "timer.h"

#define INVALID_LOBBY (QQ)0

#define INVALID_MATCH (MatchId)0

inline bool match_is_valid(MatchId id) { return id != INVALID_MATCH; }

typedef enum
{
  PRIVATE_MATCH,
  GROUP_MATCH,
  DISCUSS_MATCH
} MatchType;

class GameBase;
class Match;
class PrivateMatch;
class GroupMatch;
class DiscussMatch;
class MatchManager;

class MatchManager
{
public:
  static std::string NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid, const bool skip_config);
  static std::string ConfigOver(const UserID uid, const std::optional<GroupID> gid);
  static std::string StartGame(const UserID uid, const std::optional<GroupID> gid);
  static std::string AddPlayerToPrivateGame(const MatchId mid, const UserID uid);
  static std::string AddPlayerToPublicGame(const GroupID gid, const UserID uid);
  static std::string DeletePlayer(const UserID uid, const std::optional<GroupID> gid);
  static void DeleteMatch(const MatchId id);
  static std::shared_ptr<Match> GetMatch(const MatchId mid);
  static std::shared_ptr<Match> GetMatch(const UserID uid, const std::optional<GroupID> gid);
  static std::shared_ptr<Match> GetMatchWithGroupID(const GroupID gid);
  static void ForEachMatch(const std::function<void(const std::shared_ptr<Match>)>);

private:
  static std::string AddPlayer_(const std::shared_ptr<Match>& match, const UserID);
  static void DeleteMatch_(const MatchId id);

  template <typename IDType>
  static std::shared_ptr<Match> GetMatch_(const IDType id, const std::map<IDType, std::shared_ptr<Match>>& id2match)
  {
    const auto it = id2match.find(id);
    return (it == id2match.end()) ? nullptr : it->second;
  }

  template <typename IDType>
  static void BindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match, std::shared_ptr<Match> match)
  {
    if (!id2match.emplace(id, match).second) { assert(false); }
  }

  template <typename IDType>
  static void UnbindMatch_(const IDType id, std::map<IDType, std::shared_ptr<Match>>& id2match)
  {
    id2match.erase(id);
  }

  static MatchId NewMatchID_();

  static std::map<MatchId, std::shared_ptr<Match>> mid2match_;
  static std::map<UserID, std::shared_ptr<Match>> uid2match_;
  static std::map<GroupID, std::shared_ptr<Match>> gid2match_;
  static MatchId next_mid_;
  static SpinLock spinlock_;
};

class Match : public std::enable_shared_from_this<Match>
{
public:
  enum State { IN_CONFIGURING = 'C', NOT_STARTED = 'N', IS_STARTED = 'S' };
  static const uint32_t kAvgScoreOffset = 10;

  Match(const MatchId id, const GameHandle& game_handle, const UserID host_uid, const std::optional<GroupID> gid, const bool skip_config);
  ~Match();

  std::string Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg);
  std::string GameConfigOver();
  std::string GameStart();
  std::string Join(const UserID uid);
  std::string Leave(const UserID uid);
  void BoardcastPlayers(const std::string& msg) const;
  void TellPlayer(const uint64_t pid, const std::string& msg) const;
  std::string AtPlayer(const uint64_t pid) const;
  void AtPlayer(const uint64_t pid, char* buf, const uint64_t len) const;
  void GameOver(const int64_t scores[]);
  void StartTimer(const uint64_t sec);
  void StopTimer();

  bool SwitchHost();

  bool Has(const UserID uid) const;
  bool IsPrivate() const { return !gid_.has_value(); }
  const GameHandle& game_handle() const { return game_handle_; }

  MatchId mid() const { return mid_; }
  std::optional<GroupID> gid() const { return gid_; }
  UserID host_uid() const { return host_uid_; }
  const std::set<UserID>& ready_uid_set() const { return ready_uid_set_; }
  const State state() const { return state_; }

  struct ScoreInfo
  {
    UserID uid_;
    int64_t game_score_;
    double zero_sum_match_score_;
    uint64_t poss_match_score_;
  };
private:
  std::string State2String()
  {
    switch (state_)
    {
    case State::IN_CONFIGURING: return "配置中";
    case State::NOT_STARTED: return "未开始";
    case State::IS_STARTED: return "已开始";
    }
  }
  std::vector<ScoreInfo> CalScores_(const int64_t scores[]) const;

  const MatchId                     mid_;
  const GameHandle&                 game_handle_;
  UserID                     host_uid_;
  const std::optional<GroupID> gid_;
  State state_;
  std::unique_ptr<GameBase, const std::function<void(GameBase* const)>>       game_;
  std::set<UserID>            ready_uid_set_;
  std::map<UserID, uint64_t>       uid2pid_;
  std::vector<UserID>              pid2uid_;
  std::unique_ptr<Timer, std::function<void(Timer*)>>      timer_;
  std::chrono::time_point<std::chrono::system_clock> start_time_;
  std::chrono::time_point<std::chrono::system_clock> end_time_;
  const uint16_t multiple_;
};
