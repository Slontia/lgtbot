#pragma once

#include <map>
#include <functional>
#include <memory>
#include <cassert>
#include <string>

#include "log.h"
#include "time_trigger.h"

/* Two ways to end up a game Stage:
 * 1. Time up
 * 2. Request() returns true
 */

// game data stored here

class Game;
class GameStage;

typedef int32_t StageId;

class GameStage
{
private:
  Game& game_;
  enum { NOT_STARTED, IN_PROGRESS, OVER } status_;

public:
  const std::string                       name_ = "default";

  /* Constructor. */
  GameStage(Game& game, GameStage& main_stage);

  /* Destructor. */
  virtual ~GameStage();

  virtual void start_up();

  void end_up();

  bool is_in_progress() const;

  bool is_over() const;

  /* Triggered when player send messages. */
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

protected:
  GameStage& main_stage_;

  /* Triggered when Stage beginning. */
  virtual void                            Start() = 0;

  /* Triggered when Stage over. */
  virtual void                            Over() = 0;

  /* Triggered when time up. */
  virtual bool                            TimerCallback() = 0;

  /* Send msg to a specific player. */
  void Reply(uint32_t pid, std::string msg) const;

  /* Send msg to all player. */
  void Broadcast(std::string msg) const;

  std::unique_ptr<GameStage> MakeSubstage(const StageId& id, GameStage& father_stage);
};

template <class MyGame, int T>
class TimerStage : public GameStage
{
public:
  TimerStage(Game& game, GameStage& main_stage) :
    GameStage(game, main_stage), kTimeSec(T) {}

  virtual void start_up()
  {
    GameStage::start_up();
    if (kTimeSec > 0) timer.Time(kTimeSec);
  }

  virtual ~TimerStage()
  {
    LOG_INFO("AtomStage desturct.");
  }

private:
  const int                               kTimeSec;
};

template <class MyGame>
class CompStage : public GameStage
{
private:
  int32_t                                 subid_; // substage不存储id，无法只凭substage判断当前处于什么状态
  std::unique_ptr<GameStage>              substage_;

public:
  CompStage(Game& game, GameStage& main_stage) : GameStage(game, main_stage), subid_(-1), substage_(nullptr) {}

  virtual ~CompStage()
  {
    LOG_INFO("CompStage desturct.");
  }

protected:
  /* Jump to next Stage with id.
   * Failed when substage is running or id does not exist
  */
  bool SwitchSubstage(StageId id)
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
  bool PassRequest(int32_t pid, std::string msg, int32_t sub_type)
  {
    if (!substage_ || substage_->is_over())
    {
      LOG_ERROR("Pass failed: substage must be running.");
      return false;
    }
    if (substage_->Request(pid, msg, sub_type)) // if returns true, substage over
    {
      substage_->end_up();
      return true;
    }
    return false;
  }
};

template <class Superstage>
struct SuperstageRef
{
  typedef Superstage super_type;
  Superstage& stage_;
  SuperstageRef(GameStage& stage) : stage_(dynamic_cast<Superstage&>(stage)) {}
};

template <>
struct SuperstageRef<void>
{
  typedef void super_type;
  SuperstageRef() {}
};
