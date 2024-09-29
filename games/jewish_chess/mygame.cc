// Author:  Dva
// Date:    2022.10.5

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <random>

#include "game_framework/stage.h"
#include "game_framework/util.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "犹太人棋",
    .developer_ = "dva",
    .description_ = "轮流落子，率先占满棋盘的游戏",
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
};

// ========== UI ==============

struct MyTable {
  MyTable(const CustomOptions& option, const std::string_view resource_dir)
      : resource_dir_(resource_dir),
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
  std::string Image_(const std::string_view name) const {
    return std::string("![](file:///") + resource_dir_.data() + "/" + name.data() + ".png)";
  }

  std::string_view resource_dir_;
  html::Table table_;
  int len;
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(StageUtility&& utility);
  virtual void FirstStageFsm(SubStageFsmSetter setter) override;
  virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
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
      : StageFsm(main_stage, "第" + std::to_string(++main_stage.turn_) + "回合",
                  MakeStageCommand(*this, "认输", &RoundStage::Concede_, VoidChecker("认输")),
                  MakeStageCommand(*this, "落子", &RoundStage::Set_, AnyArg("落子位置", "a1b2"))) {}

  virtual void OnStageBegin() override {
    auto current_player = Main().side_[Main().turn_ % 2];
    Global().Boardcast() << "请" << (Main().turn_ % 2 ? "黑方" : "白方")
                << At(PlayerID(Main().side_[Main().turn_ % 2])) << "落子。";
    Global().SetReady(!current_player);
    Global().StartTimer(GAME_OPTION(时限));
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    Global().Boardcast() << "玩家 " << Global().PlayerName(pid) << " 中途退出，游戏结束。";
    Main().ended_ = true;
    Main().score_[pid] = -1;
    return StageErrCode::CHECKOUT;
  }

  virtual CheckoutErrCode OnStageTimeout() override {
    auto current_player = Main().side_[Main().turn_ % 2];
    Global().Boardcast() << "玩家 " << Global().PlayerName(current_player) << " 超时未行动，游戏结束。";
    Main().ended_ = true;
    Main().score_[current_player] = -1;
    return StageErrCode::CHECKOUT;
  }

  virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override {
    Global().Boardcast() << "笨笨的机器人选择认输。";
    Main().ended_ = true;
    Main().score_[!pid] = 1;
    Global().SetReady(0), Global().SetReady(1);
    return StageErrCode::READY;
  }

 private:
  AtomReqErrCode Concede_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    int current_player = Main().side_[Main().turn_ % 2];
    if (pid != current_player) {
      return StageErrCode::FAILED;
    }
    Main().ended_ = true;
    Main().score_[!current_player] = 1;
    Global().SetReady(0), Global().SetReady(1);
    reply() << "玩家认输，游戏结束";
    return StageErrCode::OK;
  }

  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      std::string str) {
    int current_player = Main().side_[Main().turn_ % 2];
    if (pid != current_player) {
      return StageErrCode::FAILED;
    }
    std::string err;
    int x1, x2, y1, y2;
    int n = GAME_OPTION(棋盘大小);
    bool valid = check_input(str, n, err, x1, x2, y1, y2);
    if (!valid) {
      reply() << "落子失败：" << err;
      return StageErrCode::FAILED;
    }
    for (int i = x1, j = y1;;) {
      if (Main().board_[i][j] != -1) {
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
      Main().board_[i][j] = pid;
      Main().table_.SetFook(i, j, Main().turn_ % 2);
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
        if (Main().board_[i][j] == -1) has_blank = true;
      }
    }
    if (!has_blank) {
      Main().ended_ = true;
      Main().score_[current_player] = 1;
    }
    Global().SetReady(0), Global().SetReady(1);
    reply() << "落子成功。";
    return StageErrCode::OK;
  }
};

MainStage::MainStage(StageUtility&& utility)
    : StageFsm(std::move(utility)),
      ended_(false),
      table_(utility.Options(), utility.ResourceDir()),
      turn_(0),
      score_(2, 0),
      board_(9, std::vector<int>(9, -1)),
      side_(2, 0) {
  side_[0] = rand() % 2;
  side_[1] = !side_[0];
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter) {
  table_.SetName(Global().PlayerName(0), Global().PlayerName(1));
  Global().Boardcast() << Markdown(table_.ToHtml(), 600);
  setter.Emplace<RoundStage>(*this);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) {
  if (JudgeOver()) {
    return;
  }
  setter.Emplace<RoundStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  Global().Boardcast() << Markdown(table_.ToHtml(), 600);
  return ended_;
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

