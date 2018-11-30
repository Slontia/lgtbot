#include "stdafx.h"
#include "game_stage.h"
#include "log.h"
#include "game.h"
#include "time_trigger.h"

GameStage::GameStage(Game& game, GameStage& main_stage) : game_(game), main_stage_(main_stage)
{
  timer.push_handle_to_stack([this]() -> bool
  {
    end_up();
    return true;
  });
}

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

void GameStage::Broadcast(uint32_t pid, std::string msg) const
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

StagePtr GameStage::MakeSubstage(const StageId& id, GameStage& father_stage)
{
  return game_.stage_container_.Make(id, father_stage);
}

void GameStage::OperatePlayer(std::function<void(GamePlayer&)> f)
{
  for (auto it = game_.players_.begin(); it != game_.players_.end(); ++it) f(**it);
}

GamePlayer& GameStage::get_player(uint32_t pid)
{
  return *game_.players_[pid];
}

CompStage::CompStage(Game& game, GameStage& main_stage) :
  GameStage(game, main_stage), subid_(), substage_(nullptr)
{
  timer.push_handle_to_stack(std::bind(&CompStage::TimerCallback, this));
}

CompStage::~CompStage()
{
  LOG_INFO("CompStage desturct.");
}

/* Jump to next Stage with id.
* Failed when substage is running or id does not exist
*/
bool CompStage::SwitchSubstage(StageId id)
{
  if (substage_ && !substage_->is_over())
  {
    throw "Switch failed: substage must be over.";
  }
  if (!(substage_ = MakeSubstage(subid_, *this))) // set new substage
  {
    subid_ = -1;
    throw "Switch failed: no such substage.";
  }
  subid_ = id;
  substage_->start_up();
  return true;
}

/* Pass request to substage, check whether substage over or not
*/
bool CompStage::PassRequest(int32_t pid, std::string msg, int32_t sub_type)
{
  if (!substage_ || substage_->is_over())
  {
    throw "Pass failed: substage must be in process.";
    return false;
  }
  if (substage_->Request(pid, msg, sub_type)) // if returns true, substage over
  {
    substage_->end_up();
    return true;
  }
  return false;
}