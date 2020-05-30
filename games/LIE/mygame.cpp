#include "game_stage.h"
#include "msg_checker.h"
#include "dllmain.h"
#include "resource.h"
#include <memory>
#include <array>
#include <functional>
#include "resource_loader.h"

const std::string k_game_name = "LIE";
const uint64_t k_min_player = 2; /* should be larger than 1 */
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

bool GameOption::IsValidPlayerNum(const uint64_t player_num) const
{
  return true;
}

const std::string GameOption::StatusInfo() const
{
  std::stringstream ss;
  if (GET_VALUE(集齐失败)) { ss << "集齐全部数字或"; }
  ss << "单个数字达到3时玩家失败";
  return ss.str();
}
const char* Rule()
{
  static std::string rule = LoadText(IDR_TEXT1_RULE, TEXT("Text"));
  return rule.c_str();
}

class NumberStage : public SubGameStage<>
{
 public:
  NumberStage(const uint64_t questioner)
    : GameStage("设置数字阶段",
      {
        MakeStageCommand(this, "设置数字", &NumberStage::Number, ArithChecker<int, 1, 6>("数字")),
      }), questioner_(questioner), num_(0) {}

  virtual uint64_t OnStageBegin() override { return 60; }
  int num() const { return num_; }

 private:
   bool Number(const uint64_t pid, const bool is_public, const reply_type reply, const int num)
   {
     if (pid != questioner_)
     {
       reply() << "[错误] 本回合您为猜测者，无法设置数字";
       return false;
     }
     if (is_public)
     {
       reply() << "[错误] 请私信裁判选择数字，公开选择无效";
       return false;
     }
     num_ = num;
     return true;
   }

   const uint64_t questioner_;
   int num_;
};

class LieStage : public SubGameStage<>
{
public:
  LieStage(const uint64_t questioner)
    : GameStage("设置数字阶段",
      {
        MakeStageCommand(this, "提问数字", &LieStage::Lie, ArithChecker<int, 1, 6>("数字")),
      }), questioner_(questioner), lie_num_(0) {}

  virtual uint64_t OnStageBegin() override { return 60; }
  int lie_num() const { return lie_num_; }

private:
  bool Lie(const uint64_t pid, const bool is_public, const reply_type reply, const int lie_num)
  {
    if (pid != questioner_)
    {
      reply() << "[错误] 本回合您为猜测者，无法提问";
      return false;
    }
    lie_num_ = lie_num;
    return true;
  }

  const uint64_t questioner_;
  int lie_num_;
};

class GuessStage : public SubGameStage<>
{
public:
  GuessStage(const uint64_t guesser)
    : GameStage("设置数字阶段",
      {
        MakeStageCommand(this, "猜测", &GuessStage::Guess, BoolChecker("质疑", "相信")),
      }), guesser_(guesser) {}

  virtual uint64_t OnStageBegin() override { return 60; }

  bool doubt() const { return doubt_; }

private:
  bool Guess(const uint64_t pid, const bool is_public, const reply_type reply, const bool doubt)
  {
    if (pid != guesser_)
    {
      reply() << "[错误] 本回合您为提问者，无法猜测";
      return false;
    }
    doubt_ = doubt;
    return true;
  }

  const uint64_t guesser_;
  bool doubt_;
};

class RoundStage : public SubGameStage<NumberStage, LieStage, GuessStage>
{
 public:
   RoundStage(const uint64_t round, const uint64_t questioner, std::array<std::array<int, 6>, 2>& player_nums)
     : GameStage("第" + std::to_string(round) + "回合", {}),
     questioner_(questioner), num_(0), lie_num_(0), player_nums_(player_nums), loser_(0) {}

   uint64_t loser() const { return loser_; }

   virtual VariantSubStage OnStageBegin() override
   {
     Boardcast() << name_ << "开始，请玩家" << At(questioner_) << "私信裁判选择数字";
     return std::make_unique<NumberStage>(questioner_);
   }

   virtual VariantSubStage NextSubStage(NumberStage& sub_stage, const bool is_timeout) override
   {
     num_ = is_timeout ? 1 : sub_stage.num();
     Tell(questioner_) << (is_timeout ? "设置超时，数字设置为默认值1，请提问数字" : "设置成功，请提问数字");
     return std::make_unique<LieStage>(questioner_);
   }

   virtual VariantSubStage NextSubStage(LieStage& sub_stage, const bool is_timeout) override
   {
     lie_num_ = is_timeout ? 1 : sub_stage.lie_num();
     if (is_timeout) { Tell(questioner_) << "提问超时，默认提问数字1"; }
     Boardcast() << "玩家" << At(questioner_) << "提问数字" << lie_num_ << "，请玩家" << At(1 - questioner_) << "相信或质疑";
     return std::make_unique<GuessStage>(1 - questioner_);
   }

   virtual VariantSubStage NextSubStage(GuessStage& sub_stage, const bool is_timeout) override
   {
     const bool doubt = is_timeout ? false : sub_stage.doubt();
     if (is_timeout) { Tell(questioner_) << "选择超时，默认为相信"; }
     const bool suc = doubt ^ (num_ == lie_num_);
     loser_ = suc ? questioner_ : 1 - questioner_;
     ++player_nums_[loser_][num_ - 1];
     auto boardcast = Boardcast();
     boardcast << "实际数字为" << num_ << "，"
       << (doubt ? "怀疑" : "相信") << (suc ? "成功" : "失败") << "，"
       << "玩家" << At(loser_) << "获得数字" << num_ << std::endl
       << "数字获得情况：" << std::endl << At(0) << "：" << At(1);
     for (int num = 1; num <= 6; ++num)
     {
       boardcast << std::endl << player_nums_[0][num - 1] << " [" << num << "] " << player_nums_[1][num - 1];
     }
     return {};
   }

private:
  const uint64_t questioner_;
  int num_;
  int lie_num_;
  std::array<std::array<int, 6>, 2>& player_nums_;
  uint64_t loser_;
};

class MainStage : public MainGameStage<RoundStage>
{
 public:
   MainStage(const GameOption& options) : GameStage("", {}), fail_if_collected_all_(options.GET_VALUE(集齐失败)), questioner_(0), round_(1), player_nums_{ {0} } {}

  virtual VariantSubStage OnStageBegin() override
  {
    return std::make_unique<RoundStage>(1, std::rand() % 2, player_nums_);
  }

  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const bool is_timeout) override
  {
    questioner_ = sub_stage.loser();
    if (JudgeOver()) { return {}; }
    return std::make_unique<RoundStage>(++round_, questioner_, player_nums_);
  }

  int64_t PlayerScore(const uint64_t pid) const
  {
    return pid == questioner_ ? -10 : 10;
  }

 private:
   bool JudgeOver()
   {
     bool has_all_num = fail_if_collected_all_;
     for (const int count : player_nums_[questioner_])
     {
       if (count >= 3) { return true; }
       else if (count == 0) { has_all_num = false; }
     }
     return has_all_num;
   }

   const bool fail_if_collected_all_;
   uint64_t questioner_;
   uint64_t round_;
   std::array<std::array<int, 6>, 2> player_nums_;
};

std::unique_ptr<MainStageBase> MakeMainStage(const uint64_t player_num, const GameOption& options)
{
  assert(player_num == 2);
  return std::make_unique<MainStage>(options);
}
