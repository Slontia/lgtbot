#include "stdafx.h"
#include "game_stage.h"
#include "log.h"
#include "game.h"

GameStage::GameStage(Game& game) : game_(game) {}

GameStage::~GameStage() {}

/* Send msg to a specific player. */
void GameStage::Reply(uint32_t pid, std::string msg) const
{
  game_.Reply(pid, msg);
}

/* Send msg to all player. */
void GameStage::Broadcast(std::string msg) const
{
  game_.Broadcast(msg);
}

void GameStage::start_up()
{
  assert(status_ == NOT_STARTED);
  status_ = IN_PROGRESS;
  Start();
}

void GameStage::end_up()
{
  assert(status_ == IN_PROGRESS);
  status_ = OVER;
  Over();
}

bool GameStage::is_in_progress() const
{
  return status_ == IN_PROGRESS;
}

bool GameStage::is_over() const
{
  return status_ == OVER;
}