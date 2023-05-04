// Author:  Dva
// Date:    2022.10.5

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <random>

#include "game_framework/game_stage.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "犹太人棋";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */
const uint64_t k_multiple = 0;
const std::string k_developer = "dva";
const std::string k_description = "轮流落子，率先占满棋盘的游戏";

std::string GameOption::StatusInfo() const {
  std::stringstream ss;
  ss << "棋盘大小为" << GET_VALUE(棋盘大小) << "*" << GET_VALUE(棋盘大小) << "；每回合时间限制："
     << GET_VALUE(时限) << "秒";
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
        len(GET_OPTION_VALUE(option, 棋盘大小)),
        table_(GET_OPTION_VALUE(option, 棋盘大小) + 4, 2 + GET_OPTION_VALUE(option, 棋盘大小)) {
    table_.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\"");
    table_.MergeRight(0, 1, len);
    table_.Get(0, 1).SetColor("#eee");
    table_.MergeRight(len + 3, 1, len);
    table_.Get(len + 3, 1).SetColor("#eee");
    for (int i = 0; i < len; i++) {
      table_.Get(i + 2, 0).SetContent(std::string(1, (char)('A' + i)));
      table_.Get(1, i + 1).SetContent(std::to_string(1 + i));
      table_.Get(len + 2, i + 1).SetContent(std::to_string(1 + i));
      for (int j = 0; j < len; j++) {
        table_.Get(i + 2, j + 1)
            .SetContent((i + j) % 2 ? "<div style=\"width: 60px; height: 60px;\"></div>"
                                    : "<div style=\"width: 60px; height: 60px; background-color: "
                                      "#dfdfdf;\"></div>");
      }
    }
  }

  void SetFook(int i, int j, bool black) {
    std::string content =
        (i + j) % 2 ? "<div style=\"width: 60px; height: 60px;\">"
                    : "<div style=\"width: 60px; height: 60px; background-color: #ddd;\">";
    content = content +
              "<svg height=\"60px\" width=\"60px\"><circle cx=\"30\" cy=\"30\" "
              "r=\"25\"style=\"stroke:#000;stroke-width: 2;fill:#" +
              (black ? "333343" : "f3f3f3") + ";\"/></svg>";
    content += "</div>";
    table_.Get(i + 2, j + 1).SetContent(content);
  }

  void SetName(std::string p1_name, std::string p2_name) {
    table_.Get(0, 1).SetContent(HTML_COLOR_FONT_HEADER(#345a88) " **" + p1_name +
                                "** " HTML_FONT_TAIL);
    table_.Get(len + 3, 1)
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

  bool ended_;
  MyTable table_;
  int turn_;
  std::vector<int> side_;
  std::vector<int64_t> score_;
  std::vector<std::vector<int>> board_;
};

bool check_input(std::string input, int n, std::string& err, int& x1, int& x2, int& y1, int& y2) {
  if (input.length() != 2 && input.length() != 4) {
    err = "指令长度错误。注意不需要空格或分隔符，示例：a1b2";
    return false;
  }
  for (auto& c : input)
    if (c >= 'A' && c <= 'Z') c = (char)(c - ('A' - 'a'));
  if (input.length() == 2) {
    x1 = x2 = input[0] - 'a';
    y1 = y2 = input[1] - '1';
  } else {
    x1 = input[0] - 'a';
    y1 = input[1] - '1';
    x2 = input[2] - 'a';
    y2 = input[3] - '1';
    if (abs(x1 - x2) != abs(y1 - y2) && x1 != x2 && y1 != y2) {
      err = "移动方向不正确，只能横向、纵向、对角线方向移动。";
      return false;
    }
  }
  if (x1 < 0 || x1 >= n || y1 < 0 || y1 >= n || x2 < 0 || x2 >= n || y2 < 0 || y2 >= n) {
    err = "发现不合法字符。注意指令中不需要空格或分隔符，示例：a1b2";
    return false;
  }
  return true;
}

class RoundStage : public SubGameStage<> {
 public:
  RoundStage(MainStage& main_stage)
      : GameStage(main_stage, "第" + std::to_string(++main_stage.turn_) + "回合",
                  MakeStageCommand("认输", &RoundStage::Concede_, VoidChecker("认输")),
                  MakeStageCommand("落子", &RoundStage::Set_, AnyArg("落子位置", "a1b2"))) {}

  virtual void OnStageBegin() override {
    auto current_player = main_stage().side_[main_stage().turn_ % 2];
    Boardcast() << "请" << (main_stage().turn_ % 2 ? "黑方" : "白方")
                << At(PlayerID(main_stage().side_[main_stage().turn_ % 2])) << "落子。";
    SetReady(!current_player);
    StartTimer(GET_OPTION_VALUE(option(), 时限));
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    Boardcast() << "玩家 " << PlayerName(pid) << " 中途退出，游戏结束。";
    main_stage().ended_ = true;
    main_stage().score_[pid] = -1;
    return StageErrCode::CHECKOUT;
  }

  virtual CheckoutErrCode OnTimeout() override {
    auto current_player = main_stage().side_[main_stage().turn_ % 2];
    Boardcast() << "玩家 " << PlayerName(current_player) << " 超时未行动，游戏结束。";
    main_stage().ended_ = true;
    main_stage().score_[current_player] = -1;
    return StageErrCode::CHECKOUT;
  }

  virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override {
    Boardcast() << "笨笨的机器人选择认输。";
    main_stage().ended_ = true;
    main_stage().score_[!pid] = 1;
    SetReady(0), SetReady(1);
    return StageErrCode::READY;
  }

 private:
  AtomReqErrCode Concede_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    int current_player = main_stage().side_[main_stage().turn_ % 2];
    if (pid != current_player) {
      return StageErrCode::FAILED;
    }
    main_stage().ended_ = true;
    main_stage().score_[!current_player] = 1;
    SetReady(0), SetReady(1);
    reply() << "玩家认输，游戏结束";
    return StageErrCode::OK;
  }

  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      std::string str) {
    int current_player = main_stage().side_[main_stage().turn_ % 2];
    if (pid != current_player) {
      return StageErrCode::FAILED;
    }
    std::string err;
    int x1, x2, y1, y2;
    int n = GET_OPTION_VALUE(option(), 棋盘大小);
    bool valid = check_input(str, n, err, x1, x2, y1, y2);
    if (!valid) {
      reply() << "落子失败：" << err;
      return StageErrCode::FAILED;
    }
    for (int i = x1, j = y1;;) {
      if (main_stage().board_[i][j] != -1) {
        reply() << "落子失败：" << (char)('A' + i) << (j + 1) << "位置有棋子";
        return StageErrCode::FAILED;
      }
      if (i == x2 && j == y2) break;
      if (i < x2)
        i++;
      else if (i > x2)
        i--;
      if (j < y2)
        j++;
      else if (j > y2)
        j--;
    }
    for (int i = x1, j = y1;;) {
      main_stage().board_[i][j] = pid;
      main_stage().table_.SetFook(i, j, main_stage().turn_ % 2);
      if (i == x2 && j == y2) break;
      if (i < x2)
        i++;
      else if (i > x2)
        i--;
      if (j < y2)
        j++;
      else if (j > y2)
        j--;
    }
    bool has_blank = false;
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        if (main_stage().board_[i][j] == -1) has_blank = true;
      }
    }
    if (!has_blank) {
      main_stage().ended_ = true;
      main_stage().score_[current_player] = 1;
    }
    SetReady(0), SetReady(1);
    reply() << "落子成功。";
    return StageErrCode::OK;
  }
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
    : GameStage(option, match),
      ended_(false),
      table_(option),
      turn_(0),
      score_(2, 0),
      board_(9, std::vector<int>(9, -1)),
      side_(2, 0) {
  side_[0] = rand() % 2;
  side_[1] = !side_[0];
}

MainStage::VariantSubStage MainStage::OnStageBegin() {
  table_.SetName(PlayerName(0), PlayerName(1));
  Boardcast() << Markdown(table_.ToHtml(), 600);
  return std::make_unique<RoundStage>(*this);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage,
                                                   const CheckoutReason reason) {
  if (JudgeOver()) {
    return {};
  }
  return std::make_unique<RoundStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  Boardcast() << Markdown(table_.ToHtml(), 600);
  return ended_;
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
