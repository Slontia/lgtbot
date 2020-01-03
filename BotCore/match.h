#pragma once

#include <iostream>
#include <map>
#include <assert.h>
#include <set>
#include <stack>
#include <optional>

#include "lgtbot.h"
#include "message_iterator.h"

typedef uint32_t MatchId;

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


extern MatchManager match_manager;

class MatchManager
{
public:
  MatchManager();
  ~MatchManager() = default;
  std::string NewMatch(const GameHandle& game_handle, const UserID uid, const std::optional<GroupID> gid);
  std::string StartGame(const UserID uid);
  std::string AddPlayer(const MatchId mid, const UserID uid);
  std::string AddPlayer(const GroupID gid, const UserID uid);
  std::string DeletePlayer(const UserID uid);
  std::string Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg);
  void DeleteMatch(const MatchId id);

private:
  MatchId NewMatchID();

  std::map<MatchId, std::shared_ptr<Match>> mid2match_;
  std::map<UserID, std::shared_ptr<Match>> uid2match_;
  std::map<GroupID, std::shared_ptr<Match>> gid2match_;
  MatchId next_mid_;
};

class Match
{
public:
  Match(const MatchId id, const GameHandle& game_handle, const UserID host_uid, const std::optional<GroupID> gid);

  std::string         Request(const UserID uid, const std::optional<GroupID> gid, const std::string& msg);
  /* switch status, create game */
  std::string                GameStart();
  /* send msg to all player */
  virtual void broadcast(const std::string&) const = 0;
  virtual void private_respond(const QQ&, const std::string&) const = 0;
  virtual void public_respond(const QQ&, const std::string&) const = 0;
  /* bind to map, create GamePlayer and send to game */
  std::string                              Join(const UserID uid);
  std::string                              Leave(const UserID uid);

  bool Has(const UserID uid) const;
  MatchId mid() const { return mid_; }
  std::optional<GroupID> gid() const { return gid_; }
  UserID host_uid() const { return host_uid_; }
  const std::set<UserID>& ready_uid_set() const { return ready_uid_set_; }
  bool SwitchHost()
  {
    if (ready_uid_set_.empty()) { return false; }
    host_uid_ = *ready_uid_set_.begin();
    return true;
  }
  bool is_started() { return state_ != PREPARE; }

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

class PrivateMatch : public Match
{
public:
  PrivateMatch(const MatchId& id, const std::string& game_id, const int64_t& host_qq) : Match(id, game_id, host_qq, PRIVATE_MATCH) {}

  void broadcast(const std::string& msg) const
  {
    for (auto qq : ready_qq_set_)
    {
      LGTBOT::send_private_msg(qq, msg);
    }
  }

  void public_respond(const QQ& user_qq, const std::string& msg) const
  {
    LGTBOT::send_private_msg(user_qq, msg);
  }

  void private_respond(const QQ& user_qq, const std::string& msg) const
  {
    LGTBOT::send_private_msg(user_qq, msg);
  }
};

class GroupMatch : public Match
{
public:
  GroupMatch(const MatchId& id, const std::string& game_id, const int64_t& host_qq, const QQ& group_qq) : Match(id, game_id, host_qq, GROUP_MATCH), group_qq_(group_qq) {}

  void broadcast(const std::string& msg) const
  {
    LGTBOT::send_group_msg(group_qq_, msg);
  }

  void public_respond(const QQ& usr_qq, const std::string& msg) const
  {
    LGTBOT::send_group_msg(group_qq_, msg, usr_qq);
  }

  void private_respond(const QQ& user_qq, const std::string& msg) const
  {
    LGTBOT::send_private_msg(user_qq, msg);
  }

  const QQ& get_group_qq() const { return group_qq_; }

private:
  const QQ group_qq_;
};

class DiscussMatch : public Match
{
public:
  DiscussMatch(const MatchId& id, const std::string& game_id, const int64_t& host_qq, const QQ& discuss_qq) : Match(id, game_id, host_qq, DISCUSS_MATCH), discuss_qq_(discuss_qq) {}

  void broadcast(const std::string& msg) const
  {
    LGTBOT::send_discuss_msg(discuss_qq_, msg);
  }

  void public_respond(const QQ& usr_qq, const std::string& msg) const
  {
    LGTBOT::send_discuss_msg(discuss_qq_, msg, usr_qq);
  }

  void private_respond(const QQ& user_qq, const std::string& msg) const
  {
    LGTBOT::send_private_msg(user_qq, msg);
  }

  const QQ& get_discuss_qq() const { return discuss_qq_; }

private:
  const QQ discuss_qq_;
};


extern MatchManager match_manager;
