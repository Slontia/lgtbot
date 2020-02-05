#include "mygame.h"
#include "game_stage.h"
#include <memory>
#include <array>

const std::string k_game_name = "猜拳游戏";
const uint64_t k_min_player = 2; /* should be larger than 1 */
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

enum StageEnum { MAIN_STAGE, ROUND_STAGE };

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num)
{
  assert(player_num == 2);
  return NULL;
}

std::unique_ptr<Stage<StageEnum, GameEnv>> MakeMainStage(Game<StageEnum, GameEnv>& game)
{
  return NULL;
}

std::vector<int64_t> GameEnv::PlayerScores() const
{
  return {};
}

class RoundStage : public AtomStage<StageEnum, GameEnv>
{
 public:
  RoundStage(const uint64_t round, Game<StageEnum, GameEnv>& game)
    : AtomStage(ROUND_STAGE, {}, game, 2), round_(round) {}
  std::string Name() const { return "第" + std::to_string(round_) + "回合"; }

 private:
   const uint64_t round_;
};

class MainStage : public CompStage<StageEnum, GameEnv>
{
  MainStage(Game<StageEnum, GameEnv>&& game)
    : CompStage(MAIN_STAGE, {}, game, std::make_unique<RoundStage>(1, game)) {}

  virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) const override
  {

  }
  std::string Name() const { return "游戏"; }
};


