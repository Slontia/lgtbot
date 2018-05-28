#pragma once

#include <map>
#include <functional>
#include <memory>
#include "time_trigger.h"
#include "game.h"


// game data stored here

class StateContainer
{
protected:
  typedef std::shared_ptr<GameState>                              GameStatePtr;
private:
  int32_t                                       main_id_ = 0;
  Game&                                         game_;
  std::map<int32_t, std::function<GameStatePtr(GameStatePtr)>>   state_creators_;
protected:
  virtual void                                  BindCreators() = 0;

  void set_main_state_id(int32_t main_id)
  {
    main_id_ = main_id;
  }

  Game& game() const
  {
    return game_;
  }

  template <class S> void Bind(int32_t id)
  {
    state_creators[id] = [this](GameStatePtr superstate_ptr) -> GameStatePtr
    {
      return new S(this->game_, *this, superstate_ptr);
    };
  }

public:
  StateContainer(Game& game) : game_(game)
  {
    BindCreators();
  }

  GameStatePtr Make(int32_t id, GameStatePtr superstate_ptr)
  {
    return state_creators_[id](superstate_ptr);
  }

  GameStatePtr MakeMain()
  {
    return state_creators_[main_id_](nullptr);
  }
};

class GameState
{
protected:
  typedef std::shared_ptr<GameState>      GameStatePtr;
  Game&                                   game_;
  StateContainer&                         container_;
  GameStatePtr                            superstate_ptr_;
public:
  const std::string                       name_ = "default";
  /* triggered when state beginning */
  virtual void                            Start() = 0;
  /* triggered when state over */
  virtual void                            Over() = 0;
  /* triggered when player request */
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

  GameState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    game_(game), container_(container), superstate_ptr_(superstate_ptr) {}

  virtual ~GameState()
  {
    Log::print_log("GameState desturct.");
  }
};


class AtomState : public GameState
{
protected:
  const int                               kTimeSec = 300;
public:
  virtual void                            Start() = 0;
  virtual void                            Over() = 0;
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

  AtomState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    GameState(game, container, superstate_ptr)
  {
    Game::timer_.Time(kTimeSec);
  }

  virtual ~AtomState()
  {
    Log::print_log("AtomState desturct.");
  }
};


class CompState : public GameState
{
private:
  int32_t                                 subid_;
  GameStatePtr                            substate_;
public:
  virtual void                            Start() = 0;
  virtual void                            Over() = 0;
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;
  /* triggered when substate time up */
  virtual bool                            HandleTimer() = 0;

protected:
  void SwitchSubstate(int32_t id)
  {
    subid_ = id;
    substate_ = container_.Make(subid_, (GameStatePtr) this); // set new substate
    substate_->Start();
  }

  bool PassRequest(int32_t pid, std::string msg, int32_t sub_type)
  {
    if (substate_->Request(pid, msg, sub_type))
    {
      substate_->Over();
      return true;
    }
    return false;
  }

public:
  CompState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    GameState(game, container, superstate_ptr)
  {
    Game::timer_.push_handle_to_stack(std::bind(&CompState::HandleTimer, this));
  };

  virtual ~CompState()
  {
    Log::print_log("CompState desturct.");
  }
};
