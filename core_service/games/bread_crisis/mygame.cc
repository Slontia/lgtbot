#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "面包危机",                   // the game name which should be unique among all the games
    .developer_ = "dva",
    .description_ = "保存体力，尽可能活到最后的游戏",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; }  // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; }
// 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options) {
  if (generic_options_readonly.PlayerNum() < 2) {
    reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
    return false;
  }
  return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 6;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(StageUtility&& utility)
      : StageFsm(
            std::move(utility),
            MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"))),
        round_(0),
        alive_(0),
        player_scores_(Global().PlayerNum(), 0),
        player_hp_(Global().PlayerNum(), 30),
        player_select_(Global().PlayerNum(), 'N'),
        player_last_(Global().PlayerNum(), 'N'),
        player_target_(Global().PlayerNum(), 0),
        player_number_(Global().PlayerNum(), 0),
        player_eli_(Global().PlayerNum(), 0) {}

  virtual void FirstStageFsm(SubStageFsmSetter setter) override;
  virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

  virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
  std::vector<int64_t> player_scores_;

  //生命值
  std::vector<int64_t> player_hp_;
  //选择
  std::vector<char> player_select_;
  //上次选择
  std::vector<char> player_last_;
  //选择的玩家
  std::vector<int64_t> player_target_;
  //抢夺数量
  std::vector<int64_t> player_number_;
  //已经被淘汰
  std::vector<int64_t> player_eli_;

  int alive_;
  std::string Pic = "";
  std::string GetName(std::string x);

 private:
  CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    //        reply() << "当前游戏情况：未设置";

    std::string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < Global().PlayerNum(); i++) {
      PreBoard += std::to_string(i + 1) + " 号：" + Global().PlayerName(i);

      if (i != (int)Global().PlayerNum() - 1) {
        PreBoard += "\n";
      }
    }

    Global().Boardcast() << PreBoard;

    std::string WordSitu = "";
    WordSitu += "当前玩家状态如下：\n";
    for (int i = 0; i < Global().PlayerNum(); i++) {
      WordSitu += std::to_string(i + 1) + " 号：";

      WordSitu += std::to_string(player_hp_[i]) + "体力 ";
      if (player_select_[i] == 'N') WordSitu += "民  " + std::to_string(player_number_[i]);
      if (player_select_[i] == 'S')
        WordSitu +=
            "贼" + std::to_string(player_target_[i]) + "  " + std::to_string(player_number_[i]);

      if (i != (int)Global().PlayerNum() - 1) {
        WordSitu += "\n";
      }
    }

    //        select is updated unexpectedly, which may cause error, so use Pic instead now.
    //        Global().Boardcast() << WordSitu;

    Global().Boardcast() << Markdown(Pic + "</table>");

    // Returning |OK| means the game stage
    return StageErrCode::OK;
  }

  int round_;
};

class RoundStage : public SubGameStage<> {
 public:
  RoundStage(MainStage& main_stage, const uint64_t round)
      : StageFsm(
            main_stage, "第 " + std::to_string(round) + " 回合",
            MakeStageCommand(*this, "获取面包", &RoundStage::Select_N_,
                             ArithChecker<int64_t>(1, 5, "面包数量")),
            MakeStageCommand(*this, "抢夺面包", &RoundStage::Select_S_,
                             ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "抢夺目标"),
                             ArithChecker<int64_t>(1, 5, "面包数量"))) {}

  virtual void OnStageBegin() override {
    Global().StartTimer(GAME_OPTION(时限));

    Global().Boardcast() << Name() << "，请玩家私信选择行动。";
  }

  virtual CheckoutErrCode OnStageTimeout() override {
    //        Global().Boardcast() << Name() << "超时结束";

    for (int i = 0; i < Global().PlayerNum(); i++) {
      //             Global().Boardcast()<<std::to_string(Global().IsReady(i))<<std::to_string(Main().player_eli_[i]);
      if (Global().IsReady(i) == false && !Main().player_eli_[i]) {
        Global().Boardcast() << "玩家 " << Main().Global().PlayerName(i) << " 超时仍未行动，已被淘汰";
        Main().player_hp_[i] = 0;
        Main().player_select_[i] = 'N';
        Main().player_number_[i] = rand() % 5 + 1;
        Main().player_target_[i] = 0;
      }
    }

    RoundStage::calc();
    // Returning |CHECKOUT| means the current stage will be over.
    return StageErrCode::CHECKOUT;
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override {
    int i = pid;
    //        Global().Boardcast() << Global().PlayerName(i) << "退出游戏";
    Main().player_hp_[i] = 0;
    Main().player_select_[i] = 'N';
    Main().player_number_[i] = rand() % 5 + 1;
    Main().player_target_[i] = 0;
    // Returning |CONTINUE| means the current stage will be continued.
    return StageErrCode::CONTINUE;
  }

  virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override {
    int i = pid;
    Main().player_select_[i] = 'N';
    Main().player_number_[i] = rand() % 4 + 2;
    Main().player_target_[i] = 0;

    return StageErrCode::READY;
  }

  virtual CheckoutErrCode OnStageOver() override {
    Global().Boardcast() << "所有玩家选择完成，下面公布赛况。";
    RoundStage::calc();
    return StageErrCode::CHECKOUT;
  }

  void calc() {
    // clear eliminated players number
    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (Main().player_eli_[i] == 1) {
          Main().player_number_[i] = -1;
      }
    }
    
    // first calculate thives
    std::vector<bool> stolen(Global().PlayerNum(), false);
    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (Main().player_select_[i] == 'S') {
        int target = Main().player_target_[i] - 1;
        if (Main().player_select_[target] == 'N' &&
            Main().player_number_[target] == Main().player_number_[i]) {
          Main().player_hp_[i] += 5;
          Main().player_hp_[target] = 0;
          stolen[target] = true;
        } else {
          stolen[i] = true;
        }
      }
    }

    // finally calculate commons
    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (!stolen[i]) {
        Main().player_hp_[i] += Main().player_number_[i];
      }
      Main().player_hp_[i] -= 5;
    }

    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (Main().player_hp_[i] < 0) Main().player_hp_[i] = 0;
    }

    std::string x = "<tr>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
      x += "<td bgcolor=\"#DCDCDC\">";

      if (Main().player_eli_[i] == 1) {
        x += " ";
      } else {
        if (Main().player_select_[i] == 'S')
          x += "抢夺" + std::to_string(Main().player_target_[i]);
        else
          x += "获取";
        x += " " + std::to_string(Main().player_number_[i]);
      }
      x += "</td>";
    }
    x += "</tr>";
    x += "<tr>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (Main().player_select_[i] == 'S' && !stolen[i]) {
        x += "<td bgcolor=\"#BAFFA8\">";
      } else {
        x += "<td>";
      }

      if (Main().player_eli_[i] == 0) {
        x += std::to_string(Main().player_hp_[i]);
      }

      x += "</td>";
    }
    x += "</tr>";
    Main().Pic += x;
    Global().Boardcast() << Markdown(Main().Pic + "</table>");

    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (Main().player_hp_[i] <= 0) {
        Main().player_select_[i] = ' ';

        if (Main().player_eli_[i] == 0) {
          Main().player_eli_[i] = 1;
          Global().Eliminate(i);
          Main().alive_--;
        }
      } else {
        // Everyone gain 1 score for his survival
        Main().player_scores_[i]++;
      }
    }

    for (int i = 0; i < Global().PlayerNum(); i++)
      Main().player_last_[i] = Main().player_select_[i];
  }

 private:
  AtomReqErrCode Select_N_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                           const int64_t number) {
    if (is_public) {
      reply() << "[错误] 请私信裁判进行选择。";
      return StageErrCode::FAILED;
    }
    if (Global().IsReady(pid)) {
      reply() << "[错误] 您本回合已经进行过选择了。";
      return StageErrCode::FAILED;
    }
    return Selected_(pid, reply, 'N', number);
  }

  AtomReqErrCode Select_S_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                           const int64_t target, const int64_t number) {
    if (is_public) {
      reply() << "[错误] 请私信裁判进行选择。";
      return StageErrCode::FAILED;
    }
    if (Global().IsReady(pid)) {
      reply() << "[错误] 您本回合已经进行过选择了。";
      return StageErrCode::FAILED;
    }
    if (target < 1 || target > Global().PlayerNum() || target - 1 == pid ||
        Main().player_eli_[target - 1] == 1) {
      reply() << "[错误] 目标无效。";
      return StageErrCode::FAILED;
    }
    return Selected_(pid, reply, 'S', number, target);
  }

  AtomReqErrCode Selected_(const PlayerID pid, MsgSenderBase& reply, char type,
                           const int64_t number, const int64_t target = 0) {
    if (type != 'S' && type != 'N' && type != 'P') {
      Global().Boardcast() << "Wrong input on pid = " << std::to_string(pid) << std::to_string(type)
                  << " in f->Selected";
    }
    // Global().Boardcast() << pid << " " << type << number << " " << target;
    Main().player_select_[pid] = type;
    Main().player_number_[pid] = number;
    Main().player_target_[pid] = target;
    //        reply() << "设置成功。";
    // Returning |READY| means the player is ready. The current stage will be over when all
    // surviving players are ready.
    return StageErrCode::READY;
  }
};

std::string MainStage::GetName(std::string x) {
  std::string ret = "";
  int n = x.length();
  if (n == 0) return ret;

  int l = 0;
  int r = n - 1;

  if (x[0] == '<') l++;
  if (x[r] == '>') {
    while (r >= 0 && x[r] != '(') r--;
    r--;
  }

  for (int i = l; i <= r; i++) {
    ret += x[i];
  }
  return ret;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter) {
  srand((unsigned int)time(NULL));
  alive_ = Global().PlayerNum();
  for (int i = 0; i < Global().PlayerNum(); i++) {
    player_hp_[i] = GAME_OPTION(血量);
  }

  Pic += "<table><tr>";
  for (int i = 0; i < Global().PlayerNum(); i++) {
    Pic += "<th >" + std::to_string(i + 1) + " 号： " + GetName(Global().PlayerName(i)) + "　</th>";
    if (i % 4 == 3) Pic += "</tr><tr>";
  }
  Pic += "</tr><br>";

  Pic += "<table style=\"text-align:center\"><tbody>";
  Pic += "<tr>";
  for (int i = 0; i < Global().PlayerNum(); i++) {
    //        if(i%2)
    //            Pic+="<th>";
    //        else
    Pic += "<th bgcolor=\"#FFE4C4\">";

    // Please note that this is not 2 spaces but a special symbol.
    Pic += "　" + std::to_string(i + 1) + " 号　";
    Pic += "</th>";
  }
  Pic += "</tr>";

  Pic += "<tr>";
  for (int i = 0; i < Global().PlayerNum(); i++) {
    //        if(i%2)
    Pic += "<td>";
    //        else
    //            Pic+="<td bgcolor=\"#DCDCDC\">";
    Pic += std::to_string(GAME_OPTION(血量));
    Pic += "</td>";
  }
  Pic += "</tr>";

  std::string PreBoard = "";
  PreBoard += "本局玩家序号如下：\n";
  for (int i = 0; i < Global().PlayerNum(); i++) {
    PreBoard += std::to_string(i + 1) + " 号：" + Global().PlayerName(i);

    if (i != (int)Global().PlayerNum() - 1) {
      PreBoard += "\n";
    }
  }

  Global().Boardcast() << PreBoard;
  Global().Boardcast() << Markdown(Pic + "</table>");

  setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) {
  //    Global().Boardcast()<<"alive"<<std::to_string(alive_);
  if ((++round_) <= GAME_OPTION(回合数) && alive_ > 1) {
    setter.Emplace<RoundStage>(*this, round_);
    return;
  }
  //    Global().Boardcast() << "游戏结束";
  if (alive_ >= 1) {
    int maxHP = 0;
    for (int i = 0; i < Global().PlayerNum(); i++) {
      maxHP = std::max(maxHP, (int)player_hp_[i]);
    }
    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (maxHP == player_hp_[i]) {
        // The last survival gain 5 scores.
        player_scores_[i] += 5;
      } else if (player_eli_[i] == 0) {
        // The second last survival
        player_scores_[i] += 3;
      }
    }
  } else {
    // alive_ == 0
    int max_N = 0;
    std::vector<int> win_player;
    for (int i = 0; i < Global().PlayerNum(); i++) {
      if (player_target_[i] != 0) {
        player_number_[i] = -1;
      }
        if (player_number_[i] > max_N) {
          win_player.clear();
          win_player.push_back(i);
          max_N = player_number_[i];
        } else if (player_number_[i] == max_N) {
          win_player.push_back(i);
        }
    }
    for (int i = 0; i < win_player.size(); i++) {
      player_scores_[win_player[i]] += 5;
    }
  }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

