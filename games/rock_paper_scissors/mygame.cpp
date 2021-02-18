#include "game_framework/game_stage.h"
#include "utility/msg_checker.h"
#include "game_framework/game_main.h"
#include <memory>
#include <array>
#include <functional>

const std::string k_game_name = "猜拳游戏";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

const std::string GameOption::StatusInfo() const
{
  std::stringstream ss;
  const auto max_win_count = GET_VALUE(胜利局数);
  ss << 2 * max_win_count - 1 << "局" << max_win_count << "胜，局时" << GET_VALUE(局时) << "秒";
  return ss.str();
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
    Boardcast() << "玩家" << AtMsg(0) << "：" << Choise2Str(cur_choise_[0])
      << "\n玩家" << AtMsg(1) << "：" << Choise2Str(cur_choise_[1]);
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
  AtomStageErrCode Act_(const uint64_t pid, const bool is_public, const replier_t reply, Choise choise)
  {
    if (is_public)
    {
      reply() << "请私信裁判选择，公开选择无效";
      return FAILED;
    }
    Choise& cur_choise = cur_choise_[pid];
    if (cur_choise != NONE_CHOISE)
    {
      reply() << "您已经进行过选择了";
      return FAILED;
    }
    cur_choise = choise;
    reply() << "选择成功";
    return cur_choise_[0] != NONE_CHOISE && cur_choise_[1] != NONE_CHOISE ? CHECKOUT : OK;
  }

  const uint32_t max_round_sec_;
  std::array<Choise, 2> cur_choise_;
};

class MainStage : public MainGameStage<RoundStage>
{
public:
  MainStage(const GameOption& option) :
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
      // both score is 0, no winners
      return {};
    }
    HandleRoundResult_(winner);
    if (win_count_[0] == k_max_win_count_ || win_count_[1] == k_max_win_count_) { return {}; }
    return std::make_unique<RoundStage>(++round_, k_max_round_sec_);
  }

  int64_t PlayerScore(const uint64_t pid) const { return (win_count_[pid] == k_max_win_count_) ? 1 : 0; }

private:
  void HandleRoundResult_(const std::optional<uint64_t>& winner)
  {
    auto boardcast = Boardcast();
    const auto on_win = [this, &boardcast, &win_count = win_count_](const uint64_t pid)
    {
      boardcast << "玩家" << AtMsg(pid) << "胜利\n";
      ++win_count[pid];
    };
    if (winner.has_value()) { on_win(*winner); }
    else { boardcast << "平局\n"; }
    boardcast << "目前比分：\n";
    boardcast << AtMsg(0) << " " << win_count_[0] << " - " << win_count_[1] << " " << AtMsg(1);
  }

  const uint32_t k_max_win_count_;
  const uint32_t k_max_round_sec_;
  uint64_t round_;
  std::array<uint64_t, 2> win_count_;
};

std::unique_ptr<MainStageBase> MakeMainStage(const replier_t& reply, const GameOption& options)
{
  if (options.PlayerNum() != 2)
  {
    reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << options.PlayerNum();
    return nullptr;
  }
  return std::make_unique<MainStage>(options);
}
