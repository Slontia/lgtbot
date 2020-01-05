#pragma once
#include "game.h"

enum StageEnum {AAA};

struct GameEnv
{
  std::vector<int64_t> PlayerScores() const;
};

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num);
std::unique_ptr<Stage<StageEnum, GameEnv>> MakeMainStage();
