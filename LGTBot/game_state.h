#pragma once

#include <map>
#include <functional>
#include <memory>
#include "time_trigger.h"
#include "game.h"


template <class ID> class GameState;

// game data stored here

template <class ID>
class StateContainer
{
private:
  typedef std::shared_ptr<GameState<ID>>        GameStatePtr;
  Game&                                         game_;
  std::map<ID, std::function<GameStatePtr()>>   state_creators_;

public:
  StateContainer(Game& game) : game_(game) {}

  template <class S> void Bind(ID id)
  {
    state_creators[id] = [game_](GameStatePtr superstate_ptr) -> GameStatePtr
    {
      return new S(game_, *this, superstate_ptr);
    };
  }

  GameStatePtr Make(ID id, GameStatePtr superstate_ptr)
  {
    return state_creators_[id](superstate_ptr);
  }
};


template <class ID>
class GameState
{
protected:
  typedef std::shared_ptr<GameState<ID>> GameStatePtr;
  Game&                                   game_;
  StateContainer<ID>&                     container_;
  GameStatePtr                            superstate_ptr_;
public:
  const ID                                id_;
  const std::string                       name_ = "default";
  /* triggered when state beginning */
  virtual void                            Start() = 0;
  /* triggered when state over */
  virtual void                            Over() = 0;
  /* triggered when player request */
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

  GameState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    game(game_), container_(container), superstate_(superstate) {}

  virtual ~GameState()
  {
    Log::print_log("GameState desturct.");
  }
};


template <class ID>
class AtomState : public GameState<ID>
{
protected:
  const int                               kTimeSec = 300;
public:
  static TimeTrigger                      timer_;
  virtual void                            Start() = 0;
  virtual void                            Over() = 0;
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

  AtomState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    GameState(game, container, superstate_ptr)
  {
    timer_.Time(kTimeSec);
  }

  virtual ~AtomState()
  {
    Log::print_log("AtomState desturct.");
  }
};


template <class ID>
class CompState : public GameState<ID>
{
private:
  ID                                      subid_;
  GameStatePtr                            substate_;
public:
  virtual void                            Start() = 0;
  virtual void                            Over() = 0;
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
  /* triggered when substate time up */
  virtual void                            HandleTimer() = 0;

protected:
  void SwitchSubstate(ID id)
  {
    subid_ = id;
    substate_ = container_.Make(subid_); // set new substate
    substate_->Start();
  }

  bool PassRequest(int32_t pid, std::string msg, int32_t sub_type)
  {
    if (substate_.Request(pid, msg, sub_type))
    {
      substate_.Over();
      return true;
    }
    return false;
  }

public:
  CompState(Game& game, StateContainer& container, GameStatePtr superstate) :
    GameState(game, container, superstate_ptr)
  {
    AtomState<ID>.timer_.push_handle_to_stack(HandleTimer);
  };

  virtual ~CompState()
  {
    Log::print_log("CompState desturct.");
  }
};
