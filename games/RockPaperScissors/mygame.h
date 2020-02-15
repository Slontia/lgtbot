#pragma once
#include "game.h"
#include "game_main.h"
#include <array>

enum StageEnum;

enum Choise { NONE_CHOISE, ROCK_CHOISE, SCISSORS_CHOISE, PAPER_CHOISE };

struct PlayerEnv
{
  PlayerEnv() : cur_choise_(NONE_CHOISE), win_count_(0) {}
  Choise cur_choise_;
  uint64_t win_count_;
};

struct GameEnv
{
  GameEnv() {}
  std::vector<int64_t> PlayerScores() const;
  std::array<PlayerEnv, 2> player_envs_;
};
