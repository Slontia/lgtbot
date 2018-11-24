#include "stdafx.h"
#include "match.h"
#include "log.h"
#include "game_container.h"
#include "cqp.h"
#include "log.h"
#include "game.h"
#include "match.h"

GameContainer              game_container_;

Match::Match(std::string game_id, int64_t host_qq) : game_id_(game_id), host_qq_(host_qq), status_(PREPARE)
{
  game_ = game_container_.MakeGame(game_id_, *this); // create game
}

bool Match::has_qq(int64_t qq)
{
  return std::find(qq_list_.begin(), qq_list_.end(), qq) != qq_list_.end();
}

int Match::MsgHandle(int64_t from_qq, const char* msg, int32_t sub_type)
{
  if (has_qq(from_qq))  // qq not found
  {
    return NOT_JOINED;
  }
  game_->Request(qq2pid_[from_qq], msg, sub_type);
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
  player_count_ = qq_list_.size();
  if (player_count_ < game_->kMinPlayer)
  {
    return TOO_FEW_PLAYERS;
  }
  for (uint32_t i = 0; i < player_count_; i++) // build qq2pid_
  {
    qq2pid_[qq_list_[i]] = i;
    game_->Join(game_container_.MakePlayer(game_id_));  // make players from containter
  }
  status_ = GAMING;
  game_->StartGame();
  return SUCC;
}

/* send msg to a specific player */
void Match::Reply(uint32_t pid, std::string msg) const
{
  CQ_sendPrivateMsg(-1, qq_list_[pid], msg.c_str());
}
/* send msg to all player */
void Match::Broadcast(std::string msg) const
{
  CQ_sendGroupMsg(-1, 59057683, msg.c_str());
}

/* append new player to qq_list
*/
int Match::Join(int64_t qq)
{
  if (status_ != PREPARE)
  {
    return GAME_STARTED;
  }
  if (has_qq(qq))
  {
    return HAS_JOINED;
  }
  if (qq_list_.size() >= game_->kMaxPlayer)
  {
    return TOO_MANY_PLAYERS;
  }
  qq_list_.push_back(qq); // add to queue
  return SUCC;
}

/* remove player from qq_list
*/
int Match::Leave(int64_t qq)
{
  if (status_ != PREPARE)
  {
    return GAME_STARTED;
  }
  if (!has_qq(qq))
  {
    return NOT_JOINED;
  }
  qq_list_.erase(std::find(qq_list_.begin(), qq_list_.end(), qq));  // remove from queue
  return SUCC;
}