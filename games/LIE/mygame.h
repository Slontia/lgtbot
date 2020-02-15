#pragma once
#include "game.h"
#include "game_main.h"
#include <array>
#include <random>

enum StageEnum;

struct GameEnv
{
  GameEnv() : player_nums_{0}, questioner_(std::rand() % 2) {}
  std::vector<int64_t> PlayerScores() const;
  std::array<std::array<uint8_t, 6>, 2> player_nums_;
  uint64_t questioner_;
  uint8_t num_;
  uint8_t lie_num_;
};
