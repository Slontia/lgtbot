#pragma once

#include <map>
#include <functional>
#include <memory>
#include <cassert>
#include <string>

#include "log.h"
#include "timer.h"

/* Two ways to end up a game Stage:
 * 1. Time up
 * 2. Request() returns true
 */

// game data stored here

class Game;
class GamePlayer;
class MessageIterator;

typedef std::string StageId;

class GameStage
{
protected:
  Game& game_;

private:
  enum { NOT_STARTED, IN_PROGRESS, OVER } status_;

public:
  const std::string                       name_ = "default";

  /* Constructor. */
  GameStage(Game& game, const std::string&);

  /* Destructor. */
  virtual ~GameStage();

  virtual void start_up();
  virtual void end_up();
  bool is_in_progress() const;
  bool is_over() const;

  /* Triggered when player send messages. */
  virtual bool                            Request(const uint32_t& pid, MessageIterator& msg) = 0;

protected:
  /* Triggered when Stage beginning. */
  virtual void                            Start() = 0;

  /* Triggered when Stage over. */
  virtual void                            Over() = 0;


  /* Send msg to all player. */
  void Broadcast(const std::string& msg) const;

  void Broadcast(const uint32_t& pid, const std::string& msg) const;

  virtual void OperatePlayer(std::function<void(GamePlayer&)> f);

  GamePlayer& get_player(const uint32_t& pid);

  std::unique_ptr<GameStage> MakeSubstage(const StageId& id, GameStage& father_stage) const;
};

template <int TimeSec>
class TimerStage : public GameStage
{
public:
  TimerStage(Game& game, const std::string& name) : GameStage(game, name)
  {

  }

  virtual ~TimerStage()
  {
    LOG_INFO("AtomStage desturct.");
  }

  virtual void start_up()
  {
    game_.timer_.push_handle_to_stack([this]() -> bool
    {
      end_up();
      return true;
    });
    GameStage::start_up();
    if (TimeSec > 0) game_.timer_.Time(TimeSec);
  }

  virtual void end_up()
  {
    game_.timer_.Terminate();
    GameStage::end_up();
    game_.timer_.pop();
  }
};

class CompStage : public GameStage
{
private:
  StageId                                 subid_; // substage不存储id，无法只凭substage判断当前处于什么状态
  std::unique_ptr<GameStage>              substage_;

public:
  CompStage(Game& game, const std::string& name);

  virtual bool                            TimerCallback() = 0;
  virtual void start_up();
  virtual void end_up();

  virtual ~CompStage();

protected:
  /* Jump to next Stage with id.
   * Failed when substage is running or id does not exist
  */
  bool SwitchSubstage(const StageId& id);

  /* Pass request to substage, check whether substage over or not
  */
  bool PassRequest(const int32_t& pid, MessageIterator& msg);
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
