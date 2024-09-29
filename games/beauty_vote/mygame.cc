// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <vector>

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
    .name_ = "美人投票", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "选择恰当的数字，尽可能接近平均数的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 1; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
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
        alive_(0),
        player_scores_(Global().PlayerNum(), 0),
        player_hp_(Global().PlayerNum(), 10),
        player_select_(Global().PlayerNum(), 0),
        player_out_(Global().PlayerNum(), 0) {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

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
    const string HP_color = "9CCAF0";   // 生命值底色
    const string win_color = "BAFFA8";   // 命中颜色
    const string X_color = "DCDCDC";   // X栏底色
    const string crash_color = "FFA07A";   // 撞车颜色
    const string red_color = "35FF00";   // 红心颜色

    const int image_width = Global().PlayerNum() < 7 ? 500 : Global().PlayerNum() < 11 ? Global().PlayerNum() * 60 + 110 : (Global().PlayerNum() < 17 ? Global().PlayerNum() * 55 + 90 : Global().PlayerNum() * 40 + 70);

    string Specialrules(const int win_size, const int red_size, const bool onred, const bool is_status);
    string GetName(std::string x);

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string HP_Board = "";
        HP_Board += "<tr bgcolor=\""+ HP_color +"\"><th>血量</th>";
        for (int i = 0; i < Global().PlayerNum(); i++) {
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
        specialrule += Specialrules(0, 0, false, true);

        reply() << Markdown(T_Board + HP_Board + Board + "</table>", image_width);
        reply() << Markdown(specialrule + "</table>");
        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合，请玩家私信提交数字。",
                MakeStageCommand(*this, "提交数字", &RoundStage::Submit_, ArithChecker<int64_t>(0, 10000, "数字")))
    {
    }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Global().IsReady(i) == false && !Main().player_out_[i]) {
                Global().Boardcast() << "玩家 " << Main().Global().PlayerName(i) << " 行动超时，已被淘汰";
                Main().player_hp_[i] = 0;
                Main().player_select_[i] = -1;
            }
        }

        RoundStage::calc();
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_hp_[pid] = 0;
        Main().player_select_[pid] = -1;
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        int num, x0;
        const int max = GAME_OPTION(最大数字);
        const double x = Main().x * 0.8;

        if (max >= 100) {

            if (Main().round_ == 1) {

                if (rand() % 10 < 8) {
                    num = rand() % (int)(max * 0.15) + (int)(max * 0.13);
                } else {
                    num = rand() % (max + 1);
                }

            } else {

                if (Main().x < (max * 0.07) && rand() % 10 < 5) {
                    num = rand() % (int)(max * 0.10) + (int)(max * 0.15);
                } else if (Main().x > (max * 0.23) && (Main().on_crash == 0 || rand() % 10 < 3)) {
                    num = rand() % (int)(max * 0.11) + (int)(max * 0.07);
                } else if (Main().on_crash == 1 && Main().alive_ >= 8 && max <= 200) {
                    if (rand() % 10 < 2) {
                        num = rand() % (int)(max * 0.51) + (int)(max * 0.35);
                    } else {
                        num = rand() % (int)(max * 0.16) + (int)(max * 0.15);
                    }
                } else {
                    if (Main().round_ == 2 || rand() % 10 < 7) {
                        num = rand() % (int)(max * 0.13) + (int)x - (int)(max * 0.06);
                    } else {
                        if (Main().x1 == 0) {
                            x0 = 0;
                        } else {
                            x0 = (int)(Main().x * Main().x / Main().x1);
                        }
                        num = rand() % (int)(max * 0.13) + x0 - (int)(max * 0.06);
                    }
                }

            }

            // low HP
            int lowhp_count = 0;
            for (int i = 0; i < Global().PlayerNum(); i++) {
                if (Main().player_hp_[i] <= 2 && Main().player_hp_[i] > 0) {
                    lowhp_count++;
                }
            }
            if (Main().player_hp_[pid] <= 2 && lowhp_count <= 6) {
                int r = rand() % 4;
                for (int i = (int)x - (int)(max * 0.02); i <= max; i += (int)(max * 0.01)) {
                    int c = 0;
                    for (int j = 0; j < pid; j++) {
                        if (Main().player_select_[j] == i + r) {
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
        if (Main().alive_ == 2) {
            int r = rand() % 7;
            if (r == 0) num = max * 0.6666;
            else if (r <= 2) num = 0;
            else if (r <= 4) num = 1;
            else if (r <= 6) num = max;
        }

        Main().player_select_[pid] = num;

        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        RoundStage::calc();
        return StageErrCode::CHECKOUT;
    }

    void calc() {
        int sum = 0;
        double avg;

        Main().x1 = Main().x;   // x1

        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Main().player_hp_[i] <= 0) {   // 退出和超时不计入总和
                if (Main().player_out_[i] == 0) {
                    Main().player_out_[i] = 1;
                    Global().Eliminate(i);
                    Main().alive_--;
                }
            }
            if (Main().player_select_[i] >= 0) {
                sum += Main().player_select_[i];
            }
        }
        if (Main().alive_ != 0) {
            avg = sum / (double)Main().alive_;
        } else {
            avg = 0;
        }
        Main().x = round(avg * 0.8 * 100) / 100;

        // crash
        int crash[Global().PlayerNum()+1] = {0};
        int is_crash = 0;
        if (Main().on_crash == 1) {
            for (int i = 0; i < Global().PlayerNum(); i++) {
                if (Main().player_select_[i] >= 0) {
                    for (int j = i + 1; j < Global().PlayerNum(); j++) {
                        if (Main().player_select_[j] >= 0 && (crash[j] == 0 || GAME_OPTION(撞车范围) > 0) &&
                           ((fabs(Main().player_select_[i] - Main().player_select_[j]) <= GAME_OPTION(撞车范围) && Main().alive_ > 2) || Main().player_select_[i] == Main().player_select_[j])
                        ) {
                            crash[i] = crash[j] = 1;
                            is_crash = 1;
                        }
                    }
                }
            }
            if (GAME_OPTION(撞车伤害)) {
                for (int i = 0; i < Global().PlayerNum(); i++) {
                    if (crash[i] == 1) {
                        Main().player_hp_[i]--;
                    }
                }
            }
        }

        // gap
        double min_gap = 10000;
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Main().player_select_[i] >= 0 && crash[i] == 0) {
                double gap = fabs(Main().player_select_[i] - Main().x);
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
        if (Main().alive_ <= 3) {
            for (int i = 0; i < Global().PlayerNum(); i++) {
                if (Main().player_select_[i] == 0) {
                    flag0.push_back(i);
                }
                if (Main().player_select_[i] == GAME_OPTION(最大数字) && crash[i] == 0) {
                    flag100.push_back(i);
                }
            }
        }
        if (flag0.size() && flag100.size()) {
            // Eliminate 0
            if (GAME_OPTION(淘汰规则)) {
                for (int i = 0; i < flag0.size(); i++) {
                    Main().player_hp_[flag0[i]] = 0;
                    crash[flag0[i]] = 1;
                }
            }

            for (int i = 0; i < flag100.size(); i++) {
                win.push_back(flag100[i]);
            }
            for (int i = 0; i < win.size(); i++) {
                Main().player_hp_[win[i]]++;
                Main().player_scores_[win[i]]++;
            }
        } else {
            for (int i = 0; i < Global().PlayerNum(); i++) {
                if (Main().player_select_[i] >= 0 && crash[i] == 0) {
                    double gap = fabs(Main().player_select_[i] - Main().x);
                    if (fabs(min_gap - gap) < 0.005) {
                        Main().player_hp_[i]++;
                        Main().player_scores_[i]++;

                        int x0 = (int)round(Main().x);
                        if (x0 == Main().player_select_[i]) {   // red
                            if (Main().on_red == 1) {
                                Main().player_scores_[i]++;
                                Main().player_hp_[i]++;
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
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (win.size() > 0 || red.size() > 0) {
                Main().player_hp_[i]--;
            }
            if (red.size() > 0) {
                Main().player_hp_[i]--;
            }
            if (Main().player_hp_[i] < 0) Main().player_hp_[i] = 0;
        }

        string win_p = "";
        for (int i = 0; i < win.size(); i++) {
            win_p +=  "\n" + Global().PlayerName(win[i]);
        }
        string win_r = "";
        for (int i = 0; i < red.size(); i++) {
            win_r +=  "\n" + Global().PlayerName(red[i]);
        }

        // board
        string HP_Board = "";
        HP_Board += "<tr bgcolor=\""+ Main().HP_color +"\"><th>血量</th>";
        for (int i = 0; i < Global().PlayerNum(); i++) {
            HP_Board += "<td>";
            if (Main().player_out_[i] == 0) {
                HP_Board += to_string(Main().player_hp_[i]);
            } else {
                HP_Board += "出局";
            }
            HP_Board += "</td>";
        }
        HP_Board += "<td>X</td></tr>";

        string b = "<tr><td>R" + to_string(Main().round_) + "</td>";
        for (int i = 0; i < Global().PlayerNum(); i++) {
            string color = "";
            vector<int>::iterator fwin = find(win.begin(), win.end(), i);
            if (fwin != win.end()) color = " bgcolor=\"" + Main().win_color + "\"";
            vector<int>::iterator fred = find(red.begin(), red.end(), i);
            if (fred != red.end()) color = " bgcolor=\"" + Main().red_color + "\"";
            if (crash[i] == 1) {
                color = " bgcolor=\"" + Main().crash_color + "\"";
            }
            b += "<td" + color +">";
            if (Main().player_out_[i] == 1) {
                b += " ";
            } else {
                b += to_string(Main().player_select_[i]);
            }
            b += "</td>";
        }
        char xchar[10];
        sprintf(xchar, "%.2lf", Main().x);
        string x = xchar;
        b += "<td bgcolor=\""+ Main().X_color +"\">" + x + "</td></tr>";
        Main().Board += b;

        Global().Boardcast() << Markdown(Main().T_Board + HP_Board + Main().Board + "</table>", Main().image_width);

        if (is_crash == 1) {
            Global().Boardcast() << "有玩家撞车，" << (GAME_OPTION(撞车伤害) ? "生命值额外 -1，且" : "") << "不计入本回合获胜玩家";
        }
        if (flag0.size() && flag100.size()) {
            Global().Boardcast() << "有玩家出 0，所以玩家" + win_p + "\n出 " + to_string(GAME_OPTION(最大数字)) + " 获胜" << (GAME_OPTION(淘汰规则) ? "，选 0 的玩家被淘汰" : "");
        } else if (win.size() == 0 && red.size() == 0) {
            Global().Boardcast() << "本回合 X 的值是 " + x + "，没有玩家获胜";
        } else {
            if (red.size() > 0) {
                Global().Boardcast() << "本回合 X 的值是 " + x + "，玩家" + win_r + "\n命中红心！" << (win.size() > 0 ? "\n同时玩家" + win_p + " 获胜" : "其余玩家生命值 -2");
            } else {
                Global().Boardcast() << "本回合 X 的值是 " + x + "，获胜的玩家为" + win_p;
            }
        }

        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Main().player_hp_[i] <= 0) {
                Main().player_select_[i] = -1;
                if (Main().player_out_[i] == 0) {
                    Main().player_out_[i] = 1;
                    Global().Eliminate(i);
                    Main().alive_--;
                }
            } else {
                Main().player_scores_[i]++;
            }
        }

        // 特殊规则
        string specialrule = Main().Specialrules(win.size(), red.size(), onred, false);
        if (specialrule != "") {
            Global().Boardcast() << specialrule;
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
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经提交过数字了";
            return StageErrCode::FAILED;
        }
        if (num < 0 || num > GAME_OPTION(最大数字)) {
            reply() << "[错误] 不合法的数字，提交的数字应在 0 - " + to_string(GAME_OPTION(最大数字)) + " 之间";
            return StageErrCode::FAILED;
        }
        return SubmitInternal_(pid, reply, num);
    }

    AtomReqErrCode SubmitInternal_(const PlayerID pid, MsgSenderBase& reply, const int64_t num)
    {
        Main().player_select_[pid] = num;
        reply() << "提交数字成功";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }
};

string MainStage::Specialrules(const int win_size, const int red_size, const bool onred, const bool is_status)
{
    string specialrule = "";
    int n = 0;

    bool showcrash = round_ == GAME_OPTION(撞车) - 1 || GAME_OPTION(撞车) == 1 || win_size > 1 || red_size > 1;
    if ((showcrash && on_crash == 0 && GAME_OPTION(撞车) && !is_status) || (is_status && on_crash)) {
        if (is_status) {
            specialrule += "<tr><td>";
        } else {
            specialrule += "新特殊规则——";
        }
        if (GAME_OPTION(撞车范围) == 0) {
            specialrule += "撞车：如果有玩家提交了相同的数字，";
        } else {
            specialrule += "特殊撞车：如果有玩家间提交的数字相距小于等于 [" + to_string(GAME_OPTION(撞车范围)) + "]（剩余2人时范围为0），";
        }
        if (GAME_OPTION(撞车伤害)) {
            specialrule += "这些玩家生命值额外 -1，且";
        } else {
            specialrule += "则";
        }
        specialrule += "不计入本回合获胜玩家。";
        if (is_status) {
            specialrule += "</tr></td>";
        } else {
            on_crash = 1;
        }
        n = 1;
    }
    bool showred = round_ == GAME_OPTION(红心) - 1 || GAME_OPTION(红心) == 1 || onred;
    if ((showred && on_red == 0 && GAME_OPTION(红心) && !is_status) || (is_status && on_red)) {
        if (is_status) {
            specialrule += "<tr><td>";
        } else {
            if (n == 1) { specialrule += "\n"; }
            specialrule += "新特殊规则——";
        }
        specialrule += "红心：当玩家选择的数字和 X 四舍五入后的数字一样时，视为该玩家命中红心，其他玩家生命值 -2。";
        if (is_status) {
            specialrule += "</tr></td>";
        } else {
            on_red = 1;
        }
        n = 1;
    }
    if ((alive_ > 1 && alive_ <= 3 && on_last == 0 && !is_status) || (is_status && on_last)) {
        if (is_status) {
            specialrule += "<tr><td>";
        } else {
            if (n == 1) { specialrule += "\n"; }
        }
        if (GAME_OPTION(淘汰规则)) {
            specialrule += "当前玩家数小于等于3人，自动追加一条规则：当有玩家出 0 时，当回合出 " + to_string(GAME_OPTION(最大数字)) + " 的玩家获胜，选 0 的玩家被**直接淘汰**。";
        } else {
            specialrule += "当前玩家数小于等于3人，自动追加一条规则：当有玩家出 0 时，当回合出 " + to_string(GAME_OPTION(最大数字)) + " 的玩家获胜。";
        }
        if (is_status) {
            specialrule += "</tr></td>";
        } else {
            on_last = 1;
        }
        n = 1;
    }

    if (n == 0 && is_status) {
        specialrule += "<tr><td>无</tr></td>";
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

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));
    alive_ = Global().PlayerNum();

    for (int i = 0; i < Global().PlayerNum(); i++) {
        player_hp_[i] = GAME_OPTION(血量);
    }

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
    T_Board += "<th style=\"width:70px;\">结果</th></tr>";

    string HP_Board = "";
    HP_Board += "<tr bgcolor=\""+ HP_color +"\"><th>血量</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        HP_Board += "<td>";
        HP_Board += to_string(GAME_OPTION(血量));
        HP_Board += "</td>";
    }
    HP_Board += "<td>X</td></tr>";

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + Global().PlayerName(i);

        if (i != (int)Global().PlayerNum() - 1) {
        PreBoard += "\n";
        }
    }

    Global().Boardcast() << PreBoard;
    Global().Boardcast() << Markdown(T_Board + HP_Board + "</table>", image_width);

    string specialrule = Specialrules(0, 0, false, false);
    if (specialrule != "") {
        Global().Boardcast() << specialrule;
    }

    setter.Emplace<RoundStage>(*this, ++round_);
    return;
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if ((++round_) <= GAME_OPTION(回合数) && alive_ > 1) {
        setter.Emplace<RoundStage>(*this, round_);
        return;
    }

    if ((++round_) > GAME_OPTION(回合数)) {
        Global().Boardcast() << "达到最大回合数限制";
    } else {
        if (alive_ == 0) {
            Global().Boardcast() << "游戏结束，平局！";
        } else {
            int maxHP = 0;
            for(int i = 0; i < Global().PlayerNum(); i++)
            {
                maxHP = max(maxHP, (int)player_hp_[i]);
            }
            for(int i = 0; i < Global().PlayerNum(); i++)
            {
                if (maxHP == player_hp_[i])
                {
                    player_scores_[i] += 3;
                    Global().Boardcast() << "游戏结束，恭喜胜者 " + Global().PlayerName(i) +" ！";
                }
            }
        }
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

