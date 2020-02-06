#include "mygame.h"
#include "game_stage.h"
#include "msg_checker.h"
#include <memory>
#include <array>
#include <functional>

const std::string k_game_name = "猜拳游戏";
const uint64_t k_min_player = 2; /* should be larger than 1 */
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

enum StageEnum { MAIN_STAGE, ROUND_STAGE };

std::vector<int64_t> GameEnv::PlayerScores() const
{
  const auto score = [player_envs = player_envs_](const uint64_t pid)
  {
    return (player_envs[pid].win_count_ == 3) ? 10 : -10;
  };
  return {score(0), score(1)};
}

class RoundStage : public AtomStage<StageEnum, GameEnv>
{
 public:
  RoundStage(const uint64_t round, Game<StageEnum, GameEnv>& game);
  std::string Act(const uint64_t pid, const bool is_public, Choise choise)
  {
    if (is_public) { return "请私信裁判选择，公开选择无效"; }
    Choise& cur_choise = game_.game_env().player_envs_[pid].cur_choise_;
    if (cur_choise != NONE_CHOISE) { return "您已经进行过选择了"; }
    cur_choise = choise;
    return "选择成功";
  }
  std::string Name() const { return "第" + std::to_string(round_) + "回合"; }

 private:
  const uint64_t round_;
};

RoundStage::RoundStage(const uint64_t round, Game<StageEnum, GameEnv>& game)
  : AtomStage(ROUND_STAGE,
    {
      MakeStageCommand(this, &RoundStage::Act, std::make_unique<AlterChecker<Choise>>(
        std::map<std::string, Choise> {
          { "剪刀", SCISSORS_CHOISE},
          { "石头", ROCK_CHOISE },
          { "布", PAPER_CHOISE }
        }, "选择")),
    }, game), round_(round) {}

class MainStage : public CompStage<StageEnum, GameEnv>
{
 public:
  MainStage(Game<StageEnum, GameEnv>& game)
    : CompStage(MAIN_STAGE, {}, game, std::make_unique<RoundStage>(1, game)), round_(1) {}

  virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) override
  {
    GameEnv& env = game_.game_env();
    return (env.player_envs_[0].win_count_ == 3 || env.player_envs_[1].win_count_ == 3) ? nullptr : std::make_unique<RoundStage>(++ round_, game_);
  }
  std::string Name() const { return ""; }

 private:
   uint64_t round_;
};

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num)
{
  assert(player_num == 2);
  return std::make_unique<GameEnv>();
}

std::unique_ptr<Stage<StageEnum, GameEnv>> MakeMainStage(Game<StageEnum, GameEnv>& game)
{
  return std::make_unique<MainStage>(game);
}
