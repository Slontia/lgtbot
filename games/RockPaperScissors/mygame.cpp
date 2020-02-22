#include "mygame.h"
#include "game_stage.h"
#include "msg_checker.h"
#include "dllmain.h"
#include "resource.h"
#include <memory>
#include <array>
#include <functional>
#include "resource_loader.h"

const std::string k_game_name = "猜拳游戏";
const uint64_t k_min_player = 2; /* should be larger than 1 */
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const char* Rule()
{
  static std::string rule = LoadText(IDR_TEXT1_RULE, TEXT("Text"));
  return rule.c_str();
}

enum Choise { NONE_CHOISE, ROCK_CHOISE, SCISSORS_CHOISE, PAPER_CHOISE };

static std::string Choise2Str(const Choise& choise)
{
  return choise == SCISSORS_CHOISE ? "剪刀" :
    choise == ROCK_CHOISE ? "石头" :
    choise == PAPER_CHOISE ? "布" :
    "未选择";
}

class RoundStage : public AtomStage
{
 public:
  RoundStage(const uint64_t round)
     : AtomStage("第" + std::to_string(round) + "回合",
       {
         MakeStageCommand(this, "出拳", &RoundStage::Act_, 
           std::make_unique<AlterChecker<Choise>>(std::map<std::string, Choise> {
             { "剪刀", SCISSORS_CHOISE},
             { "石头", ROCK_CHOISE },
             { "布", PAPER_CHOISE }
           }, "选择")),
       }), cur_choise_{ NONE_CHOISE, NONE_CHOISE }, winner_()
  {
    Boardcast(name_ + "开始，请私信裁判进行选择");
  }

  std::optional<uint64_t> Winner() const { return winner_; }

private:
  bool Act_(const uint64_t pid, const bool is_public, const std::function<void(const std::string&)> reply, Choise choise)
  {
    if (is_public)
    {
      reply("请私信裁判选择，公开选择无效");
      return false;
    }
    Choise& cur_choise = cur_choise_[pid];
    if (cur_choise != NONE_CHOISE)
    {
      reply("您已经进行过选择了");
      return false;
    }
    cur_choise = choise;
    reply("选择成功");
    return JudgeWinner_();
  }

  bool JudgeWinner_()
  {
    if (cur_choise_[0] == NONE_CHOISE || cur_choise_[1] == NONE_CHOISE) { return false; }
    std::stringstream ss;
    ss << "玩家" << At(0) << "：" << Choise2Str(cur_choise_[0]) << std::endl;
    ss << "玩家" << At(1) << "：" << Choise2Str(cur_choise_[1]) << std::endl;
    const auto is_win = [&cur_choise = cur_choise_](const uint64_t pid)
    {
      const Choise& my_choise = cur_choise[pid];
      const Choise& oppo_choise = cur_choise[1 - pid];
      return (my_choise == PAPER_CHOISE && oppo_choise == ROCK_CHOISE) ||
        (my_choise == ROCK_CHOISE && oppo_choise == SCISSORS_CHOISE) ||
        (my_choise == SCISSORS_CHOISE && oppo_choise == PAPER_CHOISE);
    };
    if (is_win(0)) { winner_ = 0; }
    else if (is_win(1)) { winner_ = 1; }
    return true;
  }

  std::array<Choise, 2> cur_choise_;
  std::optional<uint64_t> winner_;
};

class MainStage : public CompStage<RoundStage>
{
 public:
  MainStage() : CompStage("", {}, std::make_unique<RoundStage>(1)), round_(1), win_count_{0, 0} {}

  virtual VariantSubStage NextSubStage(RoundStage& sub_stage) override
  {
    HandleRoundResult_(sub_stage.Winner());
    if (win_count_[0] == 3 || win_count_[1] == 3) { return {}; }
    return std::make_unique<RoundStage>(++ round_);
  }

  int64_t PlayerScore(const uint64_t pid) const { return (win_count_[pid] == 3) ? 10 : -10; }

 private:
   void HandleRoundResult_(const std::optional<uint64_t>& winner)
   {
     std::stringstream ss;
     const auto on_win = [this, &ss, &win_count = win_count_](const uint64_t pid)
     {
       ss << "玩家" << At(pid) << "胜利" << std::endl;
       ++win_count[pid];
     };
     if (winner.has_value()) { on_win(*winner); }
     else { ss << "平局" << std::endl; }
     ss << "目前比分：" << std::endl;
     ss << At(0) << " " << win_count_[0] << " - " << win_count_[1] << " " << At(1);
     Boardcast(ss.str());
   }

   uint64_t round_;
   std::array<uint64_t, 2> win_count_;
};

std::pair<std::unique_ptr<Stage>, std::function<int64_t(uint64_t)>> MakeMainStage(const uint64_t player_num)
{
  assert(player_num == 2);
  std::unique_ptr<MainStage> main_stage = std::make_unique<MainStage>();
  return { static_cast<std::unique_ptr<Stage>&&>(std::move(main_stage)), std::bind(&MainStage::PlayerScore, main_stage.get(), std::placeholders::_1) };
}
