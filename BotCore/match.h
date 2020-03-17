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
  static std::string NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid);
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
  Match(const MatchId id, const GameHandle& game_handle, const UserID host_uid, const std::optional<GroupID> gid);
  ~Match();

  std::string Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg);
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

  int is_started() const { return is_started_; }
  MatchId mid() const { return mid_; }
  std::optional<GroupID> gid() const { return gid_; }
  UserID host_uid() const { return host_uid_; }
  const std::set<UserID>& ready_uid_set() const { return ready_uid_set_; }

private:
  const MatchId                     mid_;
  const GameHandle&                 game_handle_;
  UserID                     host_uid_;
  const std::optional<GroupID> gid_;
  bool    is_started_;
  GameBase*       game_;
  std::set<UserID>            ready_uid_set_;
  std::map<UserID, uint64_t>       uid2pid_;
  std::vector<UserID>              pid2uid_;
  std::unique_ptr<Timer, std::function<void(Timer*)>>      timer_;
};
