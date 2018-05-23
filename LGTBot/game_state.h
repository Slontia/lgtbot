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
  std::map<ID, std::function<std::unique_ptr<GameState>()>> state_creators_;
public:
  StateContainer(Game& game) : game_(game) {}

  template <class S> void Bind(ID id)
  {
    state_creators[id] = [GameState]()
    {
      return S(game);
    }
  }

  GameState Make(ID id)
  {
    return state_creators_[id]();
  }
};


template <class ID>
class GameState
{
protected:
  Game&               game_;
  StateContainer&     container_;
  GameState&          superstate_;
public:
  const ID            id_;
  const std::string   name_ = "default";
  GameState(Game& game, StateContainer& container, GameState& superstate) : 
    game(game_), container_(container), superstate_(superstate) {}

  virtual void        Start() = 0;   // triggered when state beginning
  virtual void        Over() = 0;    // triggered when state over
  virtual bool        Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
};


template <class ID>
class AtomState : public GameState<ID>
{
protected:
  const int           kTimeSec = 300;
public:
  static TimeTrigger  timer_;
  AtomState(Game& game, StateContainer& container, GameState& superstate) : 
    GameState(game, container, superstate) {};

  virtual void        Start() = 0;
  virtual void        Over() = 0;
  virtual bool        Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
};


template <class ID>
class CompState : public GameState<ID>
{
private:
  ID                  subid_;
  GameState           substate_;
protected:            
  void SwitchSubstate(ID id)
  {
    subid_ = id;
    substate_ = container_.Make(subid_); // set new substate
    substate_.Start();
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
  CompState(Game& game, StateContainer& container, GameState& superstate) : 
    GameState(game, container, superstate)
  {
    AtomState<ID>.timer_.PushHandle(HandleTimer);
  };

  virtual void        Start() = 0;
  virtual void        Over() = 0;
  virtual bool        Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
  virtual void        HandleTimer() = 0;
};
