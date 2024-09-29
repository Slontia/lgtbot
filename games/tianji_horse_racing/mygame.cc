// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <vector>
#include <random>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "田忌赛马", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "按恰当顺序选择数字，尽可能成为唯一最大数的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    if (GET_OPTION_VALUE(game_options, N) == 0) {
        if (generic_options_readonly.PlayerNum() <= 6) {
            GET_OPTION_VALUE(game_options, N) = 13;
        } else {
            GET_OPTION_VALUE(game_options, N) = generic_options_readonly.PlayerNum() * 2 + 1;
        }
        return true;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 5;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"))),
        round_(0),
        player_scores_(Global().PlayerNum(), 0),
        player_last_scores_(Global().PlayerNum(), 0),
        player_select_(Global().PlayerNum(), 0) {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
    
    vector<int64_t> player_scores_;
    vector<int64_t> player_last_scores_;   // 玩家上回合分数
    int round_;

    vector<vector<int64_t>> player_leftnum_;   // 玩家剩余数字
    vector<int64_t> player_select_;   // 玩家选择
    vector<int64_t> X;   // 每轮数字X
    string status_string;   // 玩家剩余数字赛况

    string T_Board = "";   // 表头
    string Board = "";   // 赛况

    // 颜色样式
    const string score_color = "FDD12E";   // 分数底色
    const string X_color = "DCDCDC";   // X栏底色
    const string win_color = "BAFFA8";   // 加分颜色
    const string lose_color = "FFB090";   // 减分颜色
    const string collision_color = "C0DFFF";   // 相撞标记颜色

    string GetName(std::string x);
    string GetScoreBoard();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string score_Board = GetScoreBoard();
        string X_Board = "<tr><td align=\"left\" colspan=" + to_string(Global().PlayerNum() + 1) + "><font size=5>· 本轮 X 为：" + to_string(X[round_ - 1]) + "</font></td></tr>";

        reply() << Markdown(T_Board + score_Board + Board + X_Board + "</table>" + status_string);

        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " / " + std::to_string(main_stage.X.size()) + " 回合",
                MakeStageCommand(*this, "提交数字", &RoundStage::Submit_, ArithChecker<int64_t>(1, main_stage.X.size(), "数字"))) {}

  private:
    virtual void OnStageBegin() override
    {
        if (Main().round_ < GAME_OPTION(N)) {
            Global().Boardcast() << "本轮 X 为 " + to_string(Main().X[Main().round_ - 1]) + "\n请玩家私信选择数字。";
            Global().StartTimer(GAME_OPTION(时限));
        } else {
            HandleUnreadyPlayers_();
        }
    }

    string GetLeftNums(const PlayerID pid)
    {
        string left_nums = "none";
        for (int i = 0; i < Main().player_leftnum_[pid].size(); i++) {
            if (i == 0)
                left_nums = "\n" + to_string(Main().player_leftnum_[pid][i]);
            else
                left_nums += " " + to_string(Main().player_leftnum_[pid][i]);
        }
        return left_nums;
    }

    void PlayerSelectNum(const PlayerID pid, const int select)
    {
        Main().player_select_[pid] = select;
        Main().player_leftnum_[pid].erase(remove(Main().player_leftnum_[pid].begin(), Main().player_leftnum_[pid].end(), select), Main().player_leftnum_[pid].end());
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t num)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判提交数字";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经提交过数字了";
            return StageErrCode::FAILED;
        }
        
        if (count(Main().player_leftnum_[pid].begin(), Main().player_leftnum_[pid].end(), num) == 0) {
            reply() << "[错误] 您已经使用过此数字了，剩余可用数字：" + GetLeftNums(pid);
            return StageErrCode::FAILED;
        }
        PlayerSelectNum(pid, num);
        reply() << "提交成功，剩余可用数字：" + GetLeftNums(pid);
        return StageErrCode::READY;
    }

    void HandleUnreadyPlayers_()
    {
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (!Global().IsReady(pid)) {
                PlayerSelectNum(pid, Main().player_leftnum_[pid][Main().player_leftnum_[pid].size() - 1]);
                Global().SetReady(pid);
            }
        }
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().HookUnreadyPlayers();
        HandleUnreadyPlayers_();
        calc();
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        int rd = rand() % Main().player_leftnum_[pid].size();
        PlayerSelectNum(pid, Main().player_leftnum_[pid][rd]);
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        if (Main().round_ < GAME_OPTION(N)) {
            Global().Boardcast() << "第 " + to_string(Main().round_) + " 回合结束，下面公布赛况";
        } else {
            Global().Boardcast() << "仅剩最后一张，将自动结算";
        }
        calc();
        return StageErrCode::CHECKOUT;
    }

    void calc() {
        vector<int> win(1, 0);
        vector<int> lose(1, 0);
		for (int i = 1; i < Global().PlayerNum(); i++) {
            if (Main().player_select_[i] > Main().player_select_[win[0]]) {
                win.clear();
				win.push_back(i);
			} else if (Main().player_select_[i] == Main().player_select_[win[0]]) {
                win.push_back(i);
			}
			if (Main().player_select_[i] < Main().player_select_[lose[0]]) {
                lose.clear();
				lose.push_back(i);
			} else if (Main().player_select_[i] == Main().player_select_[lose[0]]) {
                lose.push_back(i);
			}
		}

        if (win.size() == 1) {
            Main().player_scores_[win[0]] += Main().X[Main().round_ - 1];
        } else if (lose.size() == 1) {
            Main().player_scores_[lose[0]] -= Main().X[Main().round_ - 1];
        }

        // 每回合初始化赛况html
        Main().status_string = "";
        if (Main().round_ < GAME_OPTION(N) - 1) {
            Main().status_string += "<table style=\"text-align:center\"><tr><th colspan=" + to_string(GAME_OPTION(N) + 1) + ">剩余数字总览</th></tr><tr><th>【X】</th>";
            for (int i = 1; i <= GAME_OPTION(N); i++) {
                Main().status_string += "<td>";
                if (count(Main().X.begin() + Main().round_ + 1, Main().X.end(), i)) {
                    Main().status_string += to_string(i);
                }
                Main().status_string += "</td>";
            }
            Main().status_string += "</tr>";
            for (int pid = 0; pid < Global().PlayerNum(); pid++) {
                Main().status_string += "<tr><th>" + to_string(pid + 1) + "号：</th>";
                for (int i = 1; i <= GAME_OPTION(N); i++) {
                    Main().status_string += "<td>";
                    if (count(Main().player_leftnum_[pid].begin(), Main().player_leftnum_[pid].end(), i)) {
                        Main().status_string += to_string(i);
                    }
                    Main().status_string += "</td>";
                }
                Main().status_string += "</tr>";
            }
            Main().status_string += "</table>";
        }

        // board
        string score_Board = Main().GetScoreBoard();

        string b = "<tr><td bgcolor=\"" + Main().X_color + "\">" + to_string(Main().X[Main().round_ - 1]) + "</td>";
        for (int i = 0; i < Global().PlayerNum(); i++) {
            string color = "";
            if ((win.size() > 1 && count(win.begin(), win.end(), i)) || (win.size() > 1 && lose.size() > 1 && count(lose.begin(), lose.end(), i))) {
                color = " bgcolor=\"" + Main().collision_color + "\"";
            } else if (win.size() == 1 && win[0] == i) {
                color = " bgcolor=\"" + Main().win_color + "\"";
            } else if (win.size() > 1 && lose.size() == 1 && lose[0] == i) {
                color = " bgcolor=\"" + Main().lose_color + "\"";
            }
            b += "<td" + color +">" + to_string(Main().player_select_[i]) + "</td>";
        }
        b += "</tr>";
        Main().Board += b;

        string X_Board = "";
        if (Main().round_ < GAME_OPTION(N)) {
            X_Board = "<tr><td align=\"left\" colspan=" + to_string(Global().PlayerNum() + 1) + "><font size=5>· 本轮 X 为：" + to_string(Main().X[Main().round_]) + "</font></td></tr>";
        }

        Global().Boardcast() << Markdown(Main().T_Board + score_Board + Main().Board + X_Board + "</table>" + Main().status_string);

        if (win.size() == 1) {
            Global().Boardcast() << "最大者为\n" + Global().PlayerName(win[0]) + "\n获得 " + to_string(Main().X[Main().round_ - 1]) + " 分";
        } else if (lose.size() == 1) {
            Global().Boardcast() << "最小者为\n" + Global().PlayerName(lose[0]) + "\n失去 " + to_string(Main().X[Main().round_ - 1]) + " 分";
        } else {
            Global().Boardcast() << "最大最小者均不唯一，分数不变";
        }

        for (int i = 0; i < Global().PlayerNum(); i++) {
            Main().player_last_scores_[i] = Main().player_scores_[i];
        }
    }
};

string MainStage::GetName(std::string x) {
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

string MainStage::GetScoreBoard() {
    string score_Board = "";
    score_Board += "<tr><th bgcolor=\""+ X_color +"\">X</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        score_Board += "<td bgcolor=\""+ score_color +"\">";
        score_Board += to_string(player_last_scores_[i]);
        if (player_scores_[i] > player_last_scores_[i]) {
            score_Board += "<font color=\"#00AC30\">+" + to_string(X[round_ - 1]) + "</font>";
        } else if (player_scores_[i] < player_last_scores_[i]) {
            score_Board += "<font color=\"#FF0000\">-" + to_string(X[round_ - 1]) + "</font>";
        }
        score_Board += "</td>";
    }
    score_Board += "</tr>";
    return score_Board;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));
    player_leftnum_.resize(Global().PlayerNum());

    for (int i = 0; i < Global().PlayerNum(); i++) {
        player_leftnum_[i].resize(GAME_OPTION(N));
    }
    for (int i = 0; i < Global().PlayerNum(); i++) {
        for (int j = 1; j <= GAME_OPTION(N); j++) {
            player_leftnum_[i][j-1] = j;
        }
    }
    for (int i = 1; i <= GAME_OPTION(N); i++) {
        X.push_back(i);
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(X.begin(), X.end(), g);

    T_Board += "<table><tr>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        T_Board += "<th>" + to_string(i + 1) + " 号： " + GetName(Global().PlayerName(i)) + "　</th>";
        if (i % 4 == 3) T_Board += "</tr><tr>";
    }
    T_Board += "</tr><br>";

    T_Board += "<table style=\"text-align:center\"><tbody>";
    T_Board += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:70px;\">序号</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        T_Board += "<th style=\"width:60px;\">";
        T_Board += to_string(i + 1) + " 号";
        T_Board += "</th>";
    }
    T_Board += "</tr>";

    string score_Board = GetScoreBoard();
    string X_Board = "<tr><td align=\"left\" colspan=" + to_string(Global().PlayerNum() + 1) + "><font size=5>· 本轮 X 为：" + to_string(X[round_]) + "</font></td></tr>";

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + Global().PlayerName(i);
        if (i != (int)Global().PlayerNum() - 1) {
            PreBoard += "\n";
        }
    }

    Global().Boardcast() << PreBoard;
    Global().Boardcast() << Markdown(T_Board + score_Board + X_Board + "</table>");
    Global().Boardcast() << "[提示] 本局游戏 N 为：" + to_string(GAME_OPTION(N));

    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if ((++round_) <= GAME_OPTION(N)) {
        setter.Emplace<RoundStage>(*this, round_);
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

