// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <vector>
#include <algorithm>

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
    .name_ = "有追球", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "争夺场上的球、避免血量耗尽，尽可能存活到最后的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options{
    .is_formal_ = false,
};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 4) {
        reply() << "该游戏至少 4 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
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
        game_over_(false),
        balls_in_play_(0),
        player_scores_(Global().PlayerNum(), 0),
        player_hp_(Global().PlayerNum(), 0),
        player_last_hp_(Global().PlayerNum(), 0),
        player_out_(Global().PlayerNum(), 0),
        player_action_(Global().PlayerNum(), ' '),
        player_target_(Global().PlayerNum(), 0),
        ball_holder_(Global().PlayerNum() - 1, kFreeBall) {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    static constexpr int kFreeBall = -1;     // 自由球
    static constexpr int kRemovedBall = -2;  // 已移除的球

    int round_;
    int alive_;              // 存活玩家数
    bool game_over_;         // 是否结束
    int balls_in_play_;      // 在场球数（在场球编号恒为 1..balls_in_play_ 连续）
    vector<int64_t> player_scores_;

    vector<int64_t> player_hp_;         // 当前血量
    vector<int64_t> player_last_hp_;    // 上回合血量（用于展示增减）
    vector<int> player_out_;            // 0 在场 1 出局
    vector<char> player_action_;        // 本回合行动 H持球 P护球 D定向护球 G抢球 N不抢 X退出 ' '未行动
    vector<int64_t> player_target_;     // 定向护球的目标玩家（0 起始下标）/ 抢球的球编号（1 起始）
    vector<int> ball_holder_;           // 每个球的持有者（下标=球编号-1），kFreeBall 自由，kRemovedBall 已移除

    string T_Board = "";        // 表头
    string Board = "";          // 赛况
    string game_details = "";   // 回合详情（随棋盘图片一同展示）

    // 颜色样式
    const string hp_color = "9CCAF0";             // 血量行底色
    const string ball_row_color = "FDD12E";       // 持球行底色
    const string gain_color = "BAFFA8";           // 抢到球颜色
    const string keep_color = "A0FFFF";           // 保住球权颜色
    const string directed_kill_color = "FFD4F3";  // 定向护球命中颜色
    const string lose_color = "FFD6D6";           // 失去球权（丢球）颜色
    const string out_color = "AAAAAA";            // 出局颜色

    const int image_width = Global().PlayerNum() < 8 ? Global().PlayerNum() * 80 + 90 : (Global().PlayerNum() < 16 ? Global().PlayerNum() * 70 + 70 : Global().PlayerNum() * 40 + 50);

    string GetName(std::string x);
    string GetStatusBoard();
    string GetFreeBallLine();

    // 返回玩家所持球的编号（1 起始），无球返回 0
    int BallOf(const int pid) const
    {
        for (int b = 1; b <= balls_in_play_; b++) {
            if (ball_holder_[b - 1] == pid) {
                return b;
            }
        }
        return 0;
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string status_Board = GetStatusBoard();
        reply() << Markdown(T_Board + status_Board + Board + "</table>" + GetFreeBallLine() + game_details, image_width);
        return StageErrCode::OK;
    }
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand(*this, "持球（有球时可用）", &RoundStage::Hold_, VoidChecker("持球", "持")),
                MakeStageCommand(*this, "护球（有球时可用）", &RoundStage::Protect_, VoidChecker("护球", "护")),
                MakeStageCommand(*this, "定向护球，指定一名其他玩家（有球时可用）", &RoundStage::Directed_,
                        VoidChecker("定向", "定向护球"), ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "目标玩家")),
                MakeStageCommand(*this, "抢夺指定编号的球", &RoundStage::Grab_,
                        VoidChecker("抢", "抢球"), ArithChecker<int64_t>(1, main_stage.Global().PlayerNum() - 1, "球编号")),
                MakeStageCommand(*this, "不抢球（无球时可用）", &RoundStage::Pass_, VoidChecker("不抢", "不"))) {}

    virtual void OnStageBegin() override
    {
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (Main().player_out_[i] == 0) {
                Main().player_action_[i] = ' ';
                Main().player_target_[i] = 0;
            }
        }
        Global().Boardcast() << Name() << "，请所有玩家私信选择行动。\n"
                "有球可选：持 / 护 / 定向 <目标玩家> / 抢 <球编号>\n"
                "无球可选：抢 <球编号> / 不抢";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (!Global().IsReady(i) && Main().player_out_[i] == 0 && Main().player_action_[i] != 'X') {
                Main().player_action_[i] = 'T';
                Main().player_target_[i] = 0;
                Main().player_hp_[i] = 0;
                const int b = Main().BallOf(i);
                if (b != 0) {
                    Main().ball_holder_[b - 1] = MainStage::kFreeBall;
                }
            }
        }
        Global().Boardcast() << "有玩家超时未行动，已被淘汰。";
        calc();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_action_[pid] = 'X';
        Main().player_target_[pid] = 0;
        Main().player_hp_[pid] = 0;
        const int b = Main().BallOf(pid);
        if (b != 0) {
            Main().ball_holder_[b - 1] = MainStage::kFreeBall;
        }
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || Main().player_out_[pid] != 0) {
            return StageErrCode::OK;
        }
        const int my_ball = Main().BallOf(pid);
        if (my_ball != 0) {
            vector<int> targets;
            for (int j = 0; j < Global().PlayerNum(); j++) {
                if (Main().player_out_[j] == 0 && Main().player_hp_[j] > 0 && j != (int)pid) {
                    targets.push_back(j);
                }
            }
            vector<int> other_balls;
            for (int b = 1; b <= Main().balls_in_play_; b++) {
                if (b != my_ball) {
                    other_balls.push_back(b);
                }
            }
            // 随机选择：持球 / 护球 / 定向护球 / 抢其他球
            vector<int> cand = {0, 1};
            if (!targets.empty()) cand.push_back(2);
            if (!other_balls.empty()) cand.push_back(3);
            const int c = cand[rand() % cand.size()];
            if (c == 0) {
                Main().player_action_[pid] = 'H';
                Main().player_target_[pid] = 0;
            } else if (c == 1) {
                Main().player_action_[pid] = 'P';
                Main().player_target_[pid] = 0;
            } else if (c == 2) {
                Main().player_action_[pid] = 'D';
                Main().player_target_[pid] = targets[rand() % targets.size()];
            } else {
                Main().player_action_[pid] = 'G';
                Main().player_target_[pid] = other_balls[rand() % other_balls.size()];
            }
        } else {
            // 无球时：大概率抢一个在场球，否则不抢
            if (Main().balls_in_play_ > 0 && rand() % 4 != 0) {
                Main().player_action_[pid] = 'G';
                Main().player_target_[pid] = 1 + rand() % Main().balls_in_play_;
            } else {
                Main().player_action_[pid] = 'N';
                Main().player_target_[pid] = 0;
            }
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "所有玩家行动完成，下面公布赛况。";
        calc();
        return StageErrCode::CHECKOUT;
    }

    // 本回合每名玩家的行动结果，用于上色与详情播报
    enum Result { NONE, GAIN, KEEP, TRAP, LOSE, MISS };

    void calc()
    {
        const int N = Global().PlayerNum();
        vector<int> was_active(N, 0);
        for (int i = 0; i < N; i++) {
            was_active[i] = (Main().player_out_[i] == 0) ? 1 : 0;
        }
        // 记录回合开始时（弃球释放之前）各玩家是否持球，用于统一标记丢球
        vector<bool> had_ball(N, false);
        for (int i = 0; i < N; i++) {
            had_ball[i] = was_active[i] && (Main().BallOf(i) != 0);
        }

        const vector<char>& action = Main().player_action_;
        const vector<int64_t>& target = Main().player_target_;

        // 选择抢其他球的持球者先释放原球（无论其抢夺是否成功）
        for (int i = 0; i < N; i++) {
            if (was_active[i] && action[i] == 'G') {
                const int cur = Main().BallOf(i);
                if (cur != 0) {
                    Main().ball_holder_[cur - 1] = MainStage::kFreeBall;
                }
            }
        }

        // 快照各球的持有情况与抢夺者，此后逐球独立结算
        const int M = Main().balls_in_play_;
        vector<int> holder0(M + 1, MainStage::kFreeBall);
        for (int b = 1; b <= M; b++) {
            holder0[b] = Main().ball_holder_[b - 1];
        }
        vector<vector<int>> grabbers(M + 1);
        for (int i = 0; i < N; i++) {
            if (was_active[i] && action[i] == 'G' && target[i] >= 1 && target[i] <= M) {
                grabbers[target[i]].push_back(i);
            }
        }

        vector<int> result(N, NONE);
        vector<bool> directed_kill(N, false);

        for (int b = 1; b <= M; b++) {
            const int holder = holder0[b];
            const auto& g = grabbers[b];
            if (holder != MainStage::kFreeBall) {
                const char act = action[holder];
                if (act == 'D' && std::find(g.begin(), g.end(), (int)target[holder]) != g.end()) {
                    // 定向护球命中：目标立即淘汰，球权保留，其余抢球者均不获得
                    const int t = (int)target[holder];
                    directed_kill[t] = true;
                    result[holder] = TRAP;
                    for (const int i : g) {
                        if (i != t) {
                            result[i] = MISS;
                        }
                    }
                } else if (act == 'P') {
                    // 护球：有人抢则球权保留，没人抢则变为自由球
                    if (g.empty()) {
                        Main().ball_holder_[b - 1] = MainStage::kFreeBall;
                    } else {
                        result[holder] = KEEP;
                        for (const int i : g) {
                            result[i] = MISS;
                        }
                    }
                } else {
                    // 持球（以及定向护球未命中目标的情况）：恰一人抢则球权转移，多人抢则变为自由球
                    if (g.empty()) {
                        if (act == 'H') {
                            result[holder] = KEEP;
                        } else {
                            Main().ball_holder_[b - 1] = MainStage::kFreeBall;
                        }
                    } else if (g.size() == 1) {
                        Main().ball_holder_[b - 1] = g[0];
                        result[g[0]] = GAIN;
                    } else {
                        Main().ball_holder_[b - 1] = MainStage::kFreeBall;
                        for (const int i : g) {
                            result[i] = MISS;
                        }
                    }
                }
            } else {
                // 自由球（含本回合刚被释放的球）：恰一人抢则获得，否则仍为自由球
                if (g.size() == 1) {
                    Main().ball_holder_[b - 1] = g[0];
                    result[g[0]] = GAIN;
                } else {
                    for (const int i : g) {
                        result[i] = MISS;
                    }
                }
            }
        }

        // 丢球统一标红：回合开始时持球、结算后无球的存活玩家一律视为丢球
        //（涵盖持球/护球被抢走、被多人抢导致球变自由球、弃球去抢却抢夺失败等情形）
        for (int i = 0; i < N; i++) {
            if (was_active[i] && had_ball[i] && Main().BallOf(i) == 0) {
                result[i] = LOSE;
            }
        }

        // 淘汰收集：定向护球命中 / 退出 / 超时 / 血量耗尽
        vector<bool> out_now(N, false);
        for (int i = 0; i < N; i++) {
            if (was_active[i] && (directed_kill[i] || action[i] == 'X' || action[i] == 'T')) {
                out_now[i] = true;
            }
        }
        // 回合结束扣血：所有没球的玩家 -1 血
        for (int i = 0; i < N; i++) {
            if (!was_active[i] || out_now[i]) continue;
            if (Main().BallOf(i) == 0) {
                Main().player_hp_[i] -= 1;
                if (Main().player_hp_[i] <= 0) {
                    out_now[i] = true;
                }
            }
        }
        // 应用淘汰
        int out_count = 0;
        for (int i = 0; i < N; i++) {
            if (was_active[i] && out_now[i] && Main().player_out_[i] == 0) {
                Main().player_out_[i] = 1;
                if (Main().player_hp_[i] > 0) {
                    Main().player_hp_[i] = 0;
                }
                Global().Eliminate(i);
                Main().alive_--;
                out_count++;
            }
        }
        // 每淘汰一人，移除一个当前编号最大的球
        vector<pair<int, int>> removed;   // (球编号, 原持有者)
        const int remove_cnt = std::min(out_count, Main().balls_in_play_);
        for (int r = 0; r < remove_cnt; r++) {
            const int b = Main().balls_in_play_;
            removed.push_back({b, Main().ball_holder_[b - 1]});
            Main().ball_holder_[b - 1] = MainStage::kRemovedBall;
            Main().balls_in_play_--;
        }
        // 存活玩家累计生存分
        for (int i = 0; i < N; i++) {
            if (Main().player_out_[i] == 0) {
                Main().player_scores_[i]++;
            }
        }
        // 结束条件：仅剩 1 人及以下，或剩 2 人且场上的球均为自由球
        bool ball_all_free = true;
        for (int b = 1; b <= Main().balls_in_play_; b++) {
            if (Main().ball_holder_[b - 1] != MainStage::kFreeBall) {
                ball_all_free = false;
                break;
            }
        }
        if (Main().alive_ <= 1 || (Main().alive_ == 2 && ball_all_free)) {
            Main().game_over_ = true;
        }

        // 绘制赛况行
        string status_Board = Main().GetStatusBoard();

        string b_row = "<tr><td>R" + to_string(Main().round_) + "</td>";
        for (int i = 0; i < N; i++) {
            string color = "";
            if (!was_active[i]) {
                // 早已出局，无底色
            } else if (out_now[i]) {
                color = " bgcolor=\"" + Main().out_color + "\"";
            } else if (result[i] == TRAP) {
                color = " bgcolor=\"" + Main().directed_kill_color + "\"";
            } else if (result[i] == GAIN) {
                color = " bgcolor=\"" + Main().gain_color + "\"";
            } else if (result[i] == KEEP) {
                color = " bgcolor=\"" + Main().keep_color + "\"";
            } else if (result[i] == LOSE) {
                color = " bgcolor=\"" + Main().lose_color + "\"";
            }
            // MISS（抢夺落空）：无底色
            b_row += "<td" + color + ">";
            if (!was_active[i]) {
                b_row += " ";
            } else if (action[i] == 'H') {
                b_row += "持球";
            } else if (action[i] == 'P') {
                b_row += "护球";
            } else if (action[i] == 'D') {
                b_row += "定向→" + to_string(target[i] + 1);
            } else if (action[i] == 'G') {
                b_row += "抢" + to_string(target[i]) + "号";
            } else if (action[i] == 'N') {
                b_row += "不抢";
            } else if (action[i] == 'X') {
                b_row += "退出";
            } else if (action[i] == 'T') {
                b_row += "超时";
            }
            b_row += "</td>";
        }
        b_row += "</tr>";
        Main().Board += b_row;

        // 回合详情（随棋盘图片一同展示）
        string details = "<font size=2>";
        bool has_event = false;
        for (int i = 0; i < N; i++) {
            if (was_active[i] && directed_kill[i]) {
                details += "- 玩家 <font color=red>" + to_string(i + 1) + "号</font> 抢夺 " + to_string(target[i]) +
                        " 号球时被定向护球命中，立即淘汰！<br/>";
                has_event = true;
            }
        }
        for (const auto& [ball_id, prev_holder] : removed) {
            details += "- <font color=blue>" + to_string(ball_id) + " 号球</font> 被移出游戏";
            if (prev_holder >= 0) {
                details += "（原持有者 " + to_string(prev_holder + 1) + " 号直接失去该球）";
            }
            details += "<br/>";
            has_event = true;
        }
        if (Main().game_over_) {
            details += "- 场上的球为自由球，游戏结束！<br/>";
            has_event = true;
        }
        details += "</font>";
        Main().game_details = has_event ? details : "";

        Global().Boardcast() << Markdown(Main().T_Board + status_Board + Main().Board + "</table>" + Main().GetFreeBallLine() + Main().game_details, Main().image_width);

        for (int i = 0; i < N; i++) {
            Main().player_last_hp_[i] = Main().player_hp_[i];
        }
    }

  private:
    AtomReqErrCode Hold_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        if (Main().BallOf(pid) == 0) {
            reply() << "[错误] 您当前没有持球，无法选择该行动。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'H';
        Main().player_target_[pid] = 0;
        reply() << "成功选择「持球」";
        return StageErrCode::READY;
    }

    AtomReqErrCode Protect_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        if (Main().BallOf(pid) == 0) {
            reply() << "[错误] 您当前没有持球，无法选择该行动。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'P';
        Main().player_target_[pid] = 0;
        reply() << "成功选择「护球」";
        return StageErrCode::READY;
    }

    AtomReqErrCode Directed_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        if (Main().BallOf(pid) == 0) {
            reply() << "[错误] 您当前没有持球，无法选择该行动。";
            return StageErrCode::FAILED;
        }
        if (target - 1 == pid) {
            reply() << "[错误] 不能指定自己为目标。";
            return StageErrCode::FAILED;
        }
        if (Main().player_out_[target - 1] != 0 || Main().player_hp_[target - 1] <= 0) {
            reply() << "[错误] 目标已经出局。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'D';
        Main().player_target_[pid] = target - 1;
        reply() << "成功选择「定向护球」，目标为 " << target << " 号玩家";
        return StageErrCode::READY;
    }

    AtomReqErrCode Grab_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t ball)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        if (ball > Main().balls_in_play_) {
            reply() << "[错误] 该球已被移除。";
            return StageErrCode::FAILED;
        }
        if (ball == Main().BallOf(pid)) {
            reply() << "[错误] 不能抢夺自己持有的球。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'G';
        Main().player_target_[pid] = ball;
        reply() << "成功选择抢夺 " << ball << " 号球";
        return StageErrCode::READY;
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        if (Main().BallOf(pid) != 0) {
            reply() << "[错误] 您当前持有球，请选择持球、护球、定向护球或抢其他球。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'N';
        Main().player_target_[pid] = 0;
        reply() << "成功选择「不抢」";
        return StageErrCode::READY;
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
    // 血量行
    status_Board += "<tr bgcolor=\"" + hp_color + "\"><th>血量</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        status_Board += "<td>";
        if (player_out_[i] == 1) {
            status_Board += "出局";
        } else {
            status_Board += to_string(player_hp_[i]);
            if (player_hp_[i] < player_last_hp_[i]) {
                status_Board += "<font color=\"#FF0000\">(-" + to_string(player_last_hp_[i] - player_hp_[i]) + ")</font>";
            }
        }
        status_Board += "</td>";
    }
    status_Board += "</tr>";
    // 持球行
    status_Board += "<tr bgcolor=\"" + ball_row_color + "\"><th>持球</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        status_Board += "<td>";
        if (player_out_[i] == 1) {
            status_Board += " ";
        } else {
            const int b = BallOf(i);
            status_Board += (b == 0) ? "－" : "⚽" + (to_string(b) + "号");
        }
        status_Board += "</td>";
    }
    status_Board += "</tr>";
    return status_Board;
}

string MainStage::GetFreeBallLine() {
    string line = "<b>自由球：</b>";
    bool any = false;
    for (int b = 1; b <= balls_in_play_; b++) {
        if (ball_holder_[b - 1] == kFreeBall) {
            line += "⚽" + to_string(b) + "号　";
            any = true;
        }
    }
    if (!any) {
        line += "无";
    }
    return "<div style=\"margin:0.5em 0\">" + line + "</div>";
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));
    alive_ = Global().PlayerNum();
    balls_in_play_ = Global().PlayerNum() - 1;
    for (int i = 0; i < Global().PlayerNum(); i++) {
        player_hp_[i] = player_last_hp_[i] = GAME_OPTION(血量);
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
    T_Board += "</tr>";

    string status_Board = GetStatusBoard();

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + Global().PlayerName(i);
        if (i != (int)Global().PlayerNum() - 1) {
            PreBoard += "\n";
        }
    }

    Global().Boardcast() << PreBoard;
    Global().Boardcast() << "场上共有 " << balls_in_play_ << " 个球（编号 1 至 " << balls_in_play_ << "），初始均为自由球。";
    Global().Boardcast() << Markdown(T_Board + status_Board + "</table>" + GetFreeBallLine(), image_width);

    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (!game_over_ && alive_ > 0) {
        setter.Emplace<RoundStage>(*this, ++round_);
        return;
    }

    if (alive_ <= 0) {
        Global().Boardcast() << "所有玩家均已淘汰，游戏结束！";
        return;
    }

    int64_t max_hp = 0;
    for (int i = 0; i < Global().PlayerNum(); i++) {
        if (player_out_[i] == 0) {
            max_hp = std::max(max_hp, player_hp_[i]);
        }
    }
    auto sender = Global().Boardcast();
    sender << "游戏结束，恭喜获胜者：";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        if (player_out_[i] != 0) {
            continue;
        }
        if (player_hp_[i] == max_hp) {
            player_scores_[i] += 5;
            sender << At(PlayerID(i)) << " ";
        } else {
            player_scores_[i] += 3;
        }
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
