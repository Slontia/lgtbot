#include "stdafx.h"
#include "match.h"
#include "log.h"
#include "game_container.h"
#include "cqp.h"
#include "log.h"
#include "game.h"
#include "match.h"

GameContainer              game_container_;


MatchManager::MatchManager() : next_match_id(1) {}

std::shared_ptr<Match> MatchManager::new_private_match(const MatchId& id, const std::string& game_id, const QQ& host_qq)
{
  return std::dynamic_pointer_cast<Match>(std::make_shared<PrivateMatch>(id, game_id, host_qq));
}

std::shared_ptr<Match> MatchManager::new_group_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& group_qq)
{
  assert(group_matches != INVALID_LOBBY);
  auto match = std::make_shared<GroupMatch>(id, game_id, host_qq, group_qq);
  group_matches.emplace(group_qq, match);
  return std::dynamic_pointer_cast<Match>(match);
}

std::shared_ptr<Match> MatchManager::new_discuss_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& discuss_qq)
{
  assert(discuss_matches != INVALID_LOBBY);
  auto match = std::make_shared<DiscussMatch>(id, game_id, host_qq, discuss_qq);
  discuss_matches.emplace(discuss_qq, match);
  return std::dynamic_pointer_cast<Match>(match);
}

std::shared_ptr<Match> MatchManager::new_match(const MatchType& type, const std::string& game_id, const QQ& host_qq, const QQ& lobby_qq)
{
  if ((player_matches.find(host_qq) != player_matches.end())) return nullptr;

  /* Create id and match. */
  auto id = assign_id();
  std::shared_ptr<Match> new_match = nullptr;
  switch (type)
  {
    case PRIVATE_MATCH:
      new_match = new_private_match(id, game_id, host_qq);
      break;
    case GROUP_MATCH:
      new_match = new_group_match(id, game_id, host_qq, lobby_qq);
      break;
    case DISCUSS_MATCH:
      new_match = new_discuss_match(id, game_id, host_qq, lobby_qq);
      break;
    default:
      /* Never reach here! */
      assert(false);
  }

  /* Store into map. */
  assert(matches.find(id) == matches.end());
  matches.emplace(id, new_match);

  /* Add host player. */
  AddPlayer(id, host_qq);

  return new_match;
}

/* Assume usr_qq is valid. */
void MatchManager::AddPlayer(const MatchId& id, const QQ& usr_qq)
{
  /* Join match. */
  auto it = matches.find(id);
  assert(it != matches.end());
  auto match = it->second;

  match->broadcast("玩家 " + std::to_string(usr_qq) + " 加入了游戏");
  match->Join(usr_qq);

  /* Insert into player matches map. */
  assert(player_matches.find(usr_qq) == player_matches.end());
  player_matches.emplace(usr_qq, it->second);

  match->private_respond(usr_qq, "您已加入游戏");
}

/* Assume usr_qq is valid. */
void MatchManager::DeletePlayer(const QQ& usr_qq)
{
  /* Leave match. */
  auto it = player_matches.find(usr_qq);
  assert(it != player_matches.end());
  auto match = it->second;
  match->Leave(usr_qq);

  /* Erase from player matches map. */
  player_matches.erase(usr_qq);

  match->private_respond(usr_qq, "您已退出游戏");
  match->broadcast("玩家 " + std::to_string(usr_qq) + " 退出了游戏");

  /* If host quit, switch host. */
  if (usr_qq == match->get_host_qq() && !match->switch_host())
  {
    DeleteMatch(match->get_id());
  }
}

/* return true if is a game request */
bool MatchManager::Request(MessageIterator& msg)
{
  switch (msg.type_)
  {
    case PRIVATE_MSG:
    {
      auto match = player_matches.find(msg.usr_qq_);
      if (match == player_matches.end()) return false;
      match->second->Request(msg);
      return true;
    }
    case GROUP_MSG:
      return PublicRequest(msg, group_matches);
    case DISCUSS_MSG:
      return PublicRequest(msg, discuss_matches);
    default:
      break;
  }
  /* Never reach here! */
  assert(false);
  return false;
}

void MatchManager::DeleteMatch(MatchId id)
{
  auto it = matches.find(id);
  assert(it != matches.end());
  auto match = it->second;
  /* Delete match from group or discuss. */
  switch (match->get_type())
  {
    case GROUP_MATCH:
      group_matches.erase(std::dynamic_pointer_cast<GroupMatch>(match)->get_group_qq());
      break;
    case DISCUSS_MATCH:
      discuss_matches.erase(std::dynamic_pointer_cast<DiscussMatch>(match)->get_discuss_qq());
      break;
    default:
      assert(match->get_type() == PRIVATE_MATCH);
  }

  /* Delete match from id. */
  matches.erase(id);

  match->broadcast("游戏中止或结束");

  /* Delete players. */
  auto ready_iterators = match->get_ready_iterator();
  for (auto it = ready_iterators.first; it != ready_iterators.second; ++it)
  {
    DeletePlayer(*it);
  }
}


MatchId MatchManager::assign_id()
{
  /* If no available id, create new one. */
  if (match_ids.empty()) return next_match_id++;

  /* Or fetch from stack. */
  auto id = match_ids.top();
  match_ids.pop();
  return id;
}

bool MatchManager::PublicRequest(MessageIterator& msg, const std::unordered_map<QQ, std::shared_ptr<Match>> public_matches)
{
  auto player_match = player_matches.find(msg.usr_qq_);
  auto public_match = public_matches.find(msg.src_qq_);
  if (player_match == player_matches.end() ||
      public_match == public_matches.end() ||
      player_match->second->get_id() != public_match->second->get_id()) return false;
  player_match->second->Request(msg);
  return true;
}


Match::Match(const MatchId& id, const std::string& game_id, const int64_t& host_qq, const MatchType& type) :
  id_(id), type_(type), game_id_(game_id), host_qq_(host_qq), status_(PREPARE), game_(game_container_.MakeGame(game_id_, *this))
{}

bool Match::has_qq(const int64_t& qq) const
{
  return ready_qq_set_.find(qq) != ready_qq_set_.end();
}

int Match::Request(MessageIterator& msg)
{
  if (status_ != GAMING) return GAME_NOT_STARTED;
  assert(has_qq(from_qq));
  
  game_->Request(qq2pid_[msg.usr_qq_], msg);
  return SUCC;
}

/* Register players, switch status and start the game 
*/
int Match::GameStart()
{
  if (status_ != PREPARE)
  {
    LOG_ERROR("GameStart: Game has started.");
    return GAME_STARTED;
  }
  player_count_ = ready_qq_set_.size();
  if (player_count_ < game_->kMinPlayer)
  {
    return TOO_FEW_PLAYERS;
  }
  for (auto qq : ready_qq_set_)
  {
    pid2qq_.push_back(qq);
    qq2pid_.emplace(qq, pid2qq_.size() - 1);
  }
  status_ = GAMING;
  game_->StartGame();
  return SUCC;
}

/* append new player to qq_list
*/
int Match::Join(const int64_t& qq)
{
  if (status_ != PREPARE)
  {
    return GAME_STARTED;
  }
  if (has_qq(qq))
  {
    return HAS_JOINED;
  }
  if (ready_qq_set_.size() >= game_->kMaxPlayer)
  {
    return TOO_MANY_PLAYERS;
  }
  ready_qq_set_.insert(qq); // add to queue
  return SUCC;
}

/* remove player from qq_list
*/
int Match::Leave(const int64_t& qq)
{
  if (status_ != PREPARE)
  {
    return GAME_STARTED;
  }
  if (!has_qq(qq))
  {
    return NOT_JOINED;
  }
  ready_qq_set_.erase(qq);  // remove from queue
  return SUCC;
}