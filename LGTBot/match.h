#pragma once

#include <iostream>
#include <map>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <stack>

#include "lgtbot.h"

#define INVALID_LOBBY 0

typedef int MatchId;
typedef enum
{
  SUCC,
  TOO_MANY_PLAYERS, 
  TOO_FEW_PLAYERS,
  GAME_STARTED,
  GAME_NOT_STARTED,
  HAS_JOINED,
  NOT_JOINED
} ErrNo;

typedef enum
{
  PRIVATE_MATCH,
  GROUP_MATCH,
  DISCUSS_MATCH
} MatchType;

typedef enum
{
  PRIVATE_MATCH_MSG = 1,
  PUBLIC_MATCH_MSG,
  INVALLID_MATCH_MSG = 0
} MatchRequestType;

class GamePlayer;     // player info
class Game;
class Match;
class PrivateMatch;
class GroupMatch;
class DiscussMatch;

class MatchManager
{
public:
  MatchManager();

  std::shared_ptr<Match> new_private_match(const MatchId& id, const std::string& game_id, const QQ& host_qq);

  std::shared_ptr<Match> new_group_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& group_qq);

  std::shared_ptr<Match> new_discuss_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& discuss_qq);

  std::shared_ptr<Match> new_match(const MatchType& type, const std::string& game_id, const QQ& host_qq, const QQ& lobby_qq);

  /* Assume usr_qq is valid. */
  void AddPlayer(const MatchId& id, const QQ& usr_qq);

  /* Assume usr_qq is valid. */
  void DeletePlayer(const QQ& usr_qq);

  /* return true if is a game request */
  bool Request(MessageIterator& msg);

  void DeleteMatch(MatchId id);
  

private:
  std::map<MatchId, std::shared_ptr<Match>> matches;
  std::unordered_map<QQ, std::shared_ptr<Match>> discuss_matches;
  std::unordered_map<QQ, std::shared_ptr<Match>> group_matches;
  std::unordered_map<QQ, std::shared_ptr<Match>> player_matches;
  std::stack<MatchId> match_ids;
  MatchId next_match_id;

  MatchId assign_id();

  bool PublicRequest(MessageIterator& msg, const std::unordered_map<QQ, std::shared_ptr<Match>> public_matches);
};

class Match
{
public:
  typedef enum { PRIVATE_MATCH, GROUP_MATCH, DISCUSS_MATCH } MatchType;
  const MatchType                   type_;

  Match(const MatchId& id, const std::string& game_id, const int64_t& host_qq, const MatchType& type);

  int         Request(MessageIterator& msg);
  /* switch status, create game */
  int                GameStart();
  /* send msg to all player */
  virtual void broadcast(const std::string&) const = 0;
  virtual void private_respond(const QQ&, const std::string&) const = 0;
  virtual void public_respond(const QQ&, const std::string&) const = 0;
  /* bind to map, create GamePlayer and send to game */
  int                              Join(const int64_t& qq);
  int                              Leave(const int64_t& qq);

  bool has_qq(const int64_t& qq) const;
  const MatchId& get_id() const { return id_; }
  MatchType get_type() const { return type_; }
  std::pair<std::unordered_set<QQ>::const_iterator, std::unordered_set<QQ>::const_iterator> get_ready_iterator() const
  { 
    return { ready_qq_set_.begin(), ready_qq_set_.end() };
  }
  const QQ& get_host_qq() const { return host_qq_; }
  bool switch_host()
  {
    if (ready_qq_set_.empty()) return false;
    host_qq_ = *ready_qq_set_.begin();
    return true;
  }

protected:
  enum { PREPARE, GAMING, OVER }    status_;
  const MatchId                     id_;
  const std::string                 game_id_;
  QQ                     host_qq_;
  std::unique_ptr<Game>       game_;
  std::unordered_set<QQ>            ready_qq_set_;
  std::map<QQ, uint32_t>       qq2pid_;
  std::vector<QQ>              pid2qq_;
  uint32_t                          player_count_;
};

class PrivateMatch : public Match
{
public:
  PrivateMatch(const MatchId& id, const std::string& game_id, const int64_t& host_qq) : Match(id, game_id, host_qq, PRIVATE_MATCH) {}

  void broadcast(const std::string& msg) const
  {
    for (auto player : qq2pid_)
    {
      send_private_msg(player.first, msg);
    }
  }

  void public_respond(const QQ& user_qq, const std::string& msg) const
  {
    send_private_msg(user_qq, msg);
  }

  void private_respond(const QQ& user_qq, const std::string& msg) const
  {
    send_private_msg(user_qq, msg);
  }
};

class GroupMatch : public Match
{
public:
  GroupMatch(const MatchId& id, const std::string& game_id, const int64_t& host_qq, const QQ& group_qq) : Match(id, game_id, host_qq, GROUP_MATCH), group_qq_(group_qq) {}

  void broadcast(const std::string& msg) const
  {
    send_group_msg(group_qq_, msg);
  }

  void public_respond(const QQ& usr_qq, const std::string& msg) const
  {
    send_group_msg(group_qq_, msg, usr_qq);
  }

  void private_respond(const QQ& user_qq, const std::string& msg) const
  {
    send_private_msg(user_qq, msg);
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
    send_discuss_msg(discuss_qq_, msg);
  }

  void public_respond(const QQ& usr_qq, const std::string& msg) const
  {
    send_discuss_msg(discuss_qq_, msg, usr_qq);
  }

  void private_respond(const QQ& user_qq, const std::string& msg) const
  {
    send_private_msg(user_qq, msg);
  }

  const QQ& get_discuss_qq() const { return discuss_qq_; }

private:
  const QQ discuss_qq_;
};


extern MatchManager match_manager;
