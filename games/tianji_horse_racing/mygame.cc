// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <vector>
#include <random>

#include "game_framework/game_stage.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "田忌赛马"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 1; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "铁蛋";
const std::string k_description = "按恰当顺序选择数字，尽可能成为唯一最大数的游戏";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const { return ""; }

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    if (GET_VALUE(N) == 0) {
        if (PlayerNum() <= 6) {
            GET_VALUE(N) = 13;
        } else {
            GET_VALUE(N) = PlayerNum() * 2 + 1;
        }
        return true;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 5; }

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match, MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"))),
        round_(0),
        player_scores_(option.PlayerNum(), 0),
        player_last_scores_(option.PlayerNum(), 0),
        player_select_(option.PlayerNum(), 0) {}

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

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
    string score_color = "FDD12E";   // 分数底色
    string X_color = "DCDCDC";   // X栏底色
    string win_color = "BAFFA8";   // 加分颜色
    string lose_color = "FFB090";   // 减分颜色
    string collision_color = "C0DFFF";   // 相撞标记颜色

    string GetName(std::string x);
    string GetScoreBoard();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string score_Board = GetScoreBoard();
        string X_Board = "<tr><td align=\"left\" colspan=" + to_string(option().PlayerNum() + 1) + "><font size=5>· 本轮 X 为：" + to_string(X[round_ - 1]) + "</font></td></tr>";

        reply() << Markdown(T_Board + score_Board + Board + X_Board + "</table>" + status_string);

        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + std::to_string(round) + " / " + std::to_string(main_stage.X.size()) + " 回合",
                MakeStageCommand("提交数字", &RoundStage::Submit_, ArithChecker<int64_t>(1, main_stage.X.size(), "数字"))) {}

  private:
    virtual void OnStageBegin() override
    {
        if (main_stage().round_ < GET_OPTION_VALUE(option(), N)) {
            Boardcast() << "本轮 X 为 " + to_string(main_stage().X[main_stage().round_ - 1]) + "\n请玩家私信选择数字。";
            StartTimer(GET_OPTION_VALUE(option(), 时限));
        } else {
            HandleUnreadyPlayers_();
        }
    }

    string GetLeftNums(const PlayerID pid)
    {
        string left_nums = "none";
        for (int i = 0; i < main_stage().player_leftnum_[pid].size(); i++) {
            if (i == 0)
                left_nums = "\n" + to_string(main_stage().player_leftnum_[pid][i]);
            else
                left_nums += " " + to_string(main_stage().player_leftnum_[pid][i]);
        }
        return left_nums;
    }

    void PlayerSelectNum(const PlayerID pid, const int select)
    {
        main_stage().player_select_[pid] = select;
        main_stage().player_leftnum_[pid].erase(remove(main_stage().player_leftnum_[pid].begin(), main_stage().player_leftnum_[pid].end(), select), main_stage().player_leftnum_[pid].end());
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t num)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判提交数字";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 您本回合已经提交过数字了";
            return StageErrCode::FAILED;
        }
        
        if (count(main_stage().player_leftnum_[pid].begin(), main_stage().player_leftnum_[pid].end(), num) == 0) {
            reply() << "[错误] 您已经使用过此数字了，剩余可用数字：" + GetLeftNums(pid);
            return StageErrCode::FAILED;
        }
        PlayerSelectNum(pid, num);
        reply() << "提交成功，剩余可用数字：" + GetLeftNums(pid);
        return StageErrCode::READY;
    }

    void HandleUnreadyPlayers_()
    {
        for (int pid = 0; pid < option().PlayerNum(); pid++) {
            if (!IsReady(pid)) {
                PlayerSelectNum(pid, main_stage().player_leftnum_[pid][main_stage().player_leftnum_[pid].size() - 1]);
                SetReady(pid);
            }
        }
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HookUnreadyPlayers();
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
        if (!IsReady(pid)) {
            int rd = rand() % main_stage().player_leftnum_[pid].size();
            PlayerSelectNum(pid, main_stage().player_leftnum_[pid][rd]);
            return StageErrCode::READY;
        }
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        if (main_stage().round_ < GET_OPTION_VALUE(option(), N)) {
            Boardcast() << "第 " + to_string(main_stage().round_) + " 回合结束，下面公布赛况";
        } else {
            Boardcast() << "仅剩最后一张，将自动结算";
        }
        calc();
        return StageErrCode::CHECKOUT;
    }

    void calc() {
        vector<int> win(1, 0);
        vector<int> lose(1, 0);
		for (int i = 1; i < option().PlayerNum(); i++) {
            if (main_stage().player_select_[i] > main_stage().player_select_[win[0]]) {
                win.clear();
				win.push_back(i);
			} else if (main_stage().player_select_[i] == main_stage().player_select_[win[0]]) {
                win.push_back(i);
			}
			if (main_stage().player_select_[i] < main_stage().player_select_[lose[0]]) {
                lose.clear();
				lose.push_back(i);
			} else if (main_stage().player_select_[i] == main_stage().player_select_[lose[0]]) {
                lose.push_back(i);
			}
		}

        if (win.size() == 1) {
            main_stage().player_scores_[win[0]] += main_stage().X[main_stage().round_ - 1];
        } else if (lose.size() == 1) {
            main_stage().player_scores_[lose[0]] -= main_stage().X[main_stage().round_ - 1];
        }

        // 每回合初始化赛况html
        main_stage().status_string = "";
        if (main_stage().round_ < GET_OPTION_VALUE(option(), N) - 1) {
            main_stage().status_string += "<table style=\"text-align:center\"><tr><th colspan=" + to_string(GET_OPTION_VALUE(option(), N) + 1) + ">剩余数字总览</th></tr><tr><th>【X】</th>";
            for (int i = 1; i <= GET_OPTION_VALUE(option(), N); i++) {
                main_stage().status_string += "<td>";
                if (count(main_stage().X.begin() + main_stage().round_ + 1, main_stage().X.end(), i)) {
                    main_stage().status_string += to_string(i);
                }
                main_stage().status_string += "</td>";
            }
            main_stage().status_string += "</tr>";
            for (int pid = 0; pid < option().PlayerNum(); pid++) {
                main_stage().status_string += "<tr><th>" + to_string(pid + 1) + "号：</th>";
                for (int i = 1; i <= GET_OPTION_VALUE(option(), N); i++) {
                    main_stage().status_string += "<td>";
                    if (count(main_stage().player_leftnum_[pid].begin(), main_stage().player_leftnum_[pid].end(), i)) {
                        main_stage().status_string += to_string(i);
                    }
                    main_stage().status_string += "</td>";
                }
                main_stage().status_string += "</tr>";
            }
            main_stage().status_string += "</table>";
        }

        // board
        string score_Board = main_stage().GetScoreBoard();

        string b = "<tr><td bgcolor=\"" + main_stage().X_color + "\">" + to_string(main_stage().X[main_stage().round_ - 1]) + "</td>";
        for (int i = 0; i < option().PlayerNum(); i++) {
            string color = "";
            if ((win.size() > 1 && count(win.begin(), win.end(), i)) || (win.size() > 1 && lose.size() > 1 && count(lose.begin(), lose.end(), i))) {
                color = " bgcolor=\"" + main_stage().collision_color + "\"";
            } else if (win.size() == 1 && win[0] == i) {
                color = " bgcolor=\"" + main_stage().win_color + "\"";
            } else if (win.size() > 1 && lose.size() == 1 && lose[0] == i) {
                color = " bgcolor=\"" + main_stage().lose_color + "\"";
            }
            b += "<td" + color +">" + to_string(main_stage().player_select_[i]) + "</td>";
        }
        b += "</tr>";
        main_stage().Board += b;

        string X_Board = "";
        if (main_stage().round_ < GET_OPTION_VALUE(option(), N)) {
            X_Board = "<tr><td align=\"left\" colspan=" + to_string(option().PlayerNum() + 1) + "><font size=5>· 本轮 X 为：" + to_string(main_stage().X[main_stage().round_]) + "</font></td></tr>";
        }

        Boardcast() << Markdown(main_stage().T_Board + score_Board + main_stage().Board + X_Board + "</table>" + main_stage().status_string);

        if (win.size() == 1) {
            Boardcast() << "最大者为\n" + PlayerName(win[0]) + "\n获得 " + to_string(main_stage().X[main_stage().round_ - 1]) + " 分";
        } else if (lose.size() == 1) {
            Boardcast() << "最小者为\n" + PlayerName(lose[0]) + "\n失去 " + to_string(main_stage().X[main_stage().round_ - 1]) + " 分";
        } else {
            Boardcast() << "最大最小者均不唯一，分数不变";
        }

        for (int i = 0; i < option().PlayerNum(); i++) {
            main_stage().player_last_scores_[i] = main_stage().player_scores_[i];
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
    score_Board += "</tr><tr><th bgcolor=\""+ X_color +"\">X</th>";
    for (int i = 0; i < option().PlayerNum(); i++) {
        score_Board += "<td bgcolor=\""+ score_color +"\">";
        score_Board += to_string(player_last_scores_[i]);
        if (player_scores_[i] > player_last_scores_[i]) {
            score_Board += "<font color=\"#00AC30\">+" + to_string(X[round_ - 1]);
        } else if (player_scores_[i] < player_last_scores_[i]) {
            score_Board += "<font color=\"#FF0000\">-" + to_string(X[round_ - 1]);
        }
        if (player_scores_[i] != player_last_scores_[i]) {
            score_Board += "</font>";
        }
        score_Board += "</td>";
    }
    score_Board += "</tr>";
    return score_Board;
}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    srand((unsigned int)time(NULL));
    player_leftnum_.resize(option().PlayerNum());

    for (int i = 0; i < option().PlayerNum(); i++) {
        player_leftnum_[i].resize(GET_OPTION_VALUE(option(), N));
    }
    for (int i = 0; i < option().PlayerNum(); i++) {
        for (int j = 1; j <= GET_OPTION_VALUE(option(), N); j++) {
            player_leftnum_[i][j-1] = j;
        }
    }
    for (int i = 1; i <= GET_OPTION_VALUE(option(), N); i++) {
        X.push_back(i);
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(X.begin(), X.end(), g);

    T_Board += "<table><tr>";
    for (int i = 0; i < option().PlayerNum(); i++) {
        T_Board += "<th>" + to_string(i + 1) + " 号： " + GetName(PlayerName(i)) + "　</th>";
        if (i % 4 == 3) T_Board += "</tr><tr>";
    }
    T_Board += "</tr><br>";

    T_Board += "<table style=\"text-align:center\"><tbody>";
    T_Board += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:70px;\">序号</th>";
    for (int i = 0; i < option().PlayerNum(); i++) {
        T_Board += "<th style=\"width:60px;\">";
        T_Board += to_string(i + 1) + " 号";
        T_Board += "</th>";
    }
    T_Board += "</tr>";

    string score_Board = GetScoreBoard();
    string X_Board = "<tr><td align=\"left\" colspan=" + to_string(option().PlayerNum() + 1) + "><font size=5>· 本轮 X 为：" + to_string(X[round_]) + "</font></td></tr>";

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < option().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + PlayerName(i);
        if (i != (int)option().PlayerNum() - 1) {
            PreBoard += "\n";
        }
    }

    Boardcast() << PreBoard;
    Boardcast() << Markdown(T_Board + score_Board + X_Board + "</table>");
    Boardcast() << "[提示] 本局游戏 N 为：" + to_string(GET_OPTION_VALUE(option(), N));

    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= GET_OPTION_VALUE(option(), N)) {
        return std::make_unique<RoundStage>(*this, round_);
    }
    // Returning empty variant means the game will be over.
    return {};
}

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"
