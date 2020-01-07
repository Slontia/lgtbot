#include "mygame.h"
#include "game_stage.h"
#include <memory>

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num)
{
  return NULL;
}
std::unique_ptr<Stage<StageEnum, GameEnv>> MakeMainStage()
{
  return NULL;
}
std::vector<int64_t> GameEnv::PlayerScores() const
{
  return {};
}
