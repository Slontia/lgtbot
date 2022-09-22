// Author:  Dva
// Date:    2022.09.25

#include <array>
#include <functional>
#include <map>
#include <memory>

#include "game_framework/game_main.h"
#include "game_framework/game_options.h"
#include "game_framework/game_stage.h"
#include "nerduel_core.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "Nerduel";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 0;

std::string GameOption::StatusInfo() const {
  std::stringstream ss;
  ss << "等式长度为" << GET_VALUE(等式长度) << "；游戏模式："
     << (GET_VALUE(游戏模式) ? "标准" : "狂野");
  return ss.str();
}

bool GameOption::ToValid(MsgSenderBase& reply) {
  if (PlayerNum() < 2) {
    reply() << "人数不足。";
    return false;
  }
  reply() << std::to_string(GET_VALUE(等式长度));
  return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== UI ==============

struct MyTable {
  MyTable(const GameOption& option)
      : option_(option),
        len(option.GET_VALUE(等式长度)),
        table_(1, option.GET_VALUE(等式长度) * 2 + 9) {
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

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(const GameOption& option, MatchBase& match);
  virtual VariantSubStage OnStageBegin() override;
  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;
  int64_t PlayerScore(const PlayerID pid) const;

  bool JudgeOver();
  void Info_() {}

  bool ended_;
  MyTable table_;
  std::array<std::string, 2> target_;
  std::array<int64_t, 2> score_;
};

class SettingStage : public SubGameStage<> {
 public:
  SettingStage(MainStage& main_stage)
      : GameStage(main_stage, "设置等式阶段",
                  MakeStageCommand("设置等式", &SettingStage::Set_, AnyArg("等式", "1+2+3=6"))) {}

  virtual void OnStageBegin() override {
    Boardcast() << "请双方设置等式。";
    main_stage().table_.SetName(PlayerName(0), PlayerName(1));
    StartTimer(200);
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
  }

 private:
  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      std::string str) {
    if (is_public) {
      reply() << "请私信设置等式。";
      return StageErrCode::FAILED;
    }
    if (str.length() != option().GET_VALUE(等式长度)) {
      reply() << "输入长度不正确。本局游戏设定的等式长度为：" +
                     std::to_string(option().GET_VALUE(等式长度));
      return StageErrCode::FAILED;
    }
    std::string err;
    bool valid = check_equation(str, err, option().GET_VALUE(游戏模式));
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
    Boardcast() << "请双方做出猜测，本回合时间限制3分钟。"
                << Markdown(main_stage().table_.ToHtml());
    StartTimer(180);
  }

 private:
  AtomReqErrCode Guess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                        std::string str) {
    if (str.length() != option().GET_VALUE(等式长度)) {
      reply() << "输入长度不正确。本局游戏设定的等式长度为：" +
                     std::to_string(option().GET_VALUE(等式长度));
      return StageErrCode::FAILED;
    }
    std::string err;
    bool valid = check_equation(str, err, option().GET_VALUE(游戏模式));
    if (!valid) {
      reply() << "猜测失败：" << err;
      return StageErrCode::FAILED;
    }
    SetReady(pid);
    auto [a, b] = get_a_b(str, main_stage().target_[pid]);
    main_stage().table_.SetEquation(str, pid, a, b);
    if (a == option().GET_VALUE(等式长度)) {
      main_stage().ended_ = true;
      main_stage().score_[pid] = 1;
    }
    return StageErrCode::OK;
  }

  AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    reply() << "TODO";
    return StageErrCode::OK;
  }
};

class RoundStage : public SubGameStage<SettingStage, GuessingStage> {
 public:
  RoundStage(MainStage& main_stage) : GameStage(main_stage, "你的回合") {}

  virtual VariantSubStage OnStageBegin() override {
    return std::make_unique<SettingStage>(main_stage());
  }

  virtual VariantSubStage NextSubStage(SettingStage& sub_stage,
                                       const CheckoutReason reason) override {
    if (main_stage().ended_) {
      return {};
    }
    return std::make_unique<GuessingStage>(main_stage());
  }

  virtual VariantSubStage NextSubStage(GuessingStage& sub_stage,
                                       const CheckoutReason reason) override {
    if (main_stage().ended_) {
      return {};
    }
    main_stage().table_.AddLine();
    return std::make_unique<GuessingStage>(main_stage());
  }

 private:
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
    : GameStage(option, match), ended_(false), table_(option) {}

MainStage::VariantSubStage MainStage::OnStageBegin() { return std::make_unique<RoundStage>(*this); }

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage,
                                                   const CheckoutReason reason) {
  if (JudgeOver()) {
    return {};
  }
  return std::make_unique<RoundStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  if (ended_) {
    Boardcast() << Markdown(table_.ToHtml());
    // Boardcast() << "游戏结束。双方的目标等式为：\n";
    // for (int i = 0; i < 2; i++) {
    //   Boardcast() << PlayerName(i) << ": " << target_[i] << "\n";
    // }
    return true;
  }
  Info_();
  return false;
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match) {
  if (!options.ToValid(reply)) {
    return nullptr;
  }
  return new MainStage(options, match);
}
