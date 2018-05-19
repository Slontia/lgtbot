#pragma once

#include <iostream>
#include "game.h"
#include "time_trigger.h"

// game data stored here
class GameState
{
protected:
  std::string   state_name_;
public:
                GameState(int time, std::string name = "default");
  virtual void  Start();   // triggered when state beginning
  virtual void  Over();    // triggered when state over
  std::string   name();
  int           time_sec();
};

class AtomState : GameState 
{
protected:
  int           time_sec_;
  TimeTrigger   timer_;
public:
                AtomState(int time, std::string name = "default");  // create timer and bind Over() to it
};

class CycleState : GameState
{
private:
  void          ToNextRound();
protected:
  int32_t       round_ = 0;
  GameState     substate_;
  virtual bool  IsOver();
public:
                CycleState(int time, GameState substate, std::string name = "default");
};

class CompState : GameState
{
private:
  void                    ToNextState();
protected:
  uint32_t                cur_state_id_ = 0;
  std::vector<GameState>  substates_;
  virtual int32_t         NextStateId();  // returns -1 if over
public:
                          CompState(int time, std::vector<GameState> substates, std::string name = "default");
};