// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <vector>
#include <map>
#include <algorithm>
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
    .name_ = "狂奔马场", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "扮演骑马手，完成目标距离并尽可能获得更高的名次",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 10; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options)
{
    return ceil(GET_OPTION_VALUE(options, 目标) / 10.0 / GET_OPTION_VALUE(options, 上限));
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 5) {
        reply() << "该游戏至少 5 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
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
        player_scores_(Global().PlayerNum(), 0),
        player_rank_(Global().PlayerNum(), Global().PlayerNum()),
        player_position_(Global().PlayerNum(), 0),
        player_maxspeed_(Global().PlayerNum(), 10),
        player_last_maxspeed_(Global().PlayerNum(), 10),
        player_select_(Global().PlayerNum(), 1) {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
    
    vector<int64_t> player_scores_;
    vector<int64_t> player_rank_;   // 玩家名次
    int round_;

    vector<int64_t> player_position_;   // 玩家当前位置
    vector<int64_t> player_maxspeed_;   // 玩家当前速度上限
    vector<int64_t> player_last_maxspeed_;   // 玩家上回合速度上限
    vector<int64_t> player_select_;   // 玩家选择
    int racing_num;   // 剩余比赛人数

    string T_Board = "";   // 表头
    string Board = "";   // 赛况

    // 颜色样式
    const string position_color = "FDD12E";   // 位置底色
    const string maxspeed_color = "9CCAF0";   // 上限底色
    const string speedup_color = "BAFFA8";   // 加上限颜色
    const string slowdown_color = "FFB090";   // 减上限颜色
    const string speedlimit_color = "C0DFFF";   // 速度限制颜色

    string GetName(std::string x);
    string GetStatusBoard();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(T_Board + GetStatusBoard() + Board + "</table>");
        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合，请玩家私信选择前进数。",
                MakeStageCommand(*this, "选择前进数（不能超过自己的当前上限）", &RoundStage::Submit_, ArithChecker<int64_t>(1, 100, "前进数"))) {}

  private:
    virtual void OnStageBegin() override
    {
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (Main().player_position_[pid] >= GAME_OPTION(目标)) {
                Global().SetReady(pid);
            }
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t distance)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行选择";
            return StageErrCode::FAILED;
        }
        if (Main().player_position_[pid] >= GAME_OPTION(目标)) {
            reply() << "[错误] 您已经抵达终点，无需继续行动";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了";
            return StageErrCode::FAILED;
        }
        
        if (distance > Main().player_maxspeed_[pid]) {
            reply() << "[错误] 您当前上限不足以前进此距离，可用范围为：1 - " + to_string(Main().player_maxspeed_[pid]);
            return StageErrCode::FAILED;
        }
        Main().player_select_[pid] = distance;
        reply() << "本回合前进：" + to_string(distance) + "\n当前位置为：" + to_string(Main().player_position_[pid] + distance) + " / " + to_string(GAME_OPTION(目标))
                << (Main().player_position_[pid] + distance >= GAME_OPTION(目标) ? "\n恭喜您抵达了终点！" : "");
        return StageErrCode::READY;
    }

    void HandleUnreadyPlayers_()
    {
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (!Global().IsReady(pid)) {
                Main().player_select_[pid] = Main().player_maxspeed_[pid];
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
        int total_max_speed = 0, total_max_count = 0;
        int total_min_speed = 100, total_min_count = 0;
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Main().player_select_[i] > 0) {
                if (Main().player_maxspeed_[i] > total_max_speed) {
                    total_max_speed = Main().player_maxspeed_[i];
                    total_max_count = 0;
                } else if (Main().player_maxspeed_[i] == total_max_speed) {
                    total_max_count++;
                }
                if (Main().player_maxspeed_[i] < total_min_speed) {
                    total_min_speed = Main().player_maxspeed_[i];
                } else if (Main().player_maxspeed_[i] == total_min_speed) {
                    total_min_count++;
                }
            }
        }

        int num;
        int current_max_speed = Main().player_maxspeed_[pid];
        int last_max_speed = Main().player_last_maxspeed_[pid];
        int position = Main().player_position_[pid];
        
        if (position + current_max_speed >= GAME_OPTION(目标)) {
            // 可以达到目标，自动冲线
            num = current_max_speed;
        } else if (Main().round_ == 1) {
            // R1 8:9:10 - 1:4:1
            int rd = rand() % 6;
            if (rd == 0) num = current_max_speed;
            else if (rd == 1) num = current_max_speed - 2;
            else num = current_max_speed - 1;
        } else if (Main().round_ == 2) {
            // R2
            if (position == 8) {
                // P=8 速度未降低 8-10 已降低 最大/低概率1
                if (current_max_speed == last_max_speed) {
                    num = rand() % 3 + current_max_speed - 2;
                } else {
                    num = current_max_speed;
                    if (rand() % 10 == 0) num = 1;
                }
            } else if (position == 9) {
                // P=9 9-10
                num = rand() % 2 + current_max_speed - 1;
            } else {
                // P=10 最大
                num = current_max_speed;
            }
        } else if (Main().round_ >= 3 && Main().round_ <= 6 && GAME_OPTION(目标) >= 100) {
            // R3-R6 游戏前半进程
            if (current_max_speed == total_max_speed) {
                if (total_max_count == 1) {
                    // 唯一最高 最大/极小概率1
                    num = current_max_speed;
                    if (rand() % 20 == 0) num = 1;
                } else {
                    // 最高非唯一 最大/最大-1
                    num = rand() % 2 + current_max_speed - 1;
                }
            } else if (current_max_speed == total_min_speed) {
                if (total_min_count == 1) {
                    // 唯一最低 最大
                    num = current_max_speed;
                } else {
                    // 最低非唯一 最大/小概率最大-1
                    num = current_max_speed;
                    if (rand() % 10 == 0) num = current_max_speed - 1;
                }
            } else {
                // 非最高非最低
                if (total_max_count * 3 < Global().PlayerNum()) {
                    // 若最高人数小于总人数的1/3，出场上速度最低
                    num = total_min_speed;
                } else {
                    // 否则根据与最小速度的差值决定当前速度
                    if (current_max_speed - total_min_speed <= 2) {
                        num = total_min_speed;
                    } else if (rand() % 10 < 8) {
                        num = current_max_speed;
                    } else {
                        num = total_min_speed - 1;
                    }
                }
            }
        } else {
            // 比赛后续进程
            if (current_max_speed == total_max_speed) {
                if (total_max_count == 1) {
                    // 唯一最高 最大
                    num = current_max_speed;
                } else {
                    // 最高非唯一 最大/最大-1
                    if (rand() % 3 == 0) num = current_max_speed;
                    else num = current_max_speed - 1;
                }
            } else if (current_max_speed >= GAME_OPTION(上限) * 0.8 || rand() % 10 < 6) {
                // 速度快或大概率最大速度赶距离
                num = current_max_speed;
            } else {
                // 其他情况加速
                num = total_min_speed;
            }
        }

        Main().player_select_[pid] = num;

        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        Global().Boardcast() << "第 " + to_string(Main().round_) + " 回合结束，下面公布赛况";
        calc();
        return StageErrCode::CHECKOUT;
    }

    void calc() {
        // 玩家前进距离
        int first_racing_player;
        for (int i = Global().PlayerNum() - 1; i >= 0; i--) {
            if (Main().player_select_[i] > 0) {
                Main().player_position_[i] += Main().player_select_[i];
                first_racing_player = i;
            }
        }
        vector<int> slowdown(1, first_racing_player);
        vector<int> speedup(1, first_racing_player);
		for (int i = first_racing_player + 1; i < Global().PlayerNum(); i++) {
            if (Main().player_select_[i] > Main().player_select_[slowdown[0]]) {
                slowdown.clear();
                slowdown.push_back(i);
            } else if (Main().player_select_[i] == Main().player_select_[slowdown[0]]) {
                slowdown.push_back(i);
            }
            if (Main().player_select_[i] > 0) {
                if (Main().player_select_[i] < Main().player_select_[speedup[0]]) {
                    speedup.clear();
                    speedup.push_back(i);
                } else if (Main().player_select_[i] == Main().player_select_[speedup[0]]) {
                    speedup.push_back(i);
                }
            }
		}

        // 标记上回合最大上限
        vector<int> speedlimit_players(1, first_racing_player);
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Main().player_maxspeed_[i] > Main().player_maxspeed_[speedlimit_players[0]]) {
                speedlimit_players.clear();
                speedlimit_players.push_back(i);
            } else if (Main().player_maxspeed_[i] == Main().player_maxspeed_[speedlimit_players[0]]) {
                speedlimit_players.push_back(i);
            }
        }

        // 下回合速度变化
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (speedup.size() == Main().racing_num) {
                speedup.clear();
                slowdown.clear();
            } else if (count(speedup.begin(), speedup.end(), i) && !count(speedlimit_players.begin(), speedlimit_players.end(), i)) {
                Main().player_maxspeed_[i] += speedup.size();
            } else if (count(slowdown.begin(), slowdown.end(), i)) {
                Main().player_maxspeed_[i] -= slowdown.size();
                if (Main().player_maxspeed_[i] < 1) {
                    Main().player_maxspeed_[i] = 1;
                }
            }
        }

        // board
        string b = "<tr><td>R" + to_string(Main().round_) + "</td>";
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            string color = "";
            if (count(speedup.begin(), speedup.end(), pid)) {
                if (count(speedlimit_players.begin(), speedlimit_players.end(), pid)) {
                    color = " bgcolor=\"" + Main().speedlimit_color + "\"";
                } else {
                    color = " bgcolor=\"" + Main().speedup_color + "\"";
                }
            } else if (count(slowdown.begin(), slowdown.end(), pid)) {
                color = " bgcolor=\"" + Main().slowdown_color + "\"";
            }
            if (Main().player_select_[pid] > 0) {
                b += "<td" + color + ">" + to_string(Main().player_select_[pid]) + "</td>";
            } else {
                b += "<td> </td>";
            }
        }
        b += "</tr>";
        Main().Board += b;
        
        Global().Boardcast() << Markdown(Main().T_Board + Main().GetStatusBoard() + Main().Board + "</table>");
        
        // 玩家获得胜利
        vector<vector<int>> win_players;
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (Main().player_position_[pid] >= GAME_OPTION(目标) && Main().player_rank_[pid] == Global().PlayerNum()) {
                win_players.push_back({(int)Main().player_position_[pid], pid});
            }
        }
        sort(win_players.begin(), win_players.end(), [](const vector<int>& a, const vector<int>& b) {
            return a[0] > b[0];
        });

        for (int i = 0; i < win_players.size(); i++) {
            Main().racing_num--;
            Main().player_select_[win_players[i][1]] = 0;
            if (i > 0 && Main().player_position_[win_players[i][1]] == Main().player_position_[win_players[i - 1][1]]) {
                Main().player_rank_[win_players[i][1]] = Main().player_rank_[win_players[i - 1][1]];
            } else {
                Main().player_rank_[win_players[i][1]] = Global().PlayerNum() - Main().racing_num;
            }
        }
        if (win_players.size() > 0) {
            auto sender = Global().Boardcast();
            if (Main().racing_num <= 1) {
                sender << "比赛结束！\n终局排行如下：";
            } else {
                sender << "本回合有玩家抵达了终点！\n当前比赛排行为：";
            }
            for (int i = 1; i <= Global().PlayerNum() - Main().racing_num; i++) {
                for (int pid = 0; pid < Global().PlayerNum(); pid++) {
                    if (int(Main().player_rank_[pid]) == i) {
                        sender << ("\n第" + to_string(i) + "名：") << At(PlayerID(pid));
                    }
                }
            }
            if (Main().racing_num == 1) {
                for (int pid = 0; pid < Global().PlayerNum(); pid++) {
                    if (Main().player_rank_[pid] == Global().PlayerNum()) {
                        sender << ("\n未到达：") << At(PlayerID(pid));
                    }
                }
            }
        }

        for (int i = 0; i < Global().PlayerNum(); i++) {
            Main().player_last_maxspeed_[i] = Main().player_maxspeed_[i];
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

string MainStage::GetStatusBoard() {
    string status_Board = "";
    status_Board += "<tr bgcolor=\"" + position_color + "\"><th>位置</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        status_Board += "<td>";
        if (player_position_[i] >= GAME_OPTION(目标)) {
            status_Board += "<u>" + to_string(player_position_[i]) + "</u>";
        } else {
            status_Board += to_string(player_position_[i]);
        }
        status_Board += "</td>";
    }
    status_Board += "</tr><tr bgcolor=\"" + maxspeed_color + "\"><th>上限</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        status_Board += "<td>";
        status_Board += to_string(player_maxspeed_[i]);
        if (player_maxspeed_[i] > player_last_maxspeed_[i]) {
            status_Board += "<font color=\"#1C8A3B\">(+" + to_string(player_maxspeed_[i] - player_last_maxspeed_[i]) + ")</font>";
        } else if (player_maxspeed_[i] < player_last_maxspeed_[i]) {
            status_Board += "<font color=\"#FF0000\">(-" + to_string(player_last_maxspeed_[i] - player_maxspeed_[i]) + ")</font>";
        }
        status_Board += "</td>";
    }
    status_Board += "</tr>";
    return status_Board;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));

    racing_num = Global().PlayerNum();
    for (int i = 0; i < Global().PlayerNum(); i++) {
        player_maxspeed_[i] = GAME_OPTION(上限);
        player_last_maxspeed_[i] = GAME_OPTION(上限);
    }

    T_Board += "<table><tr>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        T_Board += "<th>" + to_string(i) + " 号： " + GetName(Global().PlayerName(i)) + "　</th>";
        if (i % 4 == 3) T_Board += "</tr><tr>";
    }
    T_Board += "</tr><br>";

    T_Board += "<table style=\"text-align:center\"><tbody>";
    T_Board += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:70px;\">序号</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        T_Board += "<th style=\"width:60px;\">";
        T_Board += to_string(i) + " 号";
        T_Board += "</th>";
    }
    T_Board += "</tr>";

    string status_Board = GetStatusBoard();

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        PreBoard += to_string(i) + " 号：" + Global().PlayerName(i);
        if (i != (int)Global().PlayerNum() - 1) {
            PreBoard += "\n";
        }
    }

    Global().Boardcast() << PreBoard;
    Global().Boardcast() << Markdown(T_Board + status_Board + "</table>");
    Global().Boardcast() << "[提示] 本局游戏的目标为：" + to_string(GAME_OPTION(目标));

    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (racing_num > 1) {
        setter.Emplace<RoundStage>(*this, ++round_);
        return;
    }
    int rank = 1;
    while(rank <= Global().PlayerNum()) {
        vector<int> parallel_players;
        for (int j = 0; j < Global().PlayerNum(); j++) {
            if (player_rank_[j] == rank) {
                parallel_players.push_back(j);
            }
        }

        // 计算并列玩家分数
        int sum_rank = 0;
        for (int j = 0; j < parallel_players.size(); j++) {
            sum_rank += rank;
            rank++;
        }
        for (int j = 0; j < parallel_players.size(); j++) {
            player_scores_[parallel_players[j]] = ((Global().PlayerNum() + 1) / 2.0 * parallel_players.size() - sum_rank) * 10 / parallel_players.size();
        }
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

