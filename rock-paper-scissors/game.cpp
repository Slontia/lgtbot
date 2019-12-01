#include "stdafx.h"
#include "game.h"
#include "timer.h"
#include "callbacks.h"

/* Constructor
*/
StageContainer::StageContainer(Game& game, StageCreatorMap&& stage_creators) :
  game_(game), stage_creators_(std::forward<StageCreatorMap>(stage_creators)) {}

/* Returns game Stage pointer with id
* if the id is main Stage id, returns the main game Stage pointer
*/
StagePtr StageContainer::Make(const StageId& id, GameStage& father_stage) const
{
  auto it = stage_creators_.find(id);
  if (it == stage_creators_.end())
  {
    /* Not found stage id. */
    throw "Stage id " + id + " has not been bound.";
  }
  return it->second(id, father_stage);
}

//std::unique_ptr<sql::Statement> state(sql::mysql::get_mysql_driver_instance()
//                                      ->connect("tcp://127.0.0.1:3306", "root", "")
//                                      ->createStatement());
