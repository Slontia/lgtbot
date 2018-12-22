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
  assert(id != INVALID_LOBBY);

  /* If the group has match, return. */
  if (group_matches.find(group_qq) != group_matches.end()) return nullptr;

  /* Create new match. */
  auto match = std::make_shared<GroupMatch>(id, game_id, host_qq, group_qq);

  group_matches.emplace(group_qq, match);
  return std::dynamic_pointer_cast<Match>(match);
}

std::shared_ptr<Match> MatchManager::new_discuss_match(const MatchId& id, const std::string& game_id, const QQ& host_qq, const QQ& discuss_qq)
{
  assert(id != INVALID_LOBBY);

  /* If the discuss has match, return. */
  if (discuss_matches.find(discuss_qq) != discuss_matches.end()) return nullptr;

  /* Create new match. */
  auto match = std::make_shared<DiscussMatch>(id, game_id, host_qq, discuss_qq);

  discuss_matches.emplace(discuss_qq, match);
  return std::dynamic_pointer_cast<Match>(match);
}

ErrMsg MatchManager::new_match(const MatchType& type, const std::string& game_id, const QQ& host_qq, const QQ& lobby_qq)
{
  if ((player_matches.find(host_qq) != player_matches.end())) return "您已加入游戏，不能新建游戏";

  /* Create id and match. */
  auto id = assign_id();
  std::shared_ptr<Match> new_match = nullptr;
  switch (type)
  {
    case PRIVATE_MATCH:
      new_match = new_private_match(id, game_id, host_qq);
      assert(new_match);
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
  if (!new_match) return "该房间已经开始了游戏，不能再新建游戏了";

  /* Store match into map. */
  assert(matches.find(id) == matches.end());
  matches.emplace(id, new_match);

  /* Add host player. */
  RETURN_IF_FAILED(AddPlayer(id, host_qq));

  return "";
}

ErrMsg MatchManager::StartGame(const QQ& host_qq)
{
  auto it = player_matches.find(host_qq);
  if (it == player_matches.end()) return "您未加入游戏";

  auto match = it->second;
  if (match->get_host_qq() != host_qq) return "您不是房主，没有开始游戏的权限";
  RETURN_IF_FAILED(match->GameStart());
  return "";
}

// private
MatchId MatchManager::get_match_id(const QQ& src_qq, std::unordered_map<QQ, std::shared_ptr<Match>> match_map)
{
  auto it = match_map.find(src_qq);
  return (it == match_map.end()) ? INVALID_MATCH : it->second->get_id();
}

MatchId MatchManager::get_group_match_id(const QQ& group_qq)
{
  return get_match_id(group_qq, group_matches);
}

MatchId MatchManager::get_discuss_match_id(const QQ& discuss_qq)
{
  return get_match_id(discuss_qq, discuss_matches);
}


/* Assume usr_qq is valid. */
std::string MatchManager::AddPlayer(const MatchId& id, const QQ& usr_qq)
{
  auto it = matches.find(id);
  if (it == matches.end()) return "该房间未进行游戏，或游戏ID不存在";
  if (player_matches.find(usr_qq) != player_matches.end()) return "您已加入其它游戏，请先退出";
  auto match = it->second;

  /* Join match. */
  RETURN_IF_FAILED(match->Join(usr_qq));
  
  /* Insert into player matches map. */
  player_matches.emplace(usr_qq, match);

  return "";
}

/* Assume usr_qq is valid. */
std::string MatchManager::DeletePlayer(const QQ& usr_qq)
{
  
  auto it = player_matches.find(usr_qq);
  if (it == player_matches.end()) return "您未加入游戏，退出失败";
  auto match = it->second;

  /* Leave match. */
  RETURN_IF_FAILED(match->Leave(usr_qq));

  /* Erase from player matches map. */
  player_matches.erase(usr_qq);

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

void Match::Request(MessageIterator& msg)
{
  if (status_ != GAMING) msg.Reply("当前游戏未在进行");
  assert(has_qq(from_qq));
  
  game_->Request(qq2pid_[msg.usr_qq_], msg);
}

/* Register players, switch status and start the game 
*/
std::string Match::GameStart()
{
  if (status_ != PREPARE)
  {
    return "开始游戏失败：游戏未处于准备状态";
  }
  player_count_ = ready_qq_set_.size();
  if (player_count_ < game_->kMinPlayer)
  {
    return "开始游戏失败：玩家人数过少";
  }
  for (auto qq : ready_qq_set_)
  {
    pid2qq_.push_back(qq);
    qq2pid_.emplace(qq, pid2qq_.size() - 1);
  }
  status_ = GAMING;
  game_->StartGame();
  return "";
}

/* append new player to qq_list
*/
std::string Match::Join(const int64_t& qq)
{
  assert(!has_qq(qq));
  if (status_ != PREPARE)
  {
    return "加入游戏失败：游戏已经开始";
  }
  if (ready_qq_set_.size() >= game_->kMaxPlayer)
  {
    return "加入游戏失败：比赛人数已达到游戏上线";
  }
  ready_qq_set_.insert(qq); // add to queue
  broadcast("玩家 " + std::to_string(qq) + " 加入了游戏");
  return "";
}

/* remove player from qq_list
*/
std::string Match::Leave(const int64_t& qq)
{
  assert(has_qq(qq));
  if (status_ != PREPARE)
  {
    return "退出失败！游戏已经开始了，你跑不了了";
  }
  broadcast("玩家 " + std::to_string(qq) + " 退出了游戏");
  ready_qq_set_.erase(qq);  // remove from queue
  return "";
}