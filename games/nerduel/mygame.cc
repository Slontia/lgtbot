// Author:  Dva
// Date:    2022.09.25

#include <array>
#include <functional>
#include <map>
#include <memory>

#include "game_framework/game_stage.h"
#include "nerduel_core.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "Nerduel";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 1;
const std::string k_developer = "dva";
const std::string k_description = "猜测对方所设置的算式的游戏";

std::string GameOption::StatusInfo() const {
  std::stringstream ss;
  ss << "等式长度为" << GET_VALUE(等式长度) << "；游戏模式："
     << (GET_VALUE(游戏模式) ? "标准" : "狂野") << "；每回合时间限制：" << GET_VALUE(时限) << "秒";
  return ss.str();
}

bool GameOption::ToValid(MsgSenderBase& reply) {
  if (PlayerNum() < 2) {
    reply() << "人数不足。";
    return false;
  }
  return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== UI ==============

struct MyTable {
  MyTable(const GameOption& option)
      : option_(option),
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
  std::string Image_(std::string name) const {
    return std::string("![](file://") + option_.ResourceDir() + "/" + std::move(name) + ".png)";
  }

  const GameOption& option_;
  html::Table table_;
  int len;
};

// ========== GAME STAGES ==========

class SettingStage;
class GuessingStage;

class MainStage : public MainGameStage<SettingStage, GuessingStage> {
 public:
  MainStage(const GameOption& option, MatchBase& match);
  virtual VariantSubStage OnStageBegin() override;
  virtual VariantSubStage NextSubStage(SettingStage& sub_stage,
                                       const CheckoutReason reason) override;
  virtual VariantSubStage NextSubStage(GuessingStage& sub_stage,
                                       const CheckoutReason reason) override;
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
      : GameStage(main_stage, "设置等式阶段",
                  MakeStageCommand("设置等式", &SettingStage::Set_, AnyArg("等式", "1+2+3=6"))) {}

  virtual void OnStageBegin() override {
    Boardcast() << "请双方设置等式。";
    main_stage().table_.SetName(PlayerName(0), PlayerName(1));
    StartTimer(GET_OPTION_VALUE(option(), 时限) + 30);
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    main_stage().ended_ = true;
    main_stage().score_[pid] = -1;
    return StageErrCode::CONTINUE;
  }

  virtual CheckoutErrCode OnTimeout() override {
    for (int i = 0; i < 2; i++) {
      if (IsReady(i) == false) {
        Boardcast() << "玩家 " << PlayerName(i) << " 超时未设置初始等式，游戏结束。";
        main_stage().ended_ = true;
        main_stage().score_[i] = -1;
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
    if (str.length() != GET_OPTION_VALUE(option(), 等式长度)) {
      reply() << "输入长度不正确。本局游戏设定的等式长度为：" +
                     std::to_string(GET_OPTION_VALUE(option(), 等式长度));
      return StageErrCode::FAILED;
    }
    std::string err;
    bool valid = check_equation(str, err, GET_OPTION_VALUE(option(), 游戏模式));
    if (!valid) {
      reply() << "设置失败：" << err;
      return StageErrCode::FAILED;
    }
    main_stage().target_[!pid] = str;
    SetReady(pid);
    reply() << "设置成功。";
    return StageErrCode::OK;
  }
};

class GuessingStage : public SubGameStage<> {
 public:
  GuessingStage(MainStage& main_stage)
      : GameStage(
            main_stage, "猜测等式阶段",
            MakeStageCommand("查看当前游戏进展情况", &GuessingStage::Status_, VoidChecker("赛况")),
            MakeStageCommand("猜测", &GuessingStage::Guess_, AnyArg("等式", "1+2+3=6"))) {}

  virtual void OnStageBegin() override {
    auto limit = GET_OPTION_VALUE(option(), 时限);
    if (limit % 60 == 0) {
      Boardcast() << "请双方做出猜测，本回合时间限制" << limit / 60 << "分钟。"
                  << Markdown(main_stage().table_.ToHtml());
    } else {
      Boardcast() << "请双方做出猜测，本回合时间限制" << limit << "秒。"
                  << Markdown(main_stage().table_.ToHtml());
    }
    StartTimer(limit);
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    main_stage().ended_ = true;
    main_stage().score_[pid] = -1;
    return StageErrCode::CONTINUE;
  }

 private:
  AtomReqErrCode Guess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                        std::string str) {
    if (str.length() != GET_OPTION_VALUE(option(), 等式长度)) {
      reply() << "输入长度不正确。本局游戏设定的等式长度为：" +
                     std::to_string(GET_OPTION_VALUE(option(), 等式长度));
      return StageErrCode::FAILED;
    }
    if (IsReady(pid)) {
      reply() << "你本回合已经做出了猜测。";
      return StageErrCode::FAILED;
    }
    std::string err;
    bool valid = check_equation(str, err, GET_OPTION_VALUE(option(), 游戏模式));
    if (!valid) {
      reply() << "猜测失败：" << err;
      return StageErrCode::FAILED;
    }
    SetReady(pid);
    auto [a, b] = get_a_b(str, main_stage().target_[pid]);
    main_stage().table_.SetEquation(str, pid, a, b);
    char tmp[128];
    sprintf(tmp, "%s %dA%dB\n", str.c_str(), a, b);
    main_stage().history_[pid] += tmp;
    if (a == GET_OPTION_VALUE(option(), 等式长度)) {
      main_stage().ended_ = true;
      main_stage().score_[pid] = 1;
    }
    reply() << "设置成功。" << err;
    return StageErrCode::OK;
  }

  AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    std::string response = "等式长度：" + std::to_string(GET_OPTION_VALUE(option(), 等式长度)) +
                           "，游戏模式：" + (GET_OPTION_VALUE(option(), 游戏模式) ? "标准" : "狂野");
    for (int i = 0; i < 2; i++) {
      response += "\n玩家 " + std::to_string(i) + ": \n" + main_stage().history_[i] + "\n";
    }
    reply() << response;
    return StageErrCode::OK;
  }
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
    : GameStage(option, match),
      ended_(false),
      table_(option),
      turn_(1),
      score_(2, 0),
      target_(2),
      history_(2) {}

MainStage::VariantSubStage MainStage::OnStageBegin() {
  return std::make_unique<SettingStage>(*this);
}

MainStage::VariantSubStage MainStage::NextSubStage(SettingStage& sub_stage,
                                                   const CheckoutReason reason) {
  if (JudgeOver()) {
    return {};
  }
  return std::make_unique<GuessingStage>(*this);
}

MainStage::VariantSubStage MainStage::NextSubStage(GuessingStage& sub_stage,
                                                   const CheckoutReason reason) {
  if (++turn_ > 50) {
    Boardcast() << "回合数超过上限，游戏结束。";
    ended_ = true;
    return {};
  }
  if (JudgeOver()) {
    return {};
  }
  table_.AddLine();
  return std::make_unique<GuessingStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  if (ended_) {
    Boardcast() << Markdown(table_.ToHtml());
    return true;
  }
  Info_();
  return false;
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
