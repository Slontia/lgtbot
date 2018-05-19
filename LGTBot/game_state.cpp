#include "stdafx.h"

GameState::GameState(Game game, std::string name) : game_(game), state_name_(name) {}

std::string GameState::name()
{
  return this->state_name_;
}

void GameState::Start() {}

void GameState::Over() {}

AtomState::AtomState(int time, Game game, std::string name) : GameState(game, name)
{
  // create timer and bind Over() to it
}

CycleState::CycleState(Game game, std::string name) : GameState(game, name) {}

template <class S>
void CycleState::BindSubstate()
{
  CreateSubstate = [GameState]() -> { return new S };
}

void CycleState::ToNextRound()
{
  round_++;
  substate_ = CreateSubstate();
  substate_.Start();
}

void CycleState::Start()
{
  ToNextRound();
}

bool CycleState::Request(int32_t pid, std::string msg, int32_t sub_type)
{
  bool subover = substate_.Request(pid, msg, sub_type);
  if (subover)
  {
    substate_.Over();
    if (IsOver())
    {
      return true;
    }
    else
    {
      ToNextRound();
    }
  }
  return false;
}
