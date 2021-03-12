#include "game_framework/game_stage.h"
#include "utility/msg_checker.h"
#include "game_framework/game_main.h"
#include <memory>
#include <array>
#include <functional>

const std::string k_game_name = "因数游戏";
const uint64_t k_max_player = 20;

static bool NeedEliminated(const uint64_t round, const GameOption& option)
{
  return round >= option.GET_VALUE(淘汰回合) && (round - option.GET_VALUE(淘汰回合)) % option.GET_VALUE(淘汰间隔) == 0;
}

const std::string GameOption::StatusInfo() const
{
  std::stringstream ss;
  ss << "每回合" << GET_VALUE(局时) << "秒，"
     << "从第" << GET_VALUE(淘汰回合) << "回合开始，每隔" << GET_VALUE(淘汰间隔) << "回合淘汰分数最末尾的玩家，"
     << "玩家可预测因数范围为1~" << GET_VALUE(最大数字);
  return ss.str();
}

class RoundStage : public SubGameStage<>
{
public:
  RoundStage(const GameOption& option, const uint64_t round, const std::vector<bool>& eliminated)
    : GameStage("第" + std::to_string(round) + "回合",
      {
        MakeStageCommand(this, "预测因数", &RoundStage::Guess_, ArithChecker<uint32_t, 1, 9999>("选择")),
      }), option_(option), eliminated_(eliminated), round_(round), guessed_factors_(option.PlayerNum(), 0) {}

  uint64_t OnStageBegin()
  {
    auto sender = Boardcast();
    sender << name_ << "开始，请私信裁判猜测因数";
    if (NeedEliminated(round_, option_)) { sender << "（本回合需淘汰分数末尾玩家）"; }
    return option_.GET_VALUE(局时);
  }

  const std::vector<uint32_t>& GuessedFactors() const { return guessed_factors_; }

private:
  AtomStageErrCode Guess_(const uint64_t pid, const bool is_public, const replier_t reply, const uint32_t factor)
  {
    if (eliminated_[pid])
    {
      reply() << "不好意思，您已经被淘汰，无法选择数字";
      return FAILED;
    }
    if (is_public)
    {
      reply() << "请私信裁判猜测，不要暴露自己的数字哦~";
      return FAILED;
    }
    if (factor == 0 || factor > option_.GET_VALUE(最大数字))
    {
      reply() << "非法的因数，合法的范围为：1~" << option_.GET_VALUE(最大数字);
      return FAILED;
    }
    if (const uint64_t old_factor = std::exchange(guessed_factors_[pid], factor); old_factor == 0)
    {
      reply() << "猜测成功：您猜测的数字为" << factor;
    }
    else
    {
      reply() << "修改猜测成功，旧猜测数字为" << old_factor << "，当前猜测数字为" << factor;
    }
    return CanOver_() ? CHECKOUT : OK;
  }

  bool CanOver_() const
  {
    bool can_over = true;
    for (uint64_t pid = 0; pid < option_.PlayerNum(); ++ pid)
    {
      can_over &= (eliminated_[pid] || guessed_factors_[pid] != 0);
    }
    return can_over;
  }

  const GameOption& option_;
  const std::vector<bool> eliminated_;
  const uint64_t round_;
  std::vector<uint32_t> guessed_factors_;
};

class MainStage : public MainGameStage<RoundStage>
{
public:
  MainStage(const GameOption& option)
    : option_(option), round_(1), scores_(option.PlayerNum(), 0), eliminated_(option.PlayerNum(), false), factor_pool_(option.PlayerNum()) {}

  virtual VariantSubStage OnStageBegin() override
  {
    return std::make_unique<RoundStage>(option_, 1, eliminated_);
  }

  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const bool is_timeout) override
  {
    auto sender = Boardcast();
    const uint64_t sum = SumPool_(sender, sub_stage.GuessedFactors());
    AddScore_(sender, sum);
    if (NeedEliminated(round_, option_) && (Eliminate_(sender), SurviveCount_() == 1 || round_ == option_.GET_VALUE(最大回合)))
    {
      return {};
    }
    return std::make_unique<RoundStage>(option_, ++round_, eliminated_);
  }

  int64_t PlayerScore(const uint64_t pid) const { return scores_[pid]; }

private:
  uint64_t SumPool_(MsgSenderWrapper<MsgSenderForGame>& sender, const std::vector<uint32_t>& guessed_factors)
  {
    assert(guessed_factors.size() == option_.PlayerNum());
    uint64_t sum = 0;
    sender << "回合结束，各玩家预测池情况（中括号内为本回合预测）：\n";
    for (uint64_t pid = 0; pid < option_.PlayerNum(); ++ pid)
    {
      sender << AtMsg(pid) << "：";
      if (eliminated_[pid])
      {
        sender << "（已淘汰）\n";
        continue;
      }
      for (const uint32_t factor : factor_pool_[pid])
      {
        sum += factor;
        sender << factor << " ";
      }
      if (guessed_factors[pid] == 0)
      {
        sender << "[超时未决定]\n";
      }
      else
      {
        sender << "[" << guessed_factors[pid] << "]\n";
        sum += guessed_factors[pid];
        factor_pool_[pid].emplace_back(guessed_factors[pid]);
      }
    }
    sender << "总和为" << sum;
    return sum;
  }

  void AddScore_(MsgSenderWrapper<MsgSenderForGame>& sender, const uint64_t sum)
  {
    sender << "\n\n得分情况：";
    for (uint64_t pid = 0; pid < option_.PlayerNum(); ++ pid)
    {
      sender << "\n" << AtMsg(pid) << "：" << scores_[pid];
      if (eliminated_[pid])
      {
        sender << "（已淘汰）";
        continue;
      }
      for (auto it = factor_pool_[pid].begin(); it != factor_pool_[pid].end(); /* do nothing */)
      {
        const auto& factor = *it;
        if (sum % factor == 0)
        {
          sender << " + " << factor;
          scores_[pid] += factor;
          it = factor_pool_[pid].erase(it);
        }
        else
        {
          ++it;
        }
      }
      sender << " => " << scores_[pid];
    }

    sender << "\n\n当前各玩家预测池情况：\n";
    for (uint64_t pid = 0; pid < option_.PlayerNum(); ++ pid)
    {
      sender << AtMsg(pid) << "：";
      if (eliminated_[pid])
      {
        sender << "（已淘汰）\n";
        continue;
      }
      if (factor_pool_[pid].empty())
      {
        sender << "（空）\n";
        continue;
      }
      for (const uint32_t factor : factor_pool_[pid])
      {
        sender << factor << " ";
      }
      sender << "\n";
    }
  }

  void Eliminate_(MsgSenderWrapper<MsgSenderForGame>& sender)
  {
    bool has_eliminate = false;
    std::vector<uint64_t> min_score_pids;
    for (uint64_t pid = 0; pid < option_.PlayerNum(); ++ pid)
    {
      if (eliminated_[pid]) { continue; }
      if (min_score_pids.empty() || (scores_[min_score_pids.front()] == scores_[pid]))
      {
        min_score_pids.emplace_back(pid);
      }
      else if (scores_[min_score_pids.front()] > scores_[pid])
      {
        has_eliminate = true;
        min_score_pids.clear();
        min_score_pids.emplace_back(pid);
      }
      else
      {
        has_eliminate = true;
      }
    }
    if (has_eliminate)
    {
      sender << "\n本回合淘汰：";
      for (const auto pid : min_score_pids)
      {
        sender << AtMsg(pid) << " ";
        eliminated_[pid] = true;
        factor_pool_[pid].clear();
      }
    }
    else
    {
      sender << "\n无人淘汰";
    }
  }

  uint64_t SurviveCount_() const
  {
    uint64_t count = 0;
    for (const bool eliminated : eliminated_)
    {
      if (!eliminated) { ++ count; }
    }
    return count;
  }

  const GameOption& option_;
  uint64_t round_;
  std::vector<uint64_t> scores_;
  std::vector<bool> eliminated_;
  std::vector<std::list<uint32_t>> factor_pool_;
};

std::unique_ptr<MainStageBase> MakeMainStage(const replier_t& reply, const GameOption& options)
{
  if (options.PlayerNum() < 2)
  {
    reply() << "该游戏至少两人参加";
    return nullptr;
  }
  if (options.GET_VALUE(最大回合) <= options.GET_VALUE(淘汰回合))
  {
    reply() << "游戏最大回合数必须大于开始淘汰的回合数，当前设置的最大回合数" << options.GET_VALUE(最大回合) <<
      "，当前设置的开始淘汰的回合数为" << options.GET_VALUE(淘汰回合);
    return nullptr;
  }
  return std::make_unique<MainStage>(options);
}
