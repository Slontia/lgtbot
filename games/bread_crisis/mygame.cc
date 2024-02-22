#include <array>
#include <functional>
#include <memory>
#include <set>

#include "game_framework/game_stage.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name =
    "面包危机";                   // the game name which should be unique among all the games
const uint64_t k_max_player = 0;  // 0 indicates no max-player limits
const uint64_t k_multiple = 1;    // the default score multiple for the game, 0 for a testing game,
// 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "dva";
const std::string k_description = "保存体力，尽可能活到最后的游戏";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const {
  return "最多 " + std::to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " +
         std::to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply) {
  if (PlayerNum() < 3) {
    reply() << "该游戏至少 3 人参加，当前玩家数为 " << PlayerNum();
    return false;
  }
  return true;
}

uint64_t GameOption::BestPlayerNum() const { return 6; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(const GameOption& option, MatchBase& match)
      : GameStage(
            option, match,
            MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"))),
        round_(0),
        alive_(0),
        player_scores_(option.PlayerNum(), 0),
        player_hp_(option.PlayerNum(), 30),
        player_select_(option.PlayerNum(), 'N'),
        player_last_(option.PlayerNum(), 'N'),
        player_target_(option.PlayerNum(), 0),
        player_number_(option.PlayerNum(), 0),
        player_eli_(option.PlayerNum(), 0) {}

  virtual VariantSubStage OnStageBegin() override;
  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

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
    for (int i = 0; i < option().PlayerNum(); i++) {
      PreBoard += std::to_string(i + 1) + " 号：" + PlayerName(i);

      if (i != (int)option().PlayerNum() - 1) {
        PreBoard += "\n";
      }
    }

    Boardcast() << PreBoard;

    std::string WordSitu = "";
    WordSitu += "当前玩家状态如下：\n";
    for (int i = 0; i < option().PlayerNum(); i++) {
      WordSitu += std::to_string(i + 1) + " 号：";

      WordSitu += std::to_string(player_hp_[i]) + "体力 ";
      if (player_select_[i] == 'N') WordSitu += "民  " + std::to_string(player_number_[i]);
      if (player_select_[i] == 'S')
        WordSitu +=
            "贼" + std::to_string(player_target_[i]) + "  " + std::to_string(player_number_[i]);

      if (i != (int)option().PlayerNum() - 1) {
        WordSitu += "\n";
      }
    }

    //        select is updated unexpectedly, which may cause error, so use Pic instead now.
    //        Boardcast() << WordSitu;

    Boardcast() << Markdown(Pic + "</table>");

    // Returning |OK| means the game stage
    return StageErrCode::OK;
  }

  int round_;
};

class RoundStage : public SubGameStage<> {
 public:
  RoundStage(MainStage& main_stage, const uint64_t round)
      : GameStage(
            main_stage, "第 " + std::to_string(round) + " 回合",
            MakeStageCommand("获取面包", &RoundStage::Select_N_,
                             ArithChecker<int64_t>(1, 5, "面包数量")),
            MakeStageCommand("抢夺面包", &RoundStage::Select_S_,
                             ArithChecker<int64_t>(1, main_stage.option().PlayerNum(), "抢夺目标"),
                             ArithChecker<int64_t>(1, 5, "面包数量"))) {}

  virtual void OnStageBegin() override {
    StartTimer(GET_OPTION_VALUE(option(), 时限));

    Boardcast() << name() << "，请玩家私信选择行动。";
  }

  virtual CheckoutErrCode OnStageTimeout() override {
    //        Boardcast() << name() << "超时结束";

    for (int i = 0; i < option().PlayerNum(); i++) {
      //             Boardcast()<<std::to_string(IsReady(i))<<std::to_string(main_stage().player_eli_[i]);
      if (IsReady(i) == false && !main_stage().player_eli_[i]) {
        Boardcast() << "玩家 " << main_stage().PlayerName(i) << " 超时仍未行动，已被淘汰";
        main_stage().player_hp_[i] = 0;
        main_stage().player_select_[i] = 'N';
        main_stage().player_number_[i] = rand() % 5 + 1;
        main_stage().player_target_[i] = 0;
      }
    }

    RoundStage::calc();
    // Returning |CHECKOUT| means the current stage will be over.
    return StageErrCode::CHECKOUT;
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override {
    int i = pid;
    //        Boardcast() << PlayerName(i) << "退出游戏";
    main_stage().player_hp_[i] = 0;
    main_stage().player_select_[i] = 'N';
    main_stage().player_number_[i] = rand() % 5 + 1;
    main_stage().player_target_[i] = 0;
    // Returning |CONTINUE| means the current stage will be continued.
    return StageErrCode::CONTINUE;
  }

  virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override {
    int i = pid;
    main_stage().player_select_[i] = 'N';
    main_stage().player_number_[i] = rand() % 4 + 2;
    main_stage().player_target_[i] = 0;

    return StageErrCode::READY;
  }

  virtual CheckoutErrCode OnStageOver() override {
    Boardcast() << "所有玩家选择完成，下面公布赛况。";
    RoundStage::calc();
    return StageErrCode::CHECKOUT;
  }

  void calc() {
    // clear eliminated players number
    for (int i = 0; i < option().PlayerNum(); i++) {
      if (main_stage().player_eli_[i] == 1) {
          main_stage().player_number_[i] = -1;
      }
    }
    
    // first calculate thives
    std::vector<bool> stolen(option().PlayerNum(), false);
    for (int i = 0; i < option().PlayerNum(); i++) {
      if (main_stage().player_select_[i] == 'S') {
        int target = main_stage().player_target_[i] - 1;
        if (main_stage().player_select_[target] == 'N' &&
            main_stage().player_number_[target] == main_stage().player_number_[i]) {
          main_stage().player_hp_[i] += 5;
          main_stage().player_hp_[target] = 0;
          stolen[target] = true;
        } else {
          stolen[i] = true;
        }
      }
    }

    // finally calculate commons
    for (int i = 0; i < option().PlayerNum(); i++) {
      if (!stolen[i]) {
        main_stage().player_hp_[i] += main_stage().player_number_[i];
      }
      main_stage().player_hp_[i] -= 5;
    }

    for (int i = 0; i < option().PlayerNum(); i++) {
      if (main_stage().player_hp_[i] < 0) main_stage().player_hp_[i] = 0;
    }

    std::string x = "<tr>";
    for (int i = 0; i < option().PlayerNum(); i++) {
      x += "<td bgcolor=\"#DCDCDC\">";

      if (main_stage().player_eli_[i] == 1) {
        x += " ";
      } else {
        if (main_stage().player_select_[i] == 'S')
          x += "抢夺" + std::to_string(main_stage().player_target_[i]);
        else
          x += "获取";
        x += " " + std::to_string(main_stage().player_number_[i]);
      }
      x += "</td>";
    }
    x += "</tr>";
    x += "<tr>";
    for (int i = 0; i < option().PlayerNum(); i++) {
      if (main_stage().player_select_[i] == 'S' && !stolen[i]) {
        x += "<td bgcolor=\"#BAFFA8\">";
      } else {
        x += "<td>";
      }

      if (main_stage().player_eli_[i] == 0) {
        x += std::to_string(main_stage().player_hp_[i]);
      }

      x += "</td>";
    }
    x += "</tr>";
    main_stage().Pic += x;
    Boardcast() << Markdown(main_stage().Pic + "</table>");

    for (int i = 0; i < option().PlayerNum(); i++) {
      if (main_stage().player_hp_[i] <= 0) {
        main_stage().player_select_[i] = ' ';

        if (main_stage().player_eli_[i] == 0) {
          main_stage().player_eli_[i] = 1;
          Eliminate(i);
          main_stage().alive_--;
        }
      } else {
        // Everyone gain 1 score for his survival
        main_stage().player_scores_[i]++;
      }
    }

    for (int i = 0; i < option().PlayerNum(); i++)
      main_stage().player_last_[i] = main_stage().player_select_[i];
  }

 private:
  AtomReqErrCode Select_N_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                           const int64_t number) {
    if (is_public) {
      reply() << "[错误] 请私信裁判进行选择。";
      return StageErrCode::FAILED;
    }
    if (IsReady(pid)) {
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
    if (IsReady(pid)) {
      reply() << "[错误] 您本回合已经进行过选择了。";
      return StageErrCode::FAILED;
    }
    if (target < 1 || target > option().PlayerNum() || target - 1 == pid ||
        main_stage().player_eli_[target - 1] == 1) {
      reply() << "[错误] 目标无效。";
      return StageErrCode::FAILED;
    }
    return Selected_(pid, reply, 'S', number, target);
  }

  AtomReqErrCode Selected_(const PlayerID pid, MsgSenderBase& reply, char type,
                           const int64_t number, const int64_t target = 0) {
    if (type != 'S' && type != 'N' && type != 'P') {
      Boardcast() << "Wrong input on pid = " << std::to_string(pid) << std::to_string(type)
                  << " in f->Selected";
    }
    // Boardcast() << pid << " " << type << number << " " << target;
    main_stage().player_select_[pid] = type;
    main_stage().player_number_[pid] = number;
    main_stage().player_target_[pid] = target;
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

MainStage::VariantSubStage MainStage::OnStageBegin() {
  srand((unsigned int)time(NULL));
  alive_ = option().PlayerNum();
  for (int i = 0; i < option().PlayerNum(); i++) {
    player_hp_[i] = GET_OPTION_VALUE(option(), 血量);
  }

  Pic += "<table><tr>";
  for (int i = 0; i < option().PlayerNum(); i++) {
    Pic += "<th >" + std::to_string(i + 1) + " 号： " + GetName(PlayerName(i)) + "　</th>";
    if (i % 4 == 3) Pic += "</tr><tr>";
  }
  Pic += "</tr><br>";

  Pic += "<table style=\"text-align:center\"><tbody>";
  Pic += "<tr>";
  for (int i = 0; i < option().PlayerNum(); i++) {
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
  for (int i = 0; i < option().PlayerNum(); i++) {
    //        if(i%2)
    Pic += "<td>";
    //        else
    //            Pic+="<td bgcolor=\"#DCDCDC\">";
    Pic += std::to_string(GET_OPTION_VALUE(option(), 血量));
    Pic += "</td>";
  }
  Pic += "</tr>";

  std::string PreBoard = "";
  PreBoard += "本局玩家序号如下：\n";
  for (int i = 0; i < option().PlayerNum(); i++) {
    PreBoard += std::to_string(i + 1) + " 号：" + PlayerName(i);

    if (i != (int)option().PlayerNum() - 1) {
      PreBoard += "\n";
    }
  }

  Boardcast() << PreBoard;
  Boardcast() << Markdown(Pic + "</table>");

  return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage,
                                                   const CheckoutReason reason) {
  //    Boardcast()<<"alive"<<std::to_string(alive_);
  if ((++round_) <= GET_OPTION_VALUE(option(), 回合数) && alive_ > 1) {
    return std::make_unique<RoundStage>(*this, round_);
  }
  //    Boardcast() << "游戏结束";
  if (alive_ == 1) {
    int maxHP = 0;
    for (int i = 0; i < option().PlayerNum(); i++) {
      maxHP = std::max(maxHP, (int)player_hp_[i]);
    }
    for (int i = 0; i < option().PlayerNum(); i++) {
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
    for (int i = 0; i < option().PlayerNum(); i++) {
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
  // Returning empty variant means the game will be over.
  return {};
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

#include "game_framework/make_main_stage.h"
