#pragma once

#include <iostream>
#include <map>
#include <assert.h>
#include <set>
#include <stack>
#include <optional>
#include <sstream>

#include "lgtbot.h"
#include "spinlock.h"

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
  static std::string NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid);
  static std::string StartGame(const UserID uid);
  static std::string AddPlayerWithMatchID(const MatchId mid, const UserID uid);
  static std::string AddPlayerWithGroupID(const GroupID gid, const UserID uid);
  static std::string DeletePlayer(const UserID uid);
  static void DeleteMatch(const MatchId id);
  static std::shared_ptr<Match> GetMatch(const MatchId mid);
  static std::shared_ptr<Match> GetMatch(const UserID uid, const std::optional<GroupID> gid);

private:
  static std::string AddPlayer_(const std::shared_ptr<Match>& match, const UserID);

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

  static MatchId NewMatchID();

  static std::map<MatchId, std::shared_ptr<Match>> mid2match_;
  static std::map<UserID, std::shared_ptr<Match>> uid2match_;
  static std::map<GroupID, std::shared_ptr<Match>> gid2match_;
  static MatchId next_mid_;
  static SpinLock spinlock_;
};

class Match
{
public:
  Match(const MatchId id, const GameHandle& game_handle, const UserID host_uid, const std::optional<GroupID> gid);
  ~Match();

  std::string Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg);
  std::string GameStart();
  std::string Join(const UserID uid);
  std::string Leave(const UserID uid);
  void Boardcast(const std::string& msg) const;
  void Tell(const uint64_t pid, const std::string& msg) const;
  std::string At(const uint64_t pid) const;
  void At(const uint64_t pid, char* buf, const uint64_t len) const;
  void GameOver(const int64_t scores[])
  {
    assert(ready_uid_set_.size() == pid2uid_.size());
    std::ostringstream ss;
    ss << "游戏结束，公布分数：" << std::endl;
    for (uint64_t pid = 0; pid < pid2uid_.size(); ++pid) { ss << At(pid) << " " << scores[pid] << std::endl; }
    ss << "感谢大家参与！";
    Boardcast(ss.str());
    //TODO: link to database
  }

  bool SwitchHost()
  {
    if (ready_uid_set_.empty()) { return false; }
    host_uid_ = *ready_uid_set_.begin();
    return true;
  }

  bool Has(const UserID uid) const;

  MatchId mid() const { return mid_; }
  std::optional<GroupID> gid() const { return gid_; }
  UserID host_uid() const { return host_uid_; }
  const std::set<UserID>& ready_uid_set() const { return ready_uid_set_; }

protected:
  const MatchId                     mid_;
  const GameHandle&                 game_handle_;
  UserID                     host_uid_;
  const std::optional<GroupID> gid_;
  enum { PREPARE, GAMING, OVER }    state_;
  GameBase*       game_;
  std::set<UserID>            ready_uid_set_;
  std::map<UserID, uint64_t>       uid2pid_;
  std::vector<UserID>              pid2uid_;
};
