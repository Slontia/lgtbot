#pragma once

#include <iostream>
#include "game.h"
#include "time_trigger.h"

// game data stored here
class GameState
{
protected:
  std::string   state_name_;
  Game          game_;
public:
                GameState(Game game, std::string name = "default");
  virtual void  Start();   // triggered when state beginning
  virtual void  Over();    // triggered when state over
  virtual bool  Request(int32_t pid, std::string msg, int32_t sub_type);
  std::string   name();
};


class AtomState : public GameState 
{
protected:
  int           time_sec_;
  TimeTrigger   timer_;
public:
                AtomState(int time, Game game, std::string name = "default"); 
  virtual void  Start();   // triggered when state beginning
  virtual void  Over();    // triggered when state over
  virtual bool  Request(int32_t pid, std::string msg, int32_t sub_type);
};

class CycleState : public GameState
{
private:
  void          ToNextRound();
protected:
  int32_t       round_ = 0;
  GameState     substate_;
  std::function<GameState()>     CreateSubstate;
  virtual bool  IsOver();
public:
                CycleState(Game game, std::string name = "default");
  template <class S>
  void  BindSubstate();  // set CreateSubstate
  virtual void  Start();   // triggered when state beginning
  virtual void  Over();    // triggered when state over
  virtual bool  Request(int32_t pid, std::string msg, int32_t sub_type);
};


template <enum StateId>
class CompState : public GameState
{
protected:
  StateId                       cur_state_id_ = 0;
  GameState                     substate_;
  std::map<StateId, std::function<GameState()>>  CreateSubstate;
  virtual bool                  IsOver();
  virtual StateId               NextStateId();  // returns -1 if over
public:
                                CompState(Game game, std::string name = "default");
  virtual void                  Start();
  virtual void                  Over();
  virtual bool                  Request(int32_t pid, std::string msg, int32_t sub_type);
};

template <enum StateId>
CompState<StateId>::CompState(Game game, std::string name) : GameState(game, name) {}

template <enum StateId>
void CompState<StateId>::Start()
{
  cur_state_id_ = 0;
  substate_ = CreateSubstate[cur_state_id_]();
  substate_.Start();
}

template <enum StateId>
void CompState<StateId>::Request(int32_t pid, std::string msg, int32_t sub_type)
{
  bool subover = substate_.Request(int32_t pid, std::string msg, int32_t sub_type);
  if (subover)
  {
    substate_.Over();
    if (IsOver())
    {
      return true;
    }
    else
    {
      cur_state_id_ = NextStateId();
      substate_ = CreateSubstate[cur_state_id_]();
      substate_.Start();
    }
  }
}