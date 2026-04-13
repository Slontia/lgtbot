// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <vector>
#include <algorithm>
#include <string>

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
    .name_ = "明争暗斗",
    .developer_ = "铁蛋",
    .description_ = "选择数字，争取最快得到目标分数的博弈游戏",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
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

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility)),
        round_(0),
        alive_(Global().PlayerNum()),
        player_scores_(Global().PlayerNum(), 0),
        player_alive_(Global().PlayerNum(), true),
        player_number_(Global().PlayerNum(), 0),
        player_target_(Global().PlayerNum(), -1),
        player_guess_(Global().PlayerNum(), 0),
        player_last_number_(Global().PlayerNum(), 0) {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    int round_;
    int alive_;
    vector<int64_t> player_scores_;
    vector<bool> player_alive_;
    vector<int> player_number_;       // 本回合选择的数字 (2-8, 0=放弃/超时)
    vector<int> player_target_;       // 目标玩家ID (-1=无目标)
    vector<int> player_guess_;        // 猜测数字 (仅刺杀用, 0=无)
    vector<int> player_last_number_;  // 上回合选择的数字

    string T_Board = "";
    string Board = "";
    string game_details = "";

    // 行动成功底色
    const string assassinate_success_color = "FFCACA";   // 刺杀成功 - 浅珊瑚红
    const string counter_success_color = "D8CAFF";       // 反制成功 - 浅薰衣草紫
    const string shield_success_color = "FFF0AA";        // 圣盾成功 - 浅金色
    const string destroy_success_color = "FFDCE8";       // 毁灭生效 - 浅玫红
    const string compete_success_color = "CAE0FF";       // 拼点成功 - 浅天蓝
    const string discard_triggered_color = "D0D0D0";     // 弃牌被触发 - 中灰
    const string giveup_color = "E5E5E5";               // 放弃/超时 - 浅灰
    // 通用底色
    const string score_color = "FDD12E";                 // 得分行底色

    const int image_width = Global().PlayerNum() < 8 ? Global().PlayerNum() * 90 + 90 :
                            (Global().PlayerNum() < 16 ? Global().PlayerNum() * 75 + 70 :
                            Global().PlayerNum() * 50 + 50);

    string GetName(string x);
    string GetScoreBoard();
};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + to_string(round) + " 回合",
            MakeStageCommand(*this, "查看当前游戏进展情况", &RoundStage::Status_,
                VoidChecker("赛况")),
            MakeStageCommand(*this, "刺杀：猜测并刺杀另一名玩家", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Assassinate_,
                AlterChecker<int>({{"刺杀", 0}, {"2", 0}}), ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "目标"), ArithChecker<int64_t>(2, 8, "猜测数字")),
            MakeStageCommand(*this, "反制：选择刺杀你的玩家淘汰", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Counter_,
                AlterChecker<int>({{"反制", 0}, {"3", 0}})),
            MakeStageCommand(*this, "圣盾：若被≥2人选择则使其失效", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Shield_,
                AlterChecker<int>({{"圣盾", 0}, {"4", 0}})),
            MakeStageCommand(*this, "毁灭：最大点数不得分", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Destroy_,
                AlterChecker<int>({{"毁灭", 0}, {"5", 0}})),
            MakeStageCommand(*this, "拼点：与目标比较数字大小", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Compete_,
                AlterChecker<int>({{"拼点", 0}, {"6", 0}}), ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "目标")),
            MakeStageCommand(*this, "弃牌：若被≥2人选择则不得分", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Discard_,
                AlterChecker<int>({{"弃牌", 0}, {"7", 0}})),
            MakeStageCommand(*this, "巨型：下回合不能选6-8", PRIVATE_ONLY | UNREADY_ONLY, &RoundStage::Giant_,
                AlterChecker<int>({{"巨型", 0}, {"8", 0}})))
    {}

  private:

    // ========== 指令处理 ==========

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string score_Board = Main().GetScoreBoard();
        reply() << Markdown(Main().T_Board + score_Board + Main().Board + "</table>" + Main().game_details, Main().image_width);
        return StageErrCode::OK;
    }

    AtomReqErrCode Assassinate_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int, const int64_t target, const int64_t guess)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        if ((int64_t)(pid + 1) == target) {
            reply() << "[错误] 不能选择自己作为目标";
            return StageErrCode::FAILED;
        }
        if (!Main().player_alive_[target - 1]) {
            reply() << "[错误] 目标玩家已被淘汰";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 2;
        Main().player_target_[pid] = target - 1;
        Main().player_guess_[pid] = guess;
        reply() << "行动成功：刺杀 " + to_string(target) + "号，猜测数字 " + to_string(guess);
        return StageErrCode::READY;
    }

    AtomReqErrCode Counter_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 3;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;
        reply() << "行动成功：反制";
        return StageErrCode::READY;
    }

    AtomReqErrCode Shield_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 4;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;
        reply() << "行动成功：圣盾";
        return StageErrCode::READY;
    }

    AtomReqErrCode Destroy_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 5;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;
        reply() << "行动成功：毁灭";
        return StageErrCode::READY;
    }

    AtomReqErrCode Compete_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int, const int64_t target)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        if (Main().player_last_number_[pid] == 8) {
            reply() << "[错误] 您上回合选择了巨型(8)，本回合不能选择 6-8";
            return StageErrCode::FAILED;
        }
        if ((int64_t)(pid + 1) == target) {
            reply() << "[错误] 不能选择自己作为目标";
            return StageErrCode::FAILED;
        }
        if (!Main().player_alive_[target - 1]) {
            reply() << "[错误] 目标玩家已被淘汰";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 6;
        Main().player_target_[pid] = target - 1;
        Main().player_guess_[pid] = 0;
        reply() << "行动成功：拼点 " + to_string(target) + "号";
        return StageErrCode::READY;
    }

    AtomReqErrCode Discard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        if (Main().player_last_number_[pid] == 8) {
            reply() << "[错误] 您上回合选择了巨型(8)，本回合不能选择 6-8";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 7;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;
        reply() << "行动成功：弃牌";
        return StageErrCode::READY;
    }

    AtomReqErrCode Giant_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int)
    {
        if (!Main().player_alive_[pid]) {
            reply() << "[错误] 您已被淘汰";
            return StageErrCode::FAILED;
        }
        if (Main().player_last_number_[pid] == 8) {
            reply() << "[错误] 您上回合选择了巨型(8)，本回合不能选择 6-8";
            return StageErrCode::FAILED;
        }
        Main().player_number_[pid] = 8;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;
        reply() << "行动成功：巨型";
        return StageErrCode::READY;
    }

    // ========== 阶段回调 ==========

    virtual void OnStageBegin() override
    {
        for (int pid = 0; pid < (int)Global().PlayerNum(); pid++) {
            if (!Main().player_alive_[pid]) {
                Global().SetReady(pid);
            }
            Main().player_number_[pid] = 0;
            Main().player_target_[pid] = -1;
            Main().player_guess_[pid] = 0;
        }

        Global().Boardcast() << "第 " + to_string(Main().round_) + " 回合，请玩家私信选择行动。";

        for (int pid = 0; pid < (int)Global().PlayerNum(); pid++) {
            if (Main().player_alive_[pid] && Main().player_last_number_[pid] == 8) {
                Global().Tell(pid) << "提醒：您上回合选择了巨型(8)，本回合不能选择 6-8";
            }
        }

        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int pid = 0; pid < (int)Global().PlayerNum(); pid++) {
            if (!Global().IsReady(pid) && Main().player_alive_[pid]) {
                Main().player_number_[pid] = 0;
                Main().player_target_[pid] = -1;
                Main().player_guess_[pid] = 0;
            }
        }
        Global().Boardcast() << "有玩家超时仍未行动，超时玩家将被淘汰";
        calc();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_number_[pid] = 0;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }

        bool restricted = (Main().player_last_number_[pid] == 8);
        int max_choice = restricted ? 5 : 8;
        int choice = rand() % (max_choice - 2 + 1) + 2;

        Main().player_number_[pid] = choice;
        Main().player_target_[pid] = -1;
        Main().player_guess_[pid] = 0;

        if (choice == 2 || choice == 6) {
            int target;
            do {
                target = rand() % Global().PlayerNum();
            } while (target == (int)pid || !Main().player_alive_[target]);
            Main().player_target_[pid] = target;

            if (choice == 2) {
                Main().player_guess_[pid] = rand() % 7 + 2;
            }
        }

        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        calc();
        return StageErrCode::CHECKOUT;
    }

    // ========== 结算逻辑 ==========

    void calc()
    {
        const int n = Global().PlayerNum();

        vector<int> gain(n, 0);                    // 本回合得分
        vector<bool> effect(n, true);              // 技能是否生效
        vector<bool> eliminated_this_round(n, false); // 本回合是否被淘汰
        vector<vector<int>> chosen(n);             // 被选择的玩家列表
        vector<bool> action_success(n, false);     // 行动特殊效果是否触发

        string round_details = "<font size=2>";

        // === 标记放弃/超时玩家 ===
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && Main().player_number_[i] == 0) {
                round_details += "- 玩家 <font color=red>" + to_string(i + 1) + "号</font> 超时/退出，被淘汰<br/>";
            }
        }

        // === 初始化得分 = 选择的数字 ===
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && Main().player_number_[i] > 0) {
                gain[i] = Main().player_number_[i];
            }
        }

        // === 构建选择列表（仅2刺杀和6拼点有目标） ===
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && Main().player_number_[i] > 0) {
                if ((Main().player_number_[i] == 2 || Main().player_number_[i] == 6) && Main().player_target_[i] >= 0) {
                    chosen[Main().player_target_[i]].push_back(i);
                }
            }
        }

        // === 结算5：毁灭 ===
        bool has5 = false;
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && Main().player_number_[i] == 5) {
                has5 = true;
                break;
            }
        }
        if (has5) {
            int maxNum = 0;
            for (int i = 0; i < n; i++) {
                if (Main().player_alive_[i] && Main().player_number_[i] > 0 && Main().player_number_[i] > maxNum) {
                    maxNum = Main().player_number_[i];
                }
            }
            bool destroyed = false;
            for (int i = 0; i < n; i++) {
                if (Main().player_alive_[i] && Main().player_number_[i] == maxNum) {
                    gain[i] = 0;
                    destroyed = true;
                }
            }
            if (destroyed) {
                round_details += "- <font color=#8B0000>毁灭</font>生效：最大数字为 " + to_string(maxNum) + "，不得分<br/>";
                // 毁灭者自身未被毁灭时标记成功
                for (int i = 0; i < n; i++) {
                    if (Main().player_alive_[i] && Main().player_number_[i] == 5 && maxNum != 5) {
                        action_success[i] = true;
                    }
                }
            }
        }

        // === 结算4：圣盾 和 7：弃牌 ===
        for (int i = 0; i < n; i++) {
            if (!Main().player_alive_[i] || Main().player_number_[i] <= 0) continue;
            if ((int)chosen[i].size() >= 2) {
                if (Main().player_number_[i] == 4) {
                    // 圣盾生效：选择自己的人技能失效且不得分
                    action_success[i] = true;
                    string targets_str = "";
                    for (int j : chosen[i]) {
                        effect[j] = false;
                        gain[j] = 0;
                        if (!targets_str.empty()) targets_str += "、";
                        targets_str += to_string(j + 1) + "号";
                    }
                    round_details += "- <font color=#B8860B>" + to_string(i + 1) + "号 圣盾</font>生效：" + targets_str + " 技能失效且不得分<br/>";
                } else if (Main().player_number_[i] == 7) {
                    // 弃牌被触发：自己不得分
                    gain[i] = 0;
                    action_success[i] = true;
                    round_details += "- " + to_string(i + 1) + "号 <font color=#696969>弃牌</font>被触发（被≥2人选择），本回合不得分<br/>";
                }
            }
        }

        // === 结算6：拼点 ===
        for (int i = 0; i < n; i++) {
            if (!Main().player_alive_[i] || Main().player_number_[i] != 6 || !effect[i]) continue;
            int target = Main().player_target_[i];
            if (target < 0 || !Main().player_alive_[target] || Main().player_number_[target] <= 0) continue;

            int myNum = Main().player_number_[i];
            int targetNum = Main().player_number_[target];

            if (myNum <= targetNum) {
                gain[i] = 0;
            }
            if (targetNum <= myNum) {
                gain[target] = 0;
            }

            if (myNum > targetNum) {
                action_success[i] = true;
                round_details += "- <font color=#1E90FF>" + to_string(i + 1) + "号 拼点</font> " + to_string(target + 1)
                    + "号：" + to_string(myNum) + " > " + to_string(targetNum) + "，" + to_string(i + 1) + "号获胜<br/>";
            } else if (myNum == targetNum) {
                round_details += "- <font color=#1E90FF>" + to_string(i + 1) + "号 拼点</font> " + to_string(target + 1)
                    + "号：" + to_string(myNum) + " = " + to_string(targetNum) + "，平局，双方均不得分<br/>";
            } else {
                round_details += "- <font color=#1E90FF>" + to_string(i + 1) + "号 拼点</font> " + to_string(target + 1)
                    + "号：" + to_string(myNum) + " < " + to_string(targetNum) + "，" + to_string(i + 1) + "号失败<br/>";
            }
        }

        // === 结算2：刺杀 和 3：反制 ===
        for (int i = 0; i < n; i++) {
            if (!Main().player_alive_[i] || Main().player_number_[i] != 2 || !effect[i]) continue;
            int target = Main().player_target_[i];
            if (target < 0 || !Main().player_alive_[target] || Main().player_number_[target] <= 0) continue;

            if (Main().player_number_[target] == 3) {
                // 被反制：刺杀者淘汰
                eliminated_this_round[i] = true;
                action_success[target] = true;
                round_details += "- <font color=#DC143C>" + to_string(i + 1) + "号 刺杀</font> " + to_string(target + 1)
                    + "号，被 <font color=#9932CC>反制</font>！" + to_string(i + 1) + "号被淘汰<br/>";
            } else if (Main().player_number_[target] == Main().player_guess_[i]) {
                // 刺杀成功：目标淘汰，获得对方数字的分值
                eliminated_this_round[target] = true;
                gain[i] = Main().player_number_[target];
                action_success[i] = true;
                round_details += "- <font color=#DC143C>" + to_string(i + 1) + "号 刺杀</font> " + to_string(target + 1)
                    + "号，猜 " + to_string(Main().player_guess_[i]) + "，<font color=green>猜中</font>！"
                    + to_string(target + 1) + "号被淘汰，" + to_string(i + 1) + "号获得 " + to_string(gain[i]) + " 分<br/>";
            } else {
                round_details += "- <font color=#DC143C>" + to_string(i + 1) + "号 刺杀</font> " + to_string(target + 1)
                    + "号，猜 " + to_string(Main().player_guess_[i]) + "，未猜中<br/>";
            }
        }

        // === 淘汰玩家本回合得分清零 ===
        for (int i = 0; i < n; i++) {
            if (eliminated_this_round[i]) {
                gain[i] = 0;
            }
        }

        // === 应用得分 ===
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && Main().player_number_[i] > 0) {
                Main().player_scores_[i] += gain[i];
            }
        }

        // === 构建回合行 (Board) ===
        string b = "<tr><td>" + to_string(Main().round_) + "</td>";
        for (int i = 0; i < n; i++) {
            if (!Main().player_alive_[i]) {
                // 已淘汰玩家显示空格
                b += "<td> </td>";
                continue;
            }
            if (Main().player_number_[i] == 0) {
                // 超时/退出
                b += "<td bgcolor=\"" + Main().giveup_color + "\"><font color=\"#FF0000\">淘汰</font></td>";
                continue;
            }

            // 判断底色
            string color = "";
            if (action_success[i]) {
                switch (Main().player_number_[i]) {
                    case 2: color = " bgcolor=\"" + Main().assassinate_success_color + "\""; break;
                    case 3: color = " bgcolor=\"" + Main().counter_success_color + "\""; break;
                    case 4: color = " bgcolor=\"" + Main().shield_success_color + "\""; break;
                    case 5: color = " bgcolor=\"" + Main().destroy_success_color + "\""; break;
                    case 6: color = " bgcolor=\"" + Main().compete_success_color + "\""; break;
                    case 7: color = " bgcolor=\"" + Main().discard_triggered_color + "\""; break;
                    default: break;
                }
            }

            b += "<td" + color + ">";
            // 行动文本
            switch (Main().player_number_[i]) {
                case 2: b += "刺杀 " + to_string(Main().player_target_[i] + 1) + " " + to_string(Main().player_guess_[i]); break;
                case 3: b += "反制"; break;
                case 4: b += "圣盾"; break;
                case 5: b += "毁灭"; break;
                case 6: b += "拼点 " + to_string(Main().player_target_[i] + 1); break;
                case 7: b += "弃牌"; break;
                case 8: b += "巨型"; break;
            }
            // 得分变化
            if (gain[i] > 0) {
                b += "<br/><font color=\"#1C8A3B\">+" + to_string(gain[i]) + "</font>";
            }
            // 本回合淘汰标记
            if (eliminated_this_round[i]) {
                b += "<br/><font color=\"#FF0000\">淘汰</font>";
            }
            b += "</td>";
        }
        b += "</tr>";
        Main().Board += b;

        // === 处理淘汰 ===
        // 放弃/超时淘汰
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && Main().player_number_[i] == 0) {
                Main().player_alive_[i] = false;
                Main().player_scores_[i] = 0;
                Main().alive_--;
                Global().Eliminate(i);
            }
        }
        // 战斗淘汰
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i] && eliminated_this_round[i]) {
                Main().player_alive_[i] = false;
                Main().player_scores_[i] = 0;
                Main().alive_--;
                Global().Eliminate(i);
            }
        }

        // === 保存上回合数字 ===
        for (int i = 0; i < n; i++) {
            if (Main().player_alive_[i]) {
                Main().player_last_number_[i] = Main().player_number_[i];
            }
        }

        round_details += "</font>";
        Main().game_details = round_details;

        // === 广播结算结果 ===
        string score_Board = Main().GetScoreBoard();
        Global().Boardcast() << Markdown(Main().T_Board + score_Board + Main().Board + "</table>" + Main().game_details, Main().image_width);
    }
};


// ========== MainStage 方法实现 ==========

string MainStage::GetName(string x)
{
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

string MainStage::GetScoreBoard()
{
    string s = "<tr bgcolor=\"" + score_color + "\"><th>得分</th>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        s += "<td>";
        if (player_alive_[i]) {
            s += to_string(player_scores_[i]);
        } else {
            s += "淘汰";
        }
        s += "</td>";
    }
    s += "</tr>";
    return s;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    srand((unsigned int)time(NULL));

    // 构建表头：玩家名称
    T_Board += "<table><tr>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        T_Board += "<th>" + to_string(i + 1) + " 号： " + GetName(Global().PlayerName(i)) + "　</th>";
        if (i % 4 == 3) T_Board += "</tr><tr>";
    }
    T_Board += "</tr><br>";

    // 构建数据表头
    T_Board += "<table style=\"text-align:center\"><tbody>";
    T_Board += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:70px;\">回合</th>";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        T_Board += "<th style=\"width:70px;\">" + to_string(i + 1) + " 号</th>";
    }
    T_Board += "</tr>";

    // 广播玩家列表
    string PreBoard = "本局玩家序号如下：\n";
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + Global().PlayerName(i);
        if (i != (int)Global().PlayerNum() - 1) PreBoard += "\n";
    }

    Global().Boardcast() << PreBoard;

    string score_Board = GetScoreBoard();
    Global().Boardcast() << Markdown(T_Board + score_Board + "</table>", image_width);

    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    // 检查是否有人达到目标分数
    bool reached_goal = false;
    for (int i = 0; i < (int)Global().PlayerNum(); i++) {
        if (player_alive_[i] && player_scores_[i] >= (int64_t)GAME_OPTION(目标分数)) {
            reached_goal = true;
            break;
        }
    }

    if (alive_ > 1 && !reached_goal) {
        setter.Emplace<RoundStage>(*this, ++round_);
        return;
    }

    // 游戏结束
    if (reached_goal) {
        Global().Boardcast() << "有玩家达到 " + to_string(GAME_OPTION(目标分数)) + " 分，游戏结束！";
    } else if (alive_ == 1) {
        Global().Boardcast() << "仅剩 1 名玩家存活，游戏结束！";
    } else {
        Global().Boardcast() << "所有玩家均已淘汰，游戏结束！";
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
