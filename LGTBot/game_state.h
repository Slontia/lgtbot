#pragma once

#include <iostream>
#include <set>
#include "time_trigger.h"
#include "game.h"

class Game;

// game data stored here
template <class ID>
class StateContainer
{
private:
  Game& game_;
  std::map<ID, std::function<std::shared_ptr<GameState>()>> state_creators_;
public:
  StateContainer(Game& game) : game_(game) {}

  template <class S> void Bind(ID id)
  {
    state_creators[id] = [GameState](Game& game, StateContainer& container, std::shared_ptr<GameState> superstate_ptr)
    {
      return new S(game, container, superstate_ptr);
    }
  }

  GameState Make(ID id, Game& game, StateContainer& container, std::shared_ptr<GameState> superstate_ptr)
  {
    return state_creators_[id](game, container, superstate_ptr);
  }
};


template <class ID>
class GameState
{
protected:
  Game&                               game_;
  StateContainer&                     container_;
  std::shared_ptr<GameState>          superstate_ptr_;
public:
  const ID            id_;
  const std::string   name_ = "default";
  GameState(Game& game, StateContainer& container, std::shared_ptr<GameState> superstate_ptr) :
    game(game_), container_(container), superstate_(superstate) {}

  virtual ~GameState()
  {
    Log::print_log("GameState desturct.");
  }

  /* triggered when state beginning */
  virtual void        Start() = 0;
  /* triggered when state over */
  virtual void        Over() = 0;
  /* triggered when player request */
  virtual bool        Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
};


template <class ID>
class AtomState : public GameState<ID>
{
protected:
  const int           kTimeSec = 300;
public:
  static TimeTrigger  timer_;
  AtomState(Game& game, StateContainer& container, std::shared_ptr<GameState> superstate_ptr) :
    GameState(game, container, superstate_ptr) {};

  virtual ~AtomState()
  {
    Log::print_log("AtomState desturct.");
  }

  virtual void        Start() = 0;
  virtual void        Over() = 0;
  virtual bool        Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
};


template <class ID>
class CompState : public GameState<ID>
{
private:
  ID                  subid_;
  std::shared_ptr<GameState>           substate_;
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
  CompState(Game& game, StateContainer& container, std::shared_ptr<GameState> superstate) : 
    GameState(game, container, superstate_ptr)
  {
    AtomState<ID>.timer_.PushHandle(HandleTimer);
  };

  virtual ~CompState()
  {
    Log::print_log("CompState desturct.");
  }

  virtual void        Start() = 0;
  virtual void        Over() = 0;
  virtual bool        Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
  /* triggered when substate time up */
  virtual void        HandleTimer() = 0;
};
