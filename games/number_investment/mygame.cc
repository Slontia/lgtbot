// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <ctime>

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
    .name_ = "数字投资",
    .developer_ = "铁蛋",
    .description_ = "合理选择数字，使得所有人之和能整除你的数字来获得分数",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options{
    .is_formal_{false},
};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
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
                generic_options.bench_computers_to_player_num_ = 4;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class SubmitStage;

class MainStage : public MainGameStage<SubmitStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"))),
        round_(0),
        current_group_idx_(0),
        max_number_(GAME_OPTION(范围)),
        total_rounds_(GAME_OPTION(范围)),
        bonus_score_(GAME_OPTION(奖励分数)),
        player_scores_(Global().PlayerNum(), 0),
        player_select_(Global().PlayerNum(), 0),
        player_last_select_(Global().PlayerNum(), 0),
        player_last_scored_(Global().PlayerNum(), false)
    {
        player_used_.resize(Global().PlayerNum());
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(SubmitStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    int round_;                           // 当前回合
    int current_group_idx_;               // 当前提交组索引
    const int max_number_;                // 最大数字（由配置项决定）
    const int total_rounds_;              // 总回合数（= 最大数字）
    const int bonus_score_;               // 连续得分奖励分数
    vector<int64_t> player_scores_;       // 玩家累计总分
    vector<int64_t> player_select_;       // 本轮选择 (0=未提交)
    vector<int64_t> player_last_select_;  // 上轮选择（用于分组排序）
    vector<set<int>> player_used_;        // 每位玩家已使用的数字集合
    vector<bool> player_last_scored_;     // 上一轮是否得分（用于连续得分判定）

    // 分组提交
    vector<vector<PlayerID>> groups_;     // 本轮各组玩家列表（按上轮数字升序排列）

    // HTML 表格
    string T_Board = "";    // 表头（固定不变）
    string Board = "";      // 历史回合记录行
    int image_width;        // 图片渲染宽度

    // 颜色常量
    const string score_color = "FDD12E";   // 得分行底色
    const string sum_color = "DCDCDC";     // 回合号与总和列底色
    const string scored_color = "BAFFA8";  // 普通得分标记
    const string bonus_color = "90EE90";   // 连续得分标记（深绿）

    string GetName(string x);
    string GetScoreBoard();
    void CalcRound();
    void BuildGroups();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string score_Board = GetScoreBoard();
        reply() << Markdown(T_Board + score_Board + Board + "</table>", image_width);
        return StageErrCode::OK;
    }
};


class SubmitStage : public SubGameStage<>
{
  public:
    SubmitStage(MainStage& main_stage, const int round, const int group_idx, const vector<PlayerID>& active_players)
        : StageFsm(main_stage,
            "第 " + to_string(round) + " / " + to_string(main_stage.total_rounds_) + " 回合" +
            (main_stage.groups_.size() > 1 ? "（第 " + to_string(group_idx + 1) + " / " + to_string(main_stage.groups_.size()) + " 组）" : ""),
            MakeStageCommand(*this, "提交数字", &SubmitStage::Submit_,
                ArithChecker<int64_t>(1, main_stage.max_number_, "数字"))),
        round_(round), group_idx_(group_idx), active_players_(active_players) {}

    int round_;
    int group_idx_;
    vector<PlayerID> active_players_;    // 当前组需要提交的玩家

  private:
    // 构建分组提交信息图片（展示已提交数字、当前组、当前总和）
    string BuildGroupInfoImage()
    {
        string html = "<table style=\"text-align:center\"><tbody>";
        // 标题行
        html += "<tr><td colspan=3 align=\"left\"><font size=4><b>第 " + to_string(round_) + " 回合 — 第 "
             + to_string(group_idx_ + 1) + " / " + to_string(Main().groups_.size()) + " 组</b></font></td></tr>";
        // 表头
        html += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:50px;\">序号</th><th style=\"width:120px;\">玩家</th><th style=\"width:70px;\">数字</th></tr>";

        int64_t current_sum = 0;
        // 已提交的组（绿色底）
        for (int g = 0; g < group_idx_; g++) {
            for (auto pid : Main().groups_[g]) {
                html += "<tr bgcolor=\"" + Main().scored_color + "\">";
                html += "<td>" + to_string(pid + 1) + " 号</td>";
                html += "<td>" + Main().GetName(Global().PlayerName(pid)) + "</td>";
                html += "<td>" + to_string(Main().player_select_[pid]) + "</td>";
                html += "</tr>";
                current_sum += Main().player_select_[pid];
            }
        }
        // 当前组（黄色底，等待提交）
        for (auto pid : active_players_) {
            html += "<tr bgcolor=\"#FFF8DC\">";
            html += "<td>" + to_string(pid + 1) + " 号</td>";
            html += "<td>" + Main().GetName(Global().PlayerName(pid)) + "</td>";
            html += "<td>等待提交</td>";
            html += "</tr>";
        }
        // 尚未轮到的组（灰色底）
        for (int g = group_idx_ + 1; g < (int)Main().groups_.size(); g++) {
            for (auto pid : Main().groups_[g]) {
                html += "<tr bgcolor=\"#F0F0F0\">";
                html += "<td>" + to_string(pid + 1) + " 号</td>";
                html += "<td>" + Main().GetName(Global().PlayerName(pid)) + "</td>";
                html += "<td>-</td>";
                html += "</tr>";
            }
        }
        // 当前数字总和
        html += "<tr bgcolor=\"" + Main().sum_color + "\"><td colspan=3><b>当前数字总和：" + to_string(current_sum) + "</b></td></tr>";
        html += "</tbody></table>";
        return html;
    }

    virtual void OnStageBegin() override
    {
        // 将不在当前组的玩家设为已准备（无需提交）
        for (int pid = 0; pid < (int)Global().PlayerNum(); pid++) {
            if (find(active_players_.begin(), active_players_.end(), (PlayerID)pid) == active_players_.end()) {
                Global().SetReady(pid);
            }
        }

        if (Main().groups_.size() > 1) {
            // 多组提交：用图片展示当前提交情况（已提交数字、当前组、总和）
            Global().Boardcast() << Markdown(BuildGroupInfoImage(), 400);
            // 简短文字提示
            string msg = "第 " + to_string(group_idx_ + 1) + " / " + to_string(Main().groups_.size()) + " 组";
            msg += "（上轮出 " + to_string(Main().player_last_select_[active_players_[0]]) + "）";
            if (Main().round_ <= 1) {
                msg += "\n请私信提交一个数字（1~" + to_string(Main().max_number_) + "）";
            } else {
                msg += "\n请提交一个数字（1~" + to_string(Main().max_number_) + "），可公屏或私信提交";
            }
            Global().Boardcast() << msg;
        } else {
            // 单组（第1轮或所有人上轮出了相同数字）
            string msg = "所有玩家同时提交";
            if (Main().round_ <= 1) {
                msg += "\n请私信提交一个数字（1~" + to_string(Main().max_number_) + "）";
            } else {
                msg += "\n请提交一个数字（1~" + to_string(Main().max_number_) + "），可公屏或私信提交";
            }
            Global().Boardcast() << msg;
        }

        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        // 不调用 HookUnreadyPlayers，超时玩家仅本轮自动选择最小可用数字，下轮仍需手动提交
        // （避免 Hook 导致后续所有回合自动以电脑身份完成，破坏多组分步提交的节奏）
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        // 玩家退出时不应该有额外的选择操作，统一交给回合结束处理
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        // 电脑随机选择一个可用数字
        vector<int> available;
        for (int i = 1; i <= Main().max_number_; i++) {
            if (Main().player_used_[pid].count(i) == 0) {
                available.push_back(i);
            }
        }
        int choice = available[rand() % available.size()];
        Main().player_select_[pid] = choice;
        Main().player_used_[pid].insert(choice);
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        return StageErrCode::CHECKOUT;
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t num)
    {
        // 第1轮强制私信提交，第2轮起允许公屏或私信
        if (is_public && Main().round_ <= 1) {
            reply() << "[错误] 第 1 轮请私信裁判提交数字";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经提交过了";
            return StageErrCode::FAILED;
        }
        if (find(active_players_.begin(), active_players_.end(), pid) == active_players_.end()) {
            reply() << "[错误] 当前不是您所在的组，请等待";
            return StageErrCode::FAILED;
        }
        // 每个数字只能使用一次
        if (Main().player_used_[pid].count(num)) {
            reply() << "[错误] 您已经使用过数字 " + to_string(num) + "\n剩余可用数字：" + GetAvailableNumbers(pid);
            return StageErrCode::FAILED;
        }

        Main().player_select_[pid] = num;
        Main().player_used_[pid].insert(num);
        reply() << "提交成功，选择了数字 " + to_string(num) + "\n剩余可用数字：" + GetAvailableNumbers(pid);

        return StageErrCode::READY;
    }

    string GetAvailableNumbers(const PlayerID pid)
    {
        string result = "";
        for (int i = 1; i <= Main().max_number_; i++) {
            if (Main().player_used_[pid].count(i) == 0) {
                if (!result.empty()) result += " ";
                result += to_string(i);
            }
        }
        return result.empty() ? "无" : result;
    }

    // 超时或离开时自动选择剩余最小的数字
    void AutoSelect_(const PlayerID pid)
    {
        if (Global().IsReady(pid)) return;
        for (int i = 1; i <= Main().max_number_; i++) {
            if (Main().player_used_[pid].count(i) == 0) {
                Main().player_select_[pid] = i;
                Main().player_used_[pid].insert(i);
                return;
            }
        }
    }

    void HandleUnreadyPlayers_()
    {
        for (auto pid : active_players_) {
            if (!Global().IsReady(pid)) {
                AutoSelect_(pid);
                Global().SetReady(pid);
            }
        }
    }
};


// ========== MainStage 实现 ==========

// 从带有HTML标签的玩家名中提取纯名称
string MainStage::GetName(string x) {
    string ret = "";
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

// 构建得分汇总行
string MainStage::GetScoreBoard() {
    string s = "<tr><th bgcolor=\"" + score_color + "\">得分</th>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        s += "<td bgcolor=\"" + score_color + "\">" + to_string(player_scores_[i]) + "</td>";
    }
    s += "<td bgcolor=\"" + sum_color + "\">总和</td></tr>";
    return s;
}

// 根据上一轮选择的数字，按升序分组
void MainStage::BuildGroups() {
    groups_.clear();
    if (round_ <= 1) {
        // 第1轮：所有玩家为一组，同时提交
        vector<PlayerID> all;
        for (int i = 0; i < (int)Global().PlayerNum(); i++) {
            all.push_back(i);
        }
        groups_.push_back(all);
        return;
    }
    // 第2轮起：按上一轮数字从小到大分组，相同数字的玩家在同一组可同时提交
    map<int64_t, vector<PlayerID>> num_to_players;
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        num_to_players[player_last_select_[i]].push_back(i);
    }
    for (auto& [num, players] : num_to_players) {
        groups_.push_back(players);
    }
}

// 本轮结算
void MainStage::CalcRound() {
    // 计算本轮所有玩家提交数字之和
    int64_t total_sum = 0;
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        total_sum += player_select_[i];
    }

    // 判定每位玩家得分
    vector<int64_t> round_base(Global().PlayerNum(), 0);    // 基础得分
    vector<int64_t> round_bonus(Global().PlayerNum(), 0);   // 连续得分奖励
    vector<bool> scored(Global().PlayerNum(), false);

    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        // 总和能整除玩家数字 → 获得该数字等值的基础分
        if (total_sum % player_select_[i] == 0) {
            round_base[i] = player_select_[i];
            scored[i] = true;
            // 连续得分判定：上一轮也得分且不是第1轮，额外加奖励分数
            if (round_ > 1 && player_last_scored_[i]) {
                round_bonus[i] = bonus_score_;
            }
        }
    }

    // 更新累计总分
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        player_scores_[i] += round_base[i] + round_bonus[i];
    }

    // 构建 HTML 结果行
    string row = "<tr><td bgcolor=\"" + sum_color + "\">R" + to_string(round_) + "</td>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        string color = "";
        if (scored[i] && round_bonus[i] > 0) {
            color = " bgcolor=\"" + bonus_color + "\"";  // 连续得分用深绿
        } else if (scored[i]) {
            color = " bgcolor=\"" + scored_color + "\"";  // 普通得分用浅绿
        }
        string cell = to_string(player_select_[i]);
        if (scored[i]) {
            cell += " <font color=\"#00AC30\" size=2>(+" + to_string(round_base[i] + round_bonus[i]) + ")</font>";
        }
        row += "<td" + color + ">" + cell + "</td>";
    }
    row += "<td bgcolor=\"" + sum_color + "\">" + to_string(total_sum) + "</td></tr>";
    Board += row;

    // 广播图片
    string score_Board = GetScoreBoard();
    Global().Boardcast() << Markdown(T_Board + score_Board + Board + "</table>", image_width);

    // 保存本轮状态，为下一轮做准备
    player_last_select_ = player_select_;
    player_last_scored_ = scored;
    fill(player_select_.begin(), player_select_.end(), 0);
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));

    // 计算图片宽度（参考 beauty_vote / gold_coins 的计算方式）
    image_width = Global().PlayerNum() < 7 ? 500 : Global().PlayerNum() * 70 + 110;

    // 构建玩家名称表头
    T_Board += "<table><tr>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        T_Board += "<th>" + to_string(i + 1) + " 号： " + GetName(Global().PlayerName(i)) + "　</th>";
        if (i % 4 == 3) T_Board += "</tr><tr>";
    }
    T_Board += "</tr><br>";

    // 构建数据表头（回合 | 各玩家列 | 总和列）
    T_Board += "<table style=\"text-align:center\"><tbody>";
    T_Board += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:60px;\">玩家</th>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        T_Board += "<th style=\"width:60px;\">" + to_string(i + 1) + " 号</th>";
    }
    T_Board += "<th style=\"width:60px;\">总和</th></tr>";

    // 广播玩家信息
    string info = "本局玩家序号如下：\n";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        info += to_string(i + 1) + " 号：" + Global().PlayerName(i);
        if (i != (int)Global().PlayerNum() - 1) info += "\n";
    }
    Global().Boardcast() << info;

    string score_Board = GetScoreBoard();
    Global().Boardcast() << Markdown(T_Board + score_Board + "</table>", image_width);
    Global().Boardcast() << "[提示] 游戏共 " + to_string(total_rounds_) + " 轮，每位玩家持有数字 1~" + to_string(max_number_) + "，每个数字只能使用一次。";

    // 开始第1轮
    round_ = 1;
    BuildGroups();
    current_group_idx_ = 0;
    setter.Emplace<SubmitStage>(*this, round_, current_group_idx_, groups_[current_group_idx_]);
}

void MainStage::NextStageFsm(SubmitStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    current_group_idx_++;

    if (current_group_idx_ < (int)groups_.size()) {
        // 当前组完成，进入下一组（组信息由 OnStageBegin 的图片展示）
        setter.Emplace<SubmitStage>(*this, round_, current_group_idx_, groups_[current_group_idx_]);
        return;
    }

    // 所有组提交完毕，进行本轮结算
    CalcRound();

    // 如果还有剩余轮次，开始下一轮
    if (round_ < total_rounds_) {
        round_++;

        // 最后一轮：每人只剩一个数字，自动提交并直接结算
        if (round_ == total_rounds_) {
            Global().Boardcast() << "最后一轮，每位玩家仅剩一个数字，自动提交。";
            for (int i = 0; i < (int)Global().PlayerNum(); i++) {
                for (int n = 1; n <= max_number_; n++) {
                    if (player_used_[i].count(n) == 0) {
                        player_select_[i] = n;
                        player_used_[i].insert(n);
                        break;
                    }
                }
            }
            CalcRound();
            return;  // 不再创建新的 SubmitStage，游戏结束
        }

        BuildGroups();
        current_group_idx_ = 0;
        setter.Emplace<SubmitStage>(*this, round_, current_group_idx_, groups_[current_group_idx_]);
    }
    // round_ == total_rounds_: 最后一轮结束，游戏自动终止
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
