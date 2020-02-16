#include "mygame.h"
#include "game_stage.h"
#include "msg_checker.h"
#include "dllmain.h"
#include "resource.h"
#include <memory>
#include <array>
#include <functional>
#include "resource_loader.h"

/*
1. command返回值为bool，表示阶段是否结束
*/

const std::string k_game_name = "LIE";
const uint64_t k_min_player = 2; /* should be larger than 1 */
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const char* Rule()
{
  static std::string rule = LoadText(IDR_TEXT1_RULE, TEXT("Text"));
  return rule.c_str();
}

enum StageEnum { MAIN_STAGE, ROUND_STAGE, NUMBER_STAGE, LIE_STAGE, GUESS_STAGE };

std::vector<int64_t> GameEnv::PlayerScores() const
{
  const auto score = [questioner = questioner_](const uint64_t pid)
  {
    return pid == questioner ? -10 : 10;
  };
  return {score(0), score(1)};
}

class NumberStage : public AtomStage<StageEnum, GameEnv>
{
 public:
  NumberStage(Game<StageEnum, GameEnv>& game)
    : AtomStage(game, NUMBER_STAGE, "设置数字阶段",
      {
        MakeStageCommand(this, "设置数字", &NumberStage::Number, std::make_unique<ArithChecker<int, 1, 6>>("数字")),
      })
  {}

 private:
   void Number(const uint64_t pid, const bool is_public, const std::function<void(const std::string&)> reply, const int num)
   {
     if (pid != game_.game_env().questioner_)
     {
       reply("[错误] 本回合您为猜测者，无法设置数字");
       return;
     }
     if (is_public)
     {
       reply("[错误] 请私信裁判选择数字，公开选择无效");
       return;
     }
     game_.game_env().num_ = num;
     reply("设置成功，请提问数字");
     Over();
   }
};

class LieStage : public AtomStage<StageEnum, GameEnv>
{
public:
  LieStage(Game<StageEnum, GameEnv>& game)
    : AtomStage(game, LIE_STAGE, "设置数字阶段",
      {
        MakeStageCommand(this, "提问数字", &LieStage::Lie, std::make_unique<ArithChecker<int, 1, 6>>("数字")),
      })
  {}

private:
  void Lie(const uint64_t pid, const bool is_public, const std::function<void(const std::string&)> reply, const int num)
  {
    if (pid != game_.game_env().questioner_)
    {
      reply("[错误] 本回合您为猜测者，无法提问");
      return;
    }
    game_.game_env().lie_num_ = num;
    game_.Boardcast((std::stringstream() << "玩家" << game_.At(pid) << "提问数字" << num << "，请玩家" << game_.At(1 - pid) << "相信或质疑").str());
    Over();
  }
};

class GuessStage : public AtomStage<StageEnum, GameEnv>
{
public:
  GuessStage(Game<StageEnum, GameEnv>& game)
    : AtomStage(game, GUESS_STAGE, "设置数字阶段",
      {
        MakeStageCommand(this, "猜测", &GuessStage::Guess, std::make_unique<BoolChecker>("质疑", "相信")),
      })
  {}

private:
  void Guess(const uint64_t pid, const bool is_public, const std::function<void(const std::string&)> reply, const bool doubt)
  {
    if (pid == game_.game_env().questioner_)
    {
      reply("[错误] 本回合您为提问者，无法猜测");
      return;
    }
    const bool suc = doubt ^ (game_.game_env().num_ == game_.game_env().lie_num_);
    const uint64_t loser = suc ? 1 - pid : pid;
    game_.game_env().questioner_ = loser;
    ++game_.game_env().player_nums_[loser][game_.game_env().num_ - 1];
    std::stringstream ss;
    ss << "实际数字为" << game_.game_env().num_ << "，"
      << (doubt ? "怀疑" : "相信") << (suc ? "成功" : "失败") << "，"
      << "玩家" << game_.At(loser) << "获得数字" << game_.game_env().num_ << std::endl
      << "数字获得情况：" << game_.At(0) << "：" << game_.At(1);
    for (int num = 1; num <= 6; ++num)
    {
      ss << std::endl << game_.game_env().player_nums_[0][num - 1] << " [" << num << "] " << game_.game_env().player_nums_[1][num - 1];
    }
    game_.Boardcast(ss.str());
    Over();
  }
};

class RoundStage : public CompStage<StageEnum, GameEnv>
{
 public:
   RoundStage(const uint64_t round, Game<StageEnum, GameEnv>& game)
     : CompStage(game, ROUND_STAGE, "第" + std::to_string(round) + "回合", {}, std::make_unique<NumberStage>(game))
   {
     game_.Boardcast(name_ + "开始，请玩家" + game_.At(game_.game_env().questioner_) + "私信裁判选择数字");
   }

   virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) override
   {
     if (cur_stage == NUMBER_STAGE) { return std::make_unique<LieStage>(game_); }
     else if (cur_stage == LIE_STAGE) { return std::make_unique<GuessStage>(game_); }
     assert(cur_stage == GUESS_STAGE);
     return nullptr;
   }
};

class MainStage : public CompStage<StageEnum, GameEnv>
{
 public:
  MainStage(Game<StageEnum, GameEnv>& game)
    : CompStage(game, MAIN_STAGE, "", {}, std::make_unique<RoundStage>(1, game)), round_(1) {}

  virtual std::unique_ptr<Stage<StageEnum, GameEnv>> NextSubStage(const StageEnum cur_stage) override
  {
    return JudgeOver() ? nullptr : std::make_unique<RoundStage>(++ round_, game_);
  }

 private:
   bool JudgeOver()
   {
     const std::array<int, 6>& nums = game_.game_env().player_nums_[game_.game_env().questioner_];
     if (nums[game_.game_env().num_ - 1] >= 3) { return true; }
     for (const int num : nums)
     {
       if (num == 0) { return false; }
     }
     return true;
   }

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

GameBase* __cdecl NewGame(void* const match, const uint64_t player_num)
{
  if (player_num < k_min_player || (player_num > k_max_player && k_max_player != 0)) { return nullptr; }
  Game<StageEnum, GameEnv>* game = new Game<StageEnum, GameEnv>(match, MakeGameEnv(player_num));
  game->SetMainStage(MakeMainStage(*game));
  return game;
}
