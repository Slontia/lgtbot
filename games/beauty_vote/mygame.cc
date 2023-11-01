// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <bits/stdc++.h>

#include "game_framework/game_stage.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "美人投票"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 1; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "铁蛋";
const std::string k_description = "选择恰当的数字，尽可能接近平均数的游戏";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const
{
    return "共 " + to_string(GET_VALUE(回合数)) + " 回合，每回合超时时间 " + to_string(GET_VALUE(时限)) + " 秒";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << PlayerNum();
        return false;
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
        alive_(0),
        player_scores_(option.PlayerNum(), 0),
        player_hp_(option.PlayerNum(), 10),
        player_select_(option.PlayerNum(), 0),
        player_out_(option.PlayerNum(), 0) {}

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
    
    vector<int64_t> player_scores_;
    int round_;
    
    vector<int64_t> player_hp_;   // 生命值
    vector<int64_t> player_select_;   // 提交数字
    vector<int64_t> player_out_;   // 淘汰玩家
    string T_Board = "";   // 表头
    string Board = "";   // 赛况
    int alive_;   // 存活数
    double x;   // X
    double x1;   // 上一轮x
    int on_crash = 0;   // 判断启用撞车
    int on_red = 0;   // 判断启用红心
    int on_last = 0;   // 判断决胜规则

    // 颜色样式
    string HP_color = "9CCAF0";   // 生命值底色
    string win_color = "BAFFA8";   // 命中颜色
    string X_color = "DCDCDC";   // X栏底色
    string crash_color = "FFA07A";   // 撞车颜色
    string red_color = "35FF00";   // 红心颜色

    string Specialrules(int win_size, int red_size, bool onred);
    string GetName(std::string x);

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string HP_Board = "";
        HP_Board += "<tr bgcolor=\""+ HP_color +"\"><th>血量</th>";
        for (int i = 0; i < option().PlayerNum(); i++) {
            HP_Board += "<td>";
            if (player_out_[i] == 0) {
                HP_Board += to_string(player_hp_[i]);
            } else {
                HP_Board += "出局";
            }
            HP_Board += "</td>";
        }
        HP_Board += "<td>X</td></tr>";

        string specialrule = "<table><tr><th style=\"width:500px;\">已开启的特殊规则：</th></tr>";
        int n = 0;
        if (on_crash == 1) {
            specialrule += "<tr><td>撞车：如果有玩家提交了相同的数字，";
            if (GET_OPTION_VALUE(option(),撞车伤害)) {
                specialrule += "这些玩家生命值额外 -1，且";
            } else {
                specialrule += "则";
            }
            specialrule += "不计入本回合获胜玩家。</td></tr>";
            n = 1;
        }
        if (on_red == 1) {
            if (n == 1) { specialrule += "\n"; }
            specialrule += "<tr><td>红心：当玩家选择的数字和 X 四舍五入后的数字一样时，视为该玩家命中红心，其他玩家生命值 -2。</td></tr>";
            n = 1;
        }
        if (on_last == 1) {
            if (n == 1) { specialrule += "\n"; }
            specialrule += "<tr><td>当前玩家数小于等于3人，自动追加一条规则：当有玩家出 0 时，当回合出 " + to_string(GET_OPTION_VALUE(option(),最大数字)) + " 的玩家获胜。</td></tr>";
            n = 1;
        }

        reply() << Markdown(T_Board + HP_Board + Board + "</table>");
        reply() << Markdown(specialrule + (n == 0 ? "<tr><td>无</tr></td>" : "") + "</table>");
        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + std::to_string(round) + " 回合，请玩家私信提交数字。",
                MakeStageCommand("提交数字", &RoundStage::Submit_, ArithChecker<int64_t>(0, 10000, "数字")))
    {
    }

    virtual void OnStageBegin() override
    {
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (IsReady(i) == false && !main_stage().player_out_[i]) {
                Boardcast() << "玩家 " << main_stage().PlayerName(i) << " 行动超时，已被淘汰";
                main_stage().player_hp_[i] = 0;
                main_stage().player_select_[i] = -1;
            }
        }

        RoundStage::calc();
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        main_stage().player_hp_[pid] = 0;
        main_stage().player_select_[pid] = -1;
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        int num, x0;
        int max = GET_OPTION_VALUE(option(),最大数字);
        double x = main_stage().x * 0.8;

        if (max >= 100) {

            if (main_stage().round_ == 1) {

                if (rand() % 10 < 8) {
                    num = rand() % (int)(max * 0.15) + (int)(max * 0.13);
                } else {
                    num = rand() % (max + 1);
                }

            } else {

                if (main_stage().x < (max * 0.07) && rand() % 10 < 5) {
                    num = rand() % (int)(max * 0.10) + (int)(max * 0.15);
                } else if (main_stage().x > (max * 0.23)) {
                    num = rand() % (int)(max * 0.09) + (int)(max * 0.07);
                } else {
                    if (main_stage().round_ == 2 || rand() % 10 < 7) {
                        num = rand() % (int)(max * 0.13) + (int)x - (int)(max * 0.06);
                    } else {
                        if (main_stage().x1 == 0) {
                            x0 = 0;
                        } else {
                            x0 = (int)(main_stage().x * main_stage().x / main_stage().x1);
                        }
                        num = rand() % (int)(max * 0.13) + x0 - (int)(max * 0.06);
                    }
                }

            }

            // low HP
            int lowhp_count = 0;
            for (int i = 0; i < option().PlayerNum(); i++) {
                if (main_stage().player_hp_[i] <= 2 && main_stage().player_hp_[i] > 0) {
                    lowhp_count++;
                }
            }
            if (main_stage().player_hp_[pid] <= 2 && lowhp_count <= 6) {
                int r = rand() % 4;
                for (int i = (int)x - (int)(max * 0.02); i <= max; i += (int)(max * 0.01)) {
                    int c = 0;
                    for (int j = 0; j < pid; j++) {
                        if (main_stage().player_select_[j] == i + r) {
                            c = 1;
                            break;
                        }
                    }
                    if (c == 0) {
                        num = i + r;
                        break;
                    }
                }
            }

            // limit
            if (num < 0)  {
                num = rand() % (int)(max * 0.05) + (int)(max * 0.95) + 1;
            } else if (num > max) {
                num = rand() % (int)(max * 0.05);
            }

        } else {
            // 小于100
            num = rand() % (max + 1);
        }

        // 2
        if (main_stage().alive_ == 2) {
            int r = rand() % 7;
            if (r == 0) num = max * 0.6666;
            else if (r <= 2) num = 0;
            else if (r <= 4) num = 1;
            else if (r <= 6) num = max;
        }

        main_stage().player_select_[pid] = num;

        return StageErrCode::READY;
    }

    virtual void OnAllPlayerReady() override
    {
        RoundStage::calc();
    }

    void calc() {
        int sum;
        double avg;
        sum = 0;

        main_stage().x1 = main_stage().x;   // x1

        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_hp_[i] <= 0) {   // 退出和超时不计入总和
                if (main_stage().player_out_[i] == 0) {
                    main_stage().player_out_[i] = 1;
                    Eliminate(i);
                    main_stage().alive_--;
                }
            }
            if (main_stage().player_select_[i] >= 0) {
                sum += main_stage().player_select_[i];
            }
        }
        avg = sum / (double)main_stage().alive_;
        main_stage().x = round(avg * 0.8 * 100) / 100;

        // crash
        int crash[option().PlayerNum()+1] = {0};
        int is_crash = 0;
        if (main_stage().on_crash == 1) {
            for (int i = 0; i < option().PlayerNum(); i++) {
                if (main_stage().player_select_[i] >= 0) {
                    for (int j = i + 1; j < option().PlayerNum(); j++) {
                        if (crash[j] == 0) {
                            if (main_stage().player_select_[i] == main_stage().player_select_[j]) {
                                crash[i] = crash[j] = 1;
                                is_crash = 1;
                            }
                        }
                    }
                }
            }
            if (GET_OPTION_VALUE(option(),撞车伤害)) {
                for (int i = 0; i < option().PlayerNum(); i++) {
                    if (crash[i] == 1) {
                        main_stage().player_hp_[i]--;
                    }
                }
            }
        }

        // gap
        double min_gap = 10000;
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_select_[i] >= 0 && crash[i] == 0) {
                double gap = fabs(main_stage().player_select_[i] - main_stage().x);
                if (min_gap > gap) {
                    min_gap = gap;
                }
            }
        }

        // win
        vector<int> win;
        vector<int> red;
        bool onred = false;
        vector<int> flag0, flag100;
        if (main_stage().alive_ <= 3) {
            for (int i = 0; i < option().PlayerNum(); i++) {
                if (main_stage().player_select_[i] == 0 && crash[i] == 0) {
                    flag0.push_back(i + 1);
                }
                if (main_stage().player_select_[i] == GET_OPTION_VALUE(option(),最大数字) && crash[i] == 0) {
                    flag100.push_back(i + 1);
                }
            }
        }
        if (flag0.size() && flag100.size()) {
            for (int i = 0; i < flag100.size(); i++) {
                win.push_back(flag100[i] - 1);
            }
            for (int i = 0; i < win.size(); i++) {
                main_stage().player_hp_[win[i]]++;
                main_stage().player_scores_[win[i]]++;
            }
        } else {
            for (int i = 0; i < option().PlayerNum(); i++) {
                if (main_stage().player_select_[i] >= 0 && crash[i] == 0) {
                    double gap = fabs(main_stage().player_select_[i] - main_stage().x);
                    if (fabs(min_gap - gap) < 0.005) {
                        main_stage().player_hp_[i]++;
                        main_stage().player_scores_[i]++;

                        int x0 = (int)round(main_stage().x);
                        if (x0 == main_stage().player_select_[i]) {   // red
                            if (main_stage().on_red == 1) {
                                main_stage().player_scores_[i]++;
                                main_stage().player_hp_[i]++;
                                red.push_back(i);
                            } else {
                                win.push_back(i);
                                onred = true;
                            }
                        } else {
                            win.push_back(i);
                        }
                    }
                }
            }
        }
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (win.size() > 0 || red.size() > 0) {
                main_stage().player_hp_[i]--;
            }
            if (red.size() > 0) {
                main_stage().player_hp_[i]--;
            }
            if (main_stage().player_hp_[i] < 0) main_stage().player_hp_[i] = 0;
        }

        string win_p = "";
        for (int i = 0; i < win.size(); i++) {
            win_p +=  "\n" + PlayerName(win[i]);
        }
        string win_r = "";
        for (int i = 0; i < red.size(); i++) {
            win_r +=  "\n" + PlayerName(red[i]);
        }

        // board
        string HP_Board = "";
        HP_Board += "<tr bgcolor=\""+ main_stage().HP_color +"\"><th>血量</th>";
        for (int i = 0; i < option().PlayerNum(); i++) {
            HP_Board += "<td>";
            if (main_stage().player_out_[i] == 0) {
                HP_Board += to_string(main_stage().player_hp_[i]);
            } else {
                HP_Board += "出局";
            }
            HP_Board += "</td>";
        }
        HP_Board += "<td>X</td></tr>";

        string b = "<tr><td>R" + to_string(main_stage().round_) + "</td>";
        for (int i = 0; i < option().PlayerNum(); i++) {
            string color = "";
            vector<int>::iterator fwin = find(win.begin(), win.end(), i);
            if (fwin != win.end()) color = " bgcolor=\"" + main_stage().win_color + "\"";
            vector<int>::iterator fred = find(red.begin(), red.end(), i);
            if (fred != red.end()) color = " bgcolor=\"" + main_stage().red_color + "\"";
            if (crash[i] == 1) {
                color = " bgcolor=\"" + main_stage().crash_color + "\"";
            }
            b += "<td" + color +">";
            if (main_stage().player_out_[i] == 1) {
                b += " ";
            } else {
                b += to_string(main_stage().player_select_[i]);
            }
            b += "</td>";
        }
        char xchar[10];
        sprintf(xchar, "%.2lf", main_stage().x);
        string x = xchar;
        b += "<td bgcolor=\""+ main_stage().X_color +"\">" + x + "</td></tr>";
        main_stage().Board += b;

        Boardcast() << Markdown(main_stage().T_Board + HP_Board + main_stage().Board + "</table>");

        if (is_crash == 1) {
            Boardcast() << "有玩家撞车，" << (GET_OPTION_VALUE(option(),撞车伤害) ? "生命值额外 -1，且" : "") << "不计入本回合获胜玩家";
        }
        if (flag0.size() && flag100.size()) {
            Boardcast() << "有玩家出 0，所以玩家" + win_p + "\n出 " + to_string(GET_OPTION_VALUE(option(),最大数字)) + " 获胜";
        } else if (win.size() == 0 && red.size() == 0) {
            Boardcast() << "本回合 X 的值是 " + x + "，没有玩家获胜";
        } else {
            if (red.size() > 0) {
                Boardcast() << "本回合 X 的值是 " + x + "，玩家" + win_r + "\n命中红心！" << (win.size() > 0 ? "\n同时玩家" + win_p + " 获胜" : "其余玩家生命值 -2");
            } else {
                Boardcast() << "本回合 X 的值是 " + x + "，获胜的玩家为" + win_p;
            }
        }

        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_hp_[i] <= 0) {
                main_stage().player_select_[i] = -1;
                if (main_stage().player_out_[i] == 0) {
                    main_stage().player_out_[i] = 1;
                    Eliminate(i);
                    main_stage().alive_--;
                }
            } else {
                main_stage().player_scores_[i]++;
            }
        }

        // 特殊规则
        string specialrule = main_stage().Specialrules(win.size(), red.size(), onred);
        if (specialrule != "") {
            Boardcast() << specialrule;
        }

        win.clear();
        red.clear();
    }

  private:
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
        if (num < 0 || num > GET_OPTION_VALUE(option(), 最大数字)) {
            reply() << "[错误] 不合法的数字，提交的数字应在 0 - " + to_string(GET_OPTION_VALUE(option(),最大数字)) + " 之间";
            return StageErrCode::FAILED;
        }
        return SubmitInternal_(pid, reply, num);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, const int64_t num)
    {
        main_stage().player_select_[pid] = num;
        reply() << "提交数字成功";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }
};

string MainStage::Specialrules(int win_size, int red_size, bool onred)
{
    string specialrule = "";
    int n = 0;

    if ((round_ == GET_OPTION_VALUE(option(), 撞车) - 1 || GET_OPTION_VALUE(option(), 撞车) == 1 || win_size > 1 || red_size > 1) && on_crash == 0 && GET_OPTION_VALUE(option(), 撞车)) {
        specialrule += "新特殊规则——撞车：如果有玩家提交了相同的数字，";
        if (GET_OPTION_VALUE(option(),撞车伤害)) {
            specialrule += "这些玩家生命值额外 -1，且";
        } else {
            specialrule += "则";
        }
        specialrule += "不计入本回合获胜玩家。";
        on_crash = 1;
        n = 1;
    }
    if ((round_ == GET_OPTION_VALUE(option(), 红心) - 1 || GET_OPTION_VALUE(option(), 红心) == 1 || onred) && on_red == 0 && GET_OPTION_VALUE(option(), 红心)) {
        if (n == 1) { specialrule += "\n"; }
        specialrule += "新特殊规则——红心：当玩家选择的数字和 X 四舍五入后的数字一样时，视为该玩家命中红心，其他玩家生命值 -2。";
        on_red = 1;
        n = 1;
    }
    if (alive_ > 1 && alive_ <= 3 && on_last == 0) {
        if (n == 1) { specialrule += "\n"; }
        specialrule += "当前玩家数小于等于3人，自动追加一条规则：当有玩家出 0 时，当回合出 " + to_string(GET_OPTION_VALUE(option(),最大数字)) + " 的玩家获胜。";
        on_last = 1;
        n = 1;
    }
    
    return specialrule;
}

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

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    srand((unsigned int)time(NULL));
    alive_ = option().PlayerNum();

    for (int i = 0; i < option().PlayerNum(); i++) {
        player_hp_[i] = GET_OPTION_VALUE(option(),血量);
    }

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
    T_Board += "<th style=\"width:70px;\">结果</th></tr>";

    string HP_Board = "";
    HP_Board += "<tr bgcolor=\""+ HP_color +"\"><th>血量</th>";
    for (int i = 0; i < option().PlayerNum(); i++) {
        HP_Board += "<td>";
        HP_Board += to_string(GET_OPTION_VALUE(option(),血量));
        HP_Board += "</td>";
    }
    HP_Board += "<td>X</td></tr>";

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < option().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + PlayerName(i);

        if (i != (int)option().PlayerNum() - 1) {
        PreBoard += "\n";
        }
    }

    Boardcast() << PreBoard;
    Boardcast() << Markdown(T_Board + HP_Board + "</table>");

    string specialrule = Specialrules(0, 0, false);
    if (specialrule != "") {
        Boardcast() << specialrule;
    }

    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= GET_OPTION_VALUE(option(), 回合数) && alive_ > 1) {
        return std::make_unique<RoundStage>(*this, round_);
    }

    if ((++round_) > GET_OPTION_VALUE(option(), 回合数)) {
        Boardcast() << "达到最大回合数限制";
    } else {
        int maxHP = 0;
        for(int i = 0; i < option().PlayerNum(); i++)
        {
            maxHP = max(maxHP, (int)player_hp_[i]);
        }
        if (alive_ == 0) {
            Boardcast() << "游戏结束，平局！";
        } else {
            for(int i = 0; i < option().PlayerNum(); i++)
            {
                if (maxHP == player_hp_[i])
                {
                    player_scores_[i] += 5;
                    Boardcast() << "游戏结束，恭喜胜者 " + PlayerName(i) +" ！";
                }
            }
        }
    }
    // Returning empty variant means the game will be over.
    return {};
}

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"
