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
    .name_ = "以假乱真", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "通过出牌与质疑进行心理博弈，尽快弃光假卡的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; }
// TODO: 正式版本上线前需将倍率调整为实际值（如 1）。当前为测试游戏，倍率为 0。
uint32_t Multiple(const CustomOptions& options) { return 0; }
const MutableGenericOptions k_default_generic_options{
    .is_formal_ = false,
};
const std::vector<RuleCommand> k_rule_commands = {};

// 手中假卡数量达到此值直接出局
static const int k_max_fake = 7;
// 超出此回合数后电脑不再进行质疑行为，防止游戏无法结束
static const int k_ai_no_challenge_round = 15;

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
        player_scores_(Global().PlayerNum(), 0),
        player_fake_(Global().PlayerNum(), 0),
        player_last_fake_(Global().PlayerNum(), 0),
        player_out_(Global().PlayerNum(), 0),
        player_action_(Global().PlayerNum(), ' '),
        player_target_(Global().PlayerNum(), 0) {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    int round_;
    int alive_;          // 存活玩家数
    bool game_over_;     // 是否结束
    vector<int64_t> player_scores_;

    vector<int64_t> player_fake_;        // 当前假卡数量
    vector<int64_t> player_last_fake_;   // 上回合假卡数量（用于展示增减）
    vector<int> player_out_;             // 0 在场 1 出局
    vector<char> player_action_;         // 本回合行动 T真 F假 Q质疑 X缺席(超时/退出) ' '未行动/已出局
    vector<int64_t> player_target_;      // 质疑目标（0 起始下标）

    string T_Board = "";        // 表头
    string Board = "";          // 赛况
    string game_details = "";   // 回合详情（随棋盘图片一同展示）

    // 颜色样式
    const string fake_color = "FDD12E";            // 假卡数量底色
    const string card_good_color = "BAFFA8";       // 出牌有利结果颜色（弃假卡 / 真卡成功）
    const string card_bad_color = "FFD6D6";        // 出牌不利结果颜色（真卡失败）
    const string challenge_color = "A0FFFF";       // 质疑命中颜色
    const string challenge_fail_color = "FFA07A";  // 质疑失败颜色
    const string out_color = "AAAAAA";             // 出局颜色

    const int image_width = Global().PlayerNum() < 8 ? Global().PlayerNum() * 80 + 90 : (Global().PlayerNum() < 16 ? Global().PlayerNum() * 70 + 70 : Global().PlayerNum() * 40 + 50);

    string GetName(std::string x);
    string GetStatusBoard();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string status_Board = GetStatusBoard();
        reply() << Markdown(T_Board + status_Board + Board + "</table>" + game_details, image_width);
        return StageErrCode::OK;
    }
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand(*this, "出「永恒真卡」", &RoundStage::PlayTrue_, VoidChecker("真", "真卡")),
                MakeStageCommand(*this, "出「失魂假卡」", &RoundStage::PlayFake_, VoidChecker("假", "假卡")),
                MakeStageCommand(*this, "质疑指定玩家", &RoundStage::Challenge_, ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "目标"))) {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Name() << "，请所有玩家私信选择行动。";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (!Global().IsReady(i) && Main().player_out_[i] == 0) {
                Main().player_action_[i] = 'X';
                Main().player_target_[i] = i;
            }
        }
        Global().Boardcast() << "有玩家超时仍未行动，已被淘汰";
        calc();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_action_[pid] = 'X';
        Main().player_target_[pid] = pid;
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        vector<int> targets;
        for (int j = 0; j < Global().PlayerNum(); j++) {
            if (Main().player_out_[j] == 0 && j != (int)pid) {
                targets.push_back(j);
            }
        }
        // 纯随机选择行动；超出限定回合数后不再质疑，保证游戏能够结束
        int choices = (Main().round_ > k_ai_no_challenge_round || targets.empty()) ? 2 : 3;
        int r = rand() % choices;
        if (r == 2) {
            Main().player_action_[pid] = 'Q';
            Main().player_target_[pid] = targets[rand() % targets.size()];
        } else if (r == 1) {
            Main().player_action_[pid] = 'F';
            Main().player_target_[pid] = pid;
        } else {
            Main().player_action_[pid] = 'T';
            Main().player_target_[pid] = pid;
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "所有玩家行动完成，下面公布赛况。";
        calc();
        return StageErrCode::CHECKOUT;
    }

    // 本回合每名玩家的出牌/质疑结果，用于上色与详情播报
    enum Result { NONE, DISCARD, GAIN, DUMP, CATCH, WRONG, VOID, WIN };

    void calc()
    {
        const int N = Global().PlayerNum();
        vector<int> was_active(N, 0);
        for (int i = 0; i < N; i++) {
            was_active[i] = (Main().player_out_[i] == 0) ? 1 : 0;
        }

        vector<int> result(N, NONE);
        vector<bool> out_now(N, false);            // 本回合出局
        vector<int> out_reason(N, 0);              // 0 无 1 被质疑命中 2 假卡达上限 3 超时/退出
        vector<bool> challenged_fake(N, false);    // 出假卡被质疑（将出局）
        vector<int> true_chal_count(N, 0);         // 质疑该出真卡玩家的人数

        const vector<char>& action = Main().player_action_;
        const vector<int64_t>& target = Main().player_target_;

        // 判定质疑者
        for (int i = 0; i < N; i++) {
            if (!was_active[i] || action[i] != 'Q') continue;
            int t = (int)target[i];
            if (!was_active[t]) {
                result[i] = VOID;
            } else if (action[t] == 'F') {
                challenged_fake[t] = true;
                result[i] = CATCH;
            } else if (action[t] == 'T') {
                true_chal_count[t]++;
                result[i] = WRONG;
            } else {
                // 目标本回合质疑或缺席，没有出牌
                result[i] = VOID;
            }
        }

        // 结算出牌玩家
        for (int i = 0; i < N; i++) {
            if (!was_active[i]) continue;
            if (action[i] == 'F') {
                if (challenged_fake[i]) {
                    out_now[i] = true;
                    out_reason[i] = 1;
                } else {
                    Main().player_fake_[i] -= 1;
                    result[i] = DISCARD;
                }
            } else if (action[i] == 'T') {
                if (true_chal_count[i] > 0) {
                    Main().player_fake_[i] -= true_chal_count[i];
                    if (Main().player_fake_[i] < 0) Main().player_fake_[i] = 0;
                    result[i] = DUMP;
                } else {
                    Main().player_fake_[i] += 1;
                    result[i] = GAIN;
                }
            } else if (action[i] == 'X') {
                out_now[i] = true;
                out_reason[i] = 3;
            }
        }

        // 质疑到真卡的玩家获得 1 张假卡
        for (int i = 0; i < N; i++) {
            if (was_active[i] && action[i] == 'Q' && result[i] == WRONG) {
                Main().player_fake_[i] += 1;
            }
        }

        // 假卡数量达到上限出局
        for (int i = 0; i < N; i++) {
            if (!was_active[i] || out_now[i]) continue;
            if (Main().player_fake_[i] >= k_max_fake) {
                out_now[i] = true;
                out_reason[i] = 2;
            }
        }

        // 弃光假卡获胜
        bool winner_exists = false;
        for (int i = 0; i < N; i++) {
            if (!was_active[i] || out_now[i]) continue;
            if (Main().player_fake_[i] == 0) {
                result[i] = WIN;
                winner_exists = true;
            }
        }

        // 应用出局
        for (int i = 0; i < N; i++) {
            if (was_active[i] && out_now[i] && Main().player_out_[i] == 0) {
                Main().player_out_[i] = 1;
                Global().Eliminate(i);
                Main().alive_--;
            }
        }

        Main().game_over_ = winner_exists || (Main().alive_ <= 1);

        // 绘制赛况行
        string status_Board = Main().GetStatusBoard();

        string b = "<tr><td>R" + to_string(Main().round_) + "</td>";
        for (int i = 0; i < N; i++) {
            string color = "";
            if (out_now[i]) {
                color = " bgcolor=\"" + Main().out_color + "\"";   // 出局：灰色
            } else if (result[i] == DISCARD || result[i] == DUMP || result[i] == WIN) {
                color = " bgcolor=\"" + Main().card_good_color + "\"";
            } else if (result[i] == GAIN) {
                color = " bgcolor=\"" + Main().card_bad_color + "\"";
            } else if (result[i] == CATCH) {
                color = " bgcolor=\"" + Main().challenge_color + "\"";
            } else if (result[i] == WRONG) {
                color = " bgcolor=\"" + Main().challenge_fail_color + "\"";   // 质疑失败
            }
            // VOID（质疑无效）：无底色
            b += "<td" + color + ">";
            if (!was_active[i]) {
                b += " ";
            } else if (action[i] == 'T') {
                b += "真";
            } else if (action[i] == 'F') {
                b += "假";
            } else if (action[i] == 'Q') {
                b += "质疑" + to_string(target[i] + 1);
            } else if (action[i] == 'X') {
                b += "出局";
            }
            b += "</td>";
        }
        b += "</tr>";
        Main().Board += b;

        // 回合详情（随棋盘图片一同展示）
        string details = "<font size=2>";
        bool has_event = false;
        for (int i = 0; i < N; i++) {
            if (!was_active[i]) continue;
            if (action[i] == 'Q' && result[i] == CATCH) {
                details += "- 玩家 <font color=blue>" + to_string(i + 1) + "号</font> 质疑 <font color=red>" + to_string(target[i] + 1) + "号</font> 命中假卡，对方出局！<br/>";
                has_event = true;
            }
            if (out_reason[i] == 2) {
                details += "- 玩家 <font color=red>" + to_string(i + 1) + "号</font> 假卡达到 " + to_string(k_max_fake) + " 张，出局！<br/>";
                has_event = true;
            }
            if (result[i] == WIN) {
                details += "- 玩家 <font color=green>" + to_string(i + 1) + "号</font> 弃光全部假卡，达成胜利！<br/>";
                has_event = true;
            }
        }
        details += "</font>";
        Main().game_details = has_event ? details : "";

        Global().Boardcast() << Markdown(Main().T_Board + status_Board + Main().Board + "</table>" + Main().game_details, Main().image_width);

        for (int i = 0; i < N; i++) {
            Main().player_last_fake_[i] = Main().player_fake_[i];
        }
    }

  private:
    AtomReqErrCode PlayTrue_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'T';
        Main().player_target_[pid] = pid;
        reply() << "成功选择「永恒真卡」";
        return StageErrCode::READY;
    }

    AtomReqErrCode PlayFake_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'F';
        Main().player_target_[pid] = pid;
        reply() << "成功选择「失魂假卡」";
        return StageErrCode::READY;
    }

    AtomReqErrCode Challenge_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经行动过了。";
            return StageErrCode::FAILED;
        }
        if (target < 1 || target > Global().PlayerNum() || target - 1 == pid) {
            reply() << "[错误] 目标无效。";
            return StageErrCode::FAILED;
        }
        if (Main().player_out_[target - 1] != 0) {
            reply() << "[错误] 目标已经出局。";
            return StageErrCode::FAILED;
        }
        Main().player_action_[pid] = 'Q';
        Main().player_target_[pid] = target - 1;
        reply() << "质疑 " << target << " 号玩家";
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
    status_Board += "<tr bgcolor=\"" + fake_color + "\"><th>假卡</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        status_Board += "<td>";
        if (player_out_[i] == 1) {
            status_Board += "出局";
        } else {
            status_Board += to_string(player_fake_[i]);
            if (player_fake_[i] > player_last_fake_[i]) {
                status_Board += "<font color=\"#FF0000\">(+" + to_string(player_fake_[i] - player_last_fake_[i]) + ")</font>";
            } else if (player_fake_[i] < player_last_fake_[i]) {
                status_Board += "<font color=\"#1C8A3B\">(-" + to_string(player_last_fake_[i] - player_fake_[i]) + ")</font>";
            }
        }
        status_Board += "</td>";
    }
    status_Board += "</tr>";
    return status_Board;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));
    alive_ = Global().PlayerNum();

    for (int i = 0; i < Global().PlayerNum(); i++) {
        player_fake_[i] = player_last_fake_[i] = GAME_OPTION(假卡);
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
    Global().Boardcast() << Markdown(T_Board + status_Board + "</table>", image_width);

    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (!game_over_) {
        setter.Emplace<RoundStage>(*this, ++round_);
        return;
    }

    for (int i = 0; i < Global().PlayerNum(); i++) {
        if (player_out_[i] == 1) {
            player_scores_[i] = -k_max_fake;
        } else {
            player_scores_[i] = -player_fake_[i];
        }
    }

    vector<int> winners;
    for (int i = 0; i < Global().PlayerNum(); i++) {
        if (player_out_[i] == 0 && player_fake_[i] == 0) {
            winners.push_back(i);
        }
    }
    if (!winners.empty()) {
        Global().Boardcast() << "有玩家弃光全部假卡，游戏结束！";
    } else if (alive_ == 1) {
        for (int i = 0; i < Global().PlayerNum(); i++) {
            if (player_out_[i] == 0) {
                Global().Boardcast() << "仅剩 " << At(PlayerID(i)) << " 存活，获得胜利！";
                break;
            }
        }
    } else {
        Global().Boardcast() << "所有玩家均已出局，游戏结束！";
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
