#include "stdafx.h"
#include "game_stage.h"
#include "log.h"



GameStage::GameStage(Game& game) : game_(game) {}

GameStage::~GameStage() {}

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