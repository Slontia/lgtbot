#pragma once

#include <iostream>
#include <map>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <stack>

#include "lgtbot.h"
#include "message_iterator.h"
#include "game.h"

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

class GamePlayer;     // player info
class Game;
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

  MatchId MatchManager::get_group_match_id(const QQ& group_qq);

  MatchId MatchManager::get_discuss_match_id(const QQ& discuss_qq);

  static MatchId MatchManager::get_match_id(const QQ& src_qq, std::unordered_map<QQ, std::shared_ptr<Match>> match_map);

  std::shared_ptr<Match> new_private_match(const MatchId& id, const std::string& game_id, const QQ& host_qq);

  std::shared_ptr<Match> new_group_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& group_qq);

  std::shared_ptr<Match> new_discuss_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& discuss_qq);

  ErrMsg new_match(const MatchType& type, const std::string& game_id, const QQ& host_qq, const QQ& lobby_qq);

  ErrMsg StartGame(const QQ& host_qq);

  /* Assume usr_qq is valid. */
  ErrMsg AddPlayer(const MatchId& id, const QQ& usr_qq);

  /* Assume usr_qq is valid. */
  ErrMsg DeletePlayer(const QQ& usr_qq);

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

  void         Request(MessageIterator& msg);
  /* switch status, create game */
  std::string                GameStart();
  /* send msg to all player */
  virtual void broadcast(const std::string&) const = 0;
  virtual void private_respond(const QQ&, const std::string&) const = 0;
  virtual void public_respond(const QQ&, const std::string&) const = 0;
  /* bind to map, create GamePlayer and send to game */
  std::string                              Join(const int64_t& qq);
  std::string                              Leave(const int64_t& qq);

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
  bool is_started() { return status_ != PREPARE; }

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
