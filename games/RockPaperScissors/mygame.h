#pragma once
#include "game.h"
#include <array>

enum StageEnum;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;

enum Choise { NONE_CHOISE, ROCK_CHOISE, SCISSORS_CHOISE, PAPER_CHOISE };

struct PlayerEnv
{
  PlayerEnv() : cur_choise_(NONE_CHOISE), win_count_(0) {}
  Choise cur_choise_;
  uint64_t win_count_;
};

struct GameEnv
{
  GameEnv() : cur_round_(0) {}
  std::vector<int64_t> PlayerScores() const;

  std::array<PlayerEnv, 2> player_envs_;
  uint64_t cur_round_;
};

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num);
std::unique_ptr<Stage<StageEnum, GameEnv>> MakeMainStage(Game<StageEnum, GameEnv>& game);
