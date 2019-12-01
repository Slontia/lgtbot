#include "stdafx.h"
#include "game_stage.h"
#include "game.h"
#include "timer.h"

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

void CompStage::start_up()
{
  game_.timer_.push_handle_to_stack([this]() -> bool
  {
    bool over = TimerCallback();
    if (over) end_up();
    return over;
  });
  GameStage::start_up();
}

void CompStage::end_up()
{
  GameStage::end_up();
  game_.timer_.pop();
}
