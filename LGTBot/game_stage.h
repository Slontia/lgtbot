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
class GamePlayer;

typedef std::string StageId;

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

  virtual void end_up();

  bool is_in_progress() const;

  bool is_over() const;

  /* Triggered when player send messages. */
  virtual bool                            Request(uint32_t pid, std::string msg, int32_t sub_type) = 0;

protected:
  GameStage& main_stage_;
  /* Triggered when Stage beginning. */
  virtual void                            Start() = 0;

  /* Triggered when Stage over. */
  virtual void                            Over() = 0;

  /* Send msg to a specific player. */
  void Reply(uint32_t pid, std::string msg) const;

  /* Send msg to all player. */
  void Broadcast(std::string msg) const;

  void Broadcast(uint32_t pid, std::string msg) const;

  virtual void OperatePlayer(std::function<void(GamePlayer&)> f);

  GamePlayer& get_player(uint32_t pid);

  std::unique_ptr<GameStage> MakeSubstage(const StageId& id, GameStage& father_stage);
};

template <int TimeSec>
class TimerStage : public GameStage
{
public:
  TimerStage(Game& game, GameStage& main_stage) : GameStage(game, main_stage) {}

  virtual ~TimerStage()
  {
    LOG_INFO("AtomStage desturct.");
  }

  virtual void start_up()
  {
    GameStage::start_up();
    if (TimeSec > 0) timer.Time(TimeSec);
  }

  virtual void end_up()
  {
    timer.Terminate();
    GameStage::end_up();
  }
};

class CompStage : public GameStage
{
private:
  StageId                                 subid_; // substage不存储id，无法只凭substage判断当前处于什么状态
  std::unique_ptr<GameStage>              substage_;

public:
  CompStage(Game& game, GameStage& main_stage);

  virtual bool                            TimerCallback() = 0;

  virtual ~CompStage();

protected:
  /* Jump to next Stage with id.
   * Failed when substage is running or id does not exist
  */
  bool SwitchSubstage(StageId id);

  /* Pass request to substage, check whether substage over or not
  */
  bool PassRequest(int32_t pid, std::string msg, int32_t sub_type);
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
