#include "game_options.h"
#include "game_stage.h"
#include "msg_checker.h"
#include "dllmain.h"
#include "resource.h"
#include "game_options.h"
#include <memory>
#include <array>
#include <functional>
#include "resource_loader.h"
const std::string k_game_name = "猜拳游戏";
const uint64_t k_min_player = 2; /* should be larger than 1 */
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

bool GameOption::IsValidPlayerNum(const uint64_t player_num) const
{
  return true;
}

const std::string GameOption::StatusInfo() const
{
  const auto max_win_count = GET_VALUE(胜利局数);
  return (std::stringstream() << 2 * max_win_count - 1 << "局" << max_win_count << "胜，局时" << GET_VALUE(局时) << "秒").str();
}

enum Choise { NONE_CHOISE, ROCK_CHOISE, SCISSORS_CHOISE, PAPER_CHOISE };

static std::string Choise2Str(const Choise& choise)
{
  return choise == SCISSORS_CHOISE ? "剪刀" :
    choise == ROCK_CHOISE ? "石头" :
    choise == PAPER_CHOISE ? "布" :
    "未选择";
}

class RoundStage : public SubGameStage<>
{
public:
  RoundStage(const uint64_t round, const uint32_t max_round_sec)
    : GameStage("第" + std::to_string(round) + "回合",
      {
        MakeStageCommand(this, "出拳", &RoundStage::Act_,
          AlterChecker<Choise>(std::map<std::string, Choise> {
            { "剪刀", SCISSORS_CHOISE},
            { "石头", ROCK_CHOISE },
            { "布", PAPER_CHOISE }
          }, "选择")),
      }), max_round_sec_(max_round_sec), cur_choise_{ NONE_CHOISE, NONE_CHOISE } {}

  uint64_t OnStageBegin()
  {
    Boardcast() << name_ << "开始，请私信裁判进行选择";
    return max_round_sec_;
  }

  std::optional<uint64_t> Winner() const
  {
    if (cur_choise_[0] == NONE_CHOISE && cur_choise_[1] == NONE_CHOISE) { return {}; }
    Boardcast() << "玩家" << At(0) << "：" << Choise2Str(cur_choise_[0]) << std::endl
      << "玩家" << At(1) << "：" << Choise2Str(cur_choise_[1]);
    const auto is_win = [&cur_choise = cur_choise_](const uint64_t pid)
    {
      const Choise& my_choise = cur_choise[pid];
      const Choise& oppo_choise = cur_choise[1 - pid];
      return (my_choise == PAPER_CHOISE && oppo_choise == ROCK_CHOISE) ||
        (my_choise == ROCK_CHOISE && oppo_choise == SCISSORS_CHOISE) ||
        (my_choise == SCISSORS_CHOISE && oppo_choise == PAPER_CHOISE) ||
        oppo_choise == NONE_CHOISE;
    };
    if (is_win(0)) { return 0; }
    else if (is_win(1)) { return 1; }
    return {};
  }

private:
  bool Act_(const uint64_t pid, const bool is_public, const reply_type reply, Choise choise)
  {
    if (is_public)
    {
      reply() << "请私信裁判选择，公开选择无效";
      return false;
    }
    Choise& cur_choise = cur_choise_[pid];
    if (cur_choise != NONE_CHOISE)
    {
      reply() << "您已经进行过选择了";
      return false;
    }
    cur_choise = choise;
    reply() << "选择成功";
    return cur_choise_[0] != NONE_CHOISE && cur_choise_[1] != NONE_CHOISE;
  }

  const uint32_t max_round_sec_;
  std::array<Choise, 2> cur_choise_;
};

class MainStage : public MainGameStage<RoundStage>
{
public:
  MainStage(const GameOption& option) : GameStage("", {}),
    k_max_win_count_(option.GET_VALUE(胜利局数)), k_max_round_sec_(option.GET_VALUE(局时)), round_(1), win_count_{ 0, 0 } {}

  virtual VariantSubStage OnStageBegin() override
  {
    return std::make_unique<RoundStage>(1, k_max_round_sec_);
  }

  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const bool is_timeout) override
  {
    std::optional<uint64_t> winner = sub_stage.Winner();
    if (is_timeout && !winner.has_value())
    {
      Boardcast() << "双方无响应";
      BreakOff();
      return {};
    }
    HandleRoundResult_(winner);
    if (win_count_[0] == k_max_win_count_ || win_count_[1] == k_max_win_count_) { return {}; }
    return std::make_unique<RoundStage>(++round_, k_max_round_sec_);
  }

  int64_t PlayerScore(const uint64_t pid) const { return (win_count_[pid] == k_max_win_count_) ? 10 : -10; }

private:
  void HandleRoundResult_(const std::optional<uint64_t>& winner)
  {
    auto boardcast = Boardcast();
    const auto on_win = [this, &boardcast, &win_count = win_count_](const uint64_t pid)
    {
      boardcast << "玩家" << At(pid) << "胜利" << std::endl;
      ++win_count[pid];
    };
    if (winner.has_value()) { on_win(*winner); }
    else { boardcast << "平局" << std::endl; }
    boardcast << "目前比分：" << std::endl;
    boardcast << At(0) << " " << win_count_[0] << " - " << win_count_[1] << " " << At(1);
  }

  const uint32_t k_max_win_count_;
  const uint32_t k_max_round_sec_;
  uint64_t round_;
  std::array<uint64_t, 2> win_count_;
};

const char* Rule()
{
  static const std::string rule = LoadText(IDR_TEXT1_RULE, TEXT("Text"));
  return rule.c_str();
}

std::unique_ptr<MainStageBase> MakeMainStage(const uint64_t player_num, const GameOption& options)
{
  assert(player_num == 2);
  return std::make_unique<MainStage>(options);
}
