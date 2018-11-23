#pragma once

#include <map>
#include <functional>
#include <memory>
#include <cassert>
#include <string>

#include "time_trigger.h"

/* Two ways to end up a game Stage:
 * 1. Time up
 * 2. Request() returns true
 */

// game data stored here

class Game;
class GameStage;

typedef int32_t StageId;
typedef std::unique_ptr<GameStage> StagePtr;
typedef std::function<StagePtr(GameStage&)> StageCreator;
typedef std::map<StageId, StageCreator> StageCreatorMap;

class StageContainer
{
private:
  Game& game_;
  const std::map<StageId, StageCreator>   stage_creators_;
public:
  /* Constructor
  */
  StageContainer(Game& game, StageCreatorMap&& stage_creators);

  /* Returns game Stage pointer with id
  * if the id is main Stage id, returns the main game Stage pointer
  */
  StagePtr Make(StageId id, GameStage& father_stage) const;

private:
  template <class S>
  static StageCreator get_creator()
  {
    return [game_](GameStage& father_stage) -> StagePtr
    {
      /* Cast GameStage& to FatherStage&. */
      return std::is_void_v<S::super_type> ?
        std::make_unique<S>(game_) :
        std::make_unique<S>(game_, father_stage);
    };
  }
};    

class GameStage
{
protected:
  enum { NOT_STARTED, IN_PROGRESS, OVER } status_;

public:
  const std::string                       name_ = "default";

  /* Constructor. */
  GameStage();

  /* Destructor. */
  virtual ~GameStage();

  /* Triggered when Stage beginning. */
  virtual void                            Start() = 0;

  /* Triggered when Stage over. */
  virtual void                            Over() = 0;

  /* Triggered when player send messages. */
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

  /* Triggered when time up. */
  virtual bool                            TimerCallback() = 0;

  void start_up();

  void end_up();

  bool is_in_progress() const;

  bool is_over() const;
};

template <class MyGame, class SuperStage>
class DependStage : public GameStage
{
public:
  typedef SuperStage super_type;

  /* Constructor. */
  DependStage(Game& game, GameStage& super_stage) :
    GameStage(), game_(dynamic_cast<MyGame&>(game)), super_stage_(std::dynamic_pointer_cast<SuperStage>(super_stage)) {}

protected:
  MyGame game_;
  SuperStage& super_stage_;
};

template <class MyGame>
class DependStage<MyGame, void> : public GameStage
{
public:
  typedef void super_type;

  /* Constructor. */
  DependStage(Game& game) : GameStage(), game_(dynamic_cast<MyGame&>(game)) {}

protected:
  MyGame game_;
};

template <class MyGame, class SuperStage = void>
class TimerStage : public DependStage<MyGame, SuperStage>
{
public:
  TimerStage(Game& game) : DependStage<SuperStage>(game)
  {
    Game::timer_.Time(kTimeSec);
  }

  TimerStage(Game& game, GameStage& super_stage) : DependStage<SuperStage>(game, super_stage)
  {
    Game::timer_.Time(kTimeSec);
  }

  virtual ~TimerStage()
  {
    LOG_INFO("AtomStage desturct.");
  }

protected:
  const int                               kTimeSec = 300;
};

template <class MyGame, class SuperStage = void>
class CompStage : public DependStage<MyGame, SuperStage>
{
private:
  int32_t                                 subid_; // subStage不存储id，无法只凭subStage判断当前处于什么状态
  std::unique_ptr<GameStage>              subStage_;

public:
  CompStage(Game& game) :
    DependStage<SuperStage>(game), subid_(-1), subStage_(nullptr) {}

  CompStage(Game& game, GameStage& super_stage) :
    DependStage<SuperStage>(game, super_stage), subid_(-1), subStage_(nullptr) {}

  virtual ~CompStage()
  {
    LOG_INFO("CompStage desturct.");
  }

protected:
  /* Jump to next Stage with id.
   * Failed when subStage is running or id does not exist
  */
  bool SwitchSubStage(StageId id)
  {
    if (subStage_ && !subStage_->is_over())
    {
      throw "Switch failed: subStage must be over.";
    }
    if (!(subStage_ = game_.state_container_.Make(subid_, *this))) // set new subStage
    {
      subid_ = -1;
      throw "Switch failed: no such subStage.";
    }
    subid_ = id;
    subStage_->start_up();
    return true;
  }

  /* Pass request to subStage, check whether subStage over or not
  */
  bool PassRequest(int32_t pid, std::string msg, int32_t sub_type)
  {
    if (!subStage_ || subStage_->is_over())
    {
      LOG_ERROR("Pass failed: subStage must be running.");
      return false;
    }
    if (subStage_->Request(pid, msg, sub_type)) // if returns true, subStage over
    {
      subStage_->end_up();
      return true;
    }
    return false;
  }
};

