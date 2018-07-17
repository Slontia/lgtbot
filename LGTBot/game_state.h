#pragma once

#include <map>
#include <functional>
#include <memory>
#include "log.h"
#include "time_trigger.h"
#include "game.h"

/* Two ways to end up a game state:
 * 1. Time up
 * 2. Request() returns true
 */


// game data stored here

class StateContainer
{
public:
  typedef std::shared_ptr<GameState>  GameStatePtr;

  /* constructor 
   */
  StateContainer(Game& game, std::function<void()> bind_creators) : 
    game_(game), BindCreators(bind_creators)
  {
    if (BindCreators)
    {
      BindCreators();
      valid_ = true;
    }
    else
    {
      valid_ = false;
    }
  }

  /* returns game state pointer with id
   * if the id is main state id, returns the main game state pointer
  */
  GameStatePtr Make(int32_t id, GameStatePtr superstate)
  {
    if (state_creators_[id] == nullptr)
    {
      LOG_ERROR("State id " << id << " not exists.");
      return nullptr;
    }
    return state_creators_[id](superstate);
  }

  /* returns main game state
  */
  GameStatePtr MakeMain()
  {
    return state_creators_[main_id_](nullptr);
  }

  bool is_valid()
  {
    return valid_;
  }

protected:
  std::function<void()> BindCreators = nullptr;

  /* set main state id, 0 is default
  */
  void set_main_state_id(int32_t main_id)
  {
    main_id_ = main_id;
  }

  /* get game
  */
  Game& game() const
  {
    return game_;
  }

  /* bind game state S to container with id
   * if the id has been used, S will replace the old state 
  */
  template <class S> void Bind(int32_t id)
  {
    state_creators[id] = [this](GameStatePtr superstate_ptr) -> GameStatePtr
    {
      return new S(this->game_, *this, superstate_ptr);
    };
  }

private:
  int32_t                                       main_id_ = 0;
  Game&                                         game_;
  std::map<int32_t, std::function<GameStatePtr(GameStatePtr)>>   state_creators_;
  bool                                          valid_;
};    


class GameState
{
protected:
  typedef std::shared_ptr<GameState>      GameStatePtr;
  Game&                                   game_;
  StateContainer&                         container_;
  GameStatePtr                            superstate_;
  bool                                    is_over_;
public:
  const std::string                       name_ = "default";

  /* constructor
  */
  GameState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    game_(game), container_(container), superstate_(superstate_ptr), is_over_(false) {}

  /* destructor
  */
  virtual ~GameState()
  {
    LOG_INFO("GameState desturct.");
  }

  /* triggered when state beginning 
  */
  virtual void                            Start() = 0;
  
  /* triggered when state over 
  */
  virtual void                            Over() = 0;
  
  /* triggered when player send messages 
  */
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

  void end_up()
  {
    Over();
    is_over_ = true;
  }

  bool is_over()
  {
    return is_over_;
  }
};


class AtomState : public GameState
{
public:
  AtomState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    GameState(game, container, superstate_ptr)
  {
    Game::timer_.Time(kTimeSec);
  }

  virtual ~AtomState()
  {
    LOG_INFO("AtomState desturct.");
  }

  virtual void                            Start() = 0;
  virtual void                            Over() = 0;
  virtual bool                            Request(int32_t pid, std::string msg, int32_t sub_type) = 0;

protected:
  const int                               kTimeSec = 300;
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
  /* Jump to next state with id.
   * Failed when substate is running or id does not exist
  */
  bool SwitchSubstate(int32_t id)
  {
    if (!substate_->is_over())
    {
      LOG_ERROR("Switch failed: substate must be over.");
      return false;
    }
    subid_ = id;
    substate_ = container_.Make(subid_, (GameStatePtr) this); // set new substate
    if (substate_)
    {
      substate_->Start();
      return true;
    }
    LOG_ERROR("Switch failed: failed to make substate.");
    subid_ = -1;
    return false;
  }

  /* Pass request to substate, check whether substate over or not
  */
  bool PassRequest(int32_t pid, std::string msg, int32_t sub_type)
  {
    if (substate_ && substate_->Request(pid, msg, sub_type)) // if returns true, substate over
    {
      substate_->end_up();
      return true;
    }
    return false;
  }

public:
  /* constructor
  */
  CompState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    GameState(game, container, superstate_ptr), subid_(-1), substate_(nullptr)
  {
    Game::timer_.push_handle_to_stack(std::bind(&CompState::HandleTimer, this));
    /* atom also */
  };

  virtual ~CompState()
  {
    LOG_INFO("CompState desturct.");
  }
};
