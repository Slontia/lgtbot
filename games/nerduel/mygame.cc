// Author:  Dva
// Date:    2022.09.25

#include <array>
#include <functional>
#include <map>
#include <memory>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "nerduel_core.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "Nerduel",
    .developer_ = "dva",
    .description_ = "猜测对方所设置的算式的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options) {
  if (generic_options_readonly.PlayerNum() < 2) {
    reply() << "人数不足。";
    return false;
  }
  return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("设置游戏模式",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const bool is_normal)
            {
                GET_OPTION_VALUE(game_options, 游戏模式) = is_normal;
                return NewGameMode::MULTIPLE_USERS;
            },
            BoolChecker("标准", "狂野")),
};

// ========== UI ==============

struct MyTable {
  MyTable(const CustomOptions& option, const std::string_view resource_dir)
      : resource_dir_(resource_dir),
        len(GET_OPTION_VALUE(option, 等式长度)),
        table_(1, GET_OPTION_VALUE(option, 等式长度) * 2 + 9) {
    table_.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
    table_.MergeRight(0, 0, len + 4);
    table_.Get(0, 0).SetColor("#eee");
    table_.MergeRight(0, len + 5, len + 4);
    table_.Get(0, len + 5).SetColor("#eee");
    AddLine();
  }

  void AddLine() {
    table_.AppendRow();
    int row = table_.Row() - 1;
    const int mid_col = len + 4, n = len * 2 + 9;
    for (uint32_t col = 0; col < len * 2 + 9; ++col) {
      table_.Get(row, col).SetContent(
          "<div style=\"width: 30px; height: 40px; font-size: 30px;\"></div>");
    }
    table_.Get(row, mid_col)
        .SetContent("<div style=\"width: 20px; height: 40px; font-size: 30px;\"></div>");
    table_.Get(row, mid_col).SetColor("black");
    table_.Get(row, mid_col - 4)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\"></div>");
    table_.Get(row, mid_col - 4).SetColor("b1fe9c");
    table_.Get(row, mid_col - 3)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\">A</div>");
    table_.Get(row, mid_col - 3).SetColor("b1fe9c");
    table_.Get(row, mid_col - 2)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\"></div>");
    table_.Get(row, mid_col - 2).SetColor("fafe9c");
    table_.Get(row, mid_col - 1)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\">B</div>");
    table_.Get(row, mid_col - 1).SetColor("fafe9c");

    table_.Get(row, n - 4)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\"></div>");
    table_.Get(row, n - 4).SetColor("b1fe9c");
    table_.Get(row, n - 3)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\">A</div>");
    table_.Get(row, n - 3).SetColor("b1fe9c");
    table_.Get(row, n - 2)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\"></div>");
    table_.Get(row, n - 2).SetColor("fafe9c");
    table_.Get(row, n - 1)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\">B</div>");
    table_.Get(row, n - 1).SetColor("fafe9c");
  }

  void SetEquation(const std::string& str, int side, int a, int b) {
    int left = side ? len + 5 : 0, right = side ? len * 2 + 9 : len + 4;
    int row = table_.Row() - 1;
    for (int i = 0; i < len; i++) {
      table_.Get(row, left + i)
          .SetContent("<div style=\"width: 30px; height: 40px; font-size: 30px;\">" +
                      str.substr(i, 1) + "</div>");
    }
    table_.Get(row, right - 4)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\">" +
                    std::to_string(a) + "</div>");
    table_.Get(row, right - 2)
        .SetContent("<div style=\"width: 40px; height: 40px; font-size: 30px;\">" +
                    std::to_string(b) + "</div>");
  }

  void SetName(std::string p1_name, std::string p2_name) {
    table_.Get(0, 0).SetContent(HTML_COLOR_FONT_HEADER(#345a88) " **" + p1_name +
                                "** " HTML_FONT_TAIL);
    table_.Get(0, len + 5)
        .SetContent(HTML_COLOR_FONT_HEADER(#880015) " **" + p2_name + "** " HTML_FONT_TAIL);
  }

  std::string ToHtml() { return table_.ToString(); }

 private:
  std::string Image_(const std::string_view name) const {
    return std::string("![](file:///") + resource_dir_.data() + "/" + name.data() + ".png)";
  }

  std::string_view resource_dir_;
  html::Table table_;
  int len;
};

// ========== GAME STAGES ==========

class SettingStage;
class GuessingStage;

class MainStage : public MainGameStage<SettingStage, GuessingStage> {
 public:
  MainStage(StageUtility&& utility);
  virtual void FirstStageFsm(SubStageFsmSetter setter) override;
  virtual void NextStageFsm(SettingStage& sub_stage,
                                       const CheckoutReason reason, SubStageFsmSetter setter) override;
  virtual void NextStageFsm(GuessingStage& sub_stage,
                                       const CheckoutReason reason, SubStageFsmSetter setter) override;
  int64_t PlayerScore(const PlayerID pid) const;

  bool JudgeOver();
  void Info_() {}

  bool ended_;
  MyTable table_;
  int turn_;
  std::vector<int64_t> score_;
  std::vector<std::string> target_;
  std::vector<std::string> history_;
};

class SettingStage : public SubGameStage<> {
 public:
  SettingStage(MainStage& main_stage)
      : StageFsm(main_stage, "设置等式阶段",
                  MakeStageCommand(*this, "设置等式", &SettingStage::Set_, AnyArg("等式", "1+2+3=6"))) {}

  virtual void OnStageBegin() override {
    Global().Boardcast() << "请双方设置等式。";
    Main().table_.SetName(Global().PlayerName(0), Global().PlayerName(1));
    Global().StartTimer(GAME_OPTION(时限) + 30);
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    Main().ended_ = true;
    Main().score_[pid] = -1;
    return StageErrCode::CONTINUE;
  }

  virtual CheckoutErrCode OnStageTimeout() override {
    for (int i = 0; i < 2; i++) {
      if (Global().IsReady(i) == false) {
        Global().Boardcast() << "玩家 " << Global().PlayerName(i) << " 超时未设置初始等式，游戏结束。";
        Main().ended_ = true;
        Main().score_[i] = -1;
        return StageErrCode::CHECKOUT;
      }
    }
    return StageErrCode::OK;
  }

 private:
  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      std::string str) {
    if (is_public) {
      reply() << "请私信设置等式。";
      return StageErrCode::FAILED;
    }
    if (str.length() != GAME_OPTION(等式长度)) {
      reply() << "输入长度不正确。本局游戏设定的等式长度为：" +
                     std::to_string(GAME_OPTION(等式长度));
      return StageErrCode::FAILED;
    }
    std::string err;
    bool valid = check_equation(str, err, GAME_OPTION(游戏模式));
    if (!valid) {
      reply() << "设置失败：" << err;
      return StageErrCode::FAILED;
    }
    Main().target_[!pid] = str;
    Global().SetReady(pid);
    reply() << "设置成功。";
    return StageErrCode::OK;
  }
};

class GuessingStage : public SubGameStage<> {
 public:
  GuessingStage(MainStage& main_stage)
      : StageFsm(
            main_stage, "猜测等式阶段",
            MakeStageCommand(*this, "查看当前游戏进展情况", &GuessingStage::Status_, VoidChecker("赛况")),
            MakeStageCommand(*this, "猜测", &GuessingStage::Guess_, AnyArg("等式", "1+2+3=6"))) {}

  virtual void OnStageBegin() override {
    auto limit = GAME_OPTION(时限);
    if (limit % 60 == 0) {
      Global().Boardcast() << "请双方做出猜测，本回合时间限制" << limit / 60 << "分钟。"
                  << Markdown(Main().table_.ToHtml());
    } else {
      Global().Boardcast() << "请双方做出猜测，本回合时间限制" << limit << "秒。"
                  << Markdown(Main().table_.ToHtml());
    }
    Global().StartTimer(limit);
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    Main().ended_ = true;
    Main().score_[pid] = -1;
    return StageErrCode::CONTINUE;
  }

 private:
  AtomReqErrCode Guess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                        std::string str) {
    if (str.length() != GAME_OPTION(等式长度)) {
      reply() << "输入长度不正确。本局游戏设定的等式长度为：" +
                     std::to_string(GAME_OPTION(等式长度));
      return StageErrCode::FAILED;
    }
    if (Global().IsReady(pid)) {
      reply() << "你本回合已经做出了猜测。";
      return StageErrCode::FAILED;
    }
    std::string err;
    bool valid = check_equation(str, err, GAME_OPTION(游戏模式));
    if (!valid) {
      reply() << "猜测失败：" << err;
      return StageErrCode::FAILED;
    }
    Global().SetReady(pid);
    auto [a, b] = get_a_b(str, Main().target_[pid]);
    Main().table_.SetEquation(str, pid, a, b);
    char tmp[128];
    sprintf(tmp, "%s %dA%dB\n", str.c_str(), a, b);
    Main().history_[pid] += tmp;
    if (a == GAME_OPTION(等式长度)) {
      Main().ended_ = true;
      Main().score_[pid] = 1;
    }
    reply() << "设置成功。" << err;
    return StageErrCode::OK;
  }

  AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    std::string response = "等式长度：" + std::to_string(GAME_OPTION(等式长度)) +
                           "，游戏模式：" + (GAME_OPTION(游戏模式) ? "标准" : "狂野");
    for (int i = 0; i < 2; i++) {
      response += "\n玩家 " + std::to_string(i) + ": \n" + Main().history_[i] + "\n";
    }
    reply() << response;
    return StageErrCode::OK;
  }
};

MainStage::MainStage(StageUtility&& utility)
    : StageFsm(std::move(utility)),
      ended_(false),
      table_(Global().Options(), Global().ResourceDir()),
      turn_(1),
      score_(2, 0),
      target_(2),
      history_(2) {}

void MainStage::FirstStageFsm(SubStageFsmSetter setter) {
  setter.Emplace<SettingStage>(*this);
}

void MainStage::NextStageFsm(SettingStage& sub_stage,
                                                   const CheckoutReason reason, SubStageFsmSetter setter) {
  if (JudgeOver()) {
    return;
  }
  setter.Emplace<GuessingStage>(*this);
}

void MainStage::NextStageFsm(GuessingStage& sub_stage,
                                                   const CheckoutReason reason, SubStageFsmSetter setter) {
  if (++turn_ > 50) {
    Global().Boardcast() << "回合数超过上限，游戏结束。";
    ended_ = true;
    return;
  }
  if (JudgeOver()) {
    return;
  }
  table_.AddLine();
  setter.Emplace<GuessingStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  if (ended_) {
    Global().Boardcast() << Markdown(table_.ToHtml());
    return true;
  }
  Info_();
  return false;
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

