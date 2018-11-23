#include "stdafx.h"
#include "game_stage.h"
#include "log.h"

/* Constructor
*/
StageContainer::StageContainer(Game& game, StageCreatorMap&& stage_creators) :
  game_(game), stage_creators_(std::forward<StageCreatorMap>(stage_creators)) {}

/* Returns game Stage pointer with id
* if the id is main Stage id, returns the main game Stage pointer
*/
StagePtr StageContainer::Make(StageId id, GameStage& father_stage) const
{
  auto it = stage_creators_.find(id);
  if (it == stage_creators_.end())
  {
    /* Not found stage id. */
    throw ("Stage id " + std::to_string(id) + " has not been bound.");
  }
  return it->second(father_stage);
}

GameStage::GameStage() {}

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