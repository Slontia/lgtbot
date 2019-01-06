#include "stdafx.h"
#include "game_stage.h"
#include "log.h"
#include "game.h"
#include "time_trigger.h"
#include "match.h"

GameStage::GameStage(Game& game, const std::string& name) : game_(game), name_(name), status_(NOT_STARTED)
{
  timer.push_handle_to_stack([this]() -> bool
  {
    end_up();
    return true;
  });
}

GameStage::~GameStage() {}


/* Send msg to all player. */
void GameStage::Broadcast(const std::string& msg) const
{
  game_.Broadcast(msg);
}

void GameStage::Broadcast(const uint32_t& pid, const std::string& msg) const
{
  // 不要这个函数了，因为at可以穿插在文本中
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

StagePtr GameStage::MakeSubstage(const StageId& id, GameStage& father_stage) const
{
  return game_.stage_container_.Make(id, father_stage);
}

void GameStage::OperatePlayer(std::function<void(GamePlayer&)> f)
{
  for (auto it = game_.players_.begin(); it != game_.players_.end(); ++it) f(**it);
}

GamePlayer& GameStage::get_player(const uint32_t& pid)
{
  return *game_.players_[pid];
}

CompStage::CompStage(Game& game, const std::string& name) :
  GameStage(game, name), subid_(), substage_(nullptr)
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
bool CompStage::SwitchSubstage(const StageId& id)
{
  if (substage_ && !substage_->is_over())
  {
    throw "Switch failed: substage must be over.";
  }
  if (!(substage_ = MakeSubstage(id, *this))) // set new substage
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
bool CompStage::PassRequest(const int32_t& pid, MessageIterator& msg)
{
  if (!substage_ || substage_->is_over())
  {
    throw "Pass failed: substage must be in process.";
    return false;
  }
  if (substage_->Request(pid, msg)) // if returns true, substage over
  {
    substage_->end_up();
    return true;
  }
  return false;
}