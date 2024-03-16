// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <vector>
#include <algorithm>

#include "game_framework/game_stage.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "爆金币"; // the game name which should be unique among all the games
const uint64_t k_max_player = 0; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "铁蛋";
const std::string k_description = "选择行动，与他人博弈抢夺金币的游戏";

const char* const settlement_details[7] = {
    R"EOF(· 完整判定顺序为：
「撤离」→「捡金币」→「夺血条」→「守金币」→「抢金币」→“爆金币”→玩家出局
 - 每个指令的详细判定细节请用指令「#规则 爆金币 指令名」来查看)EOF",

    R"EOF(「撤离」最先判定
 - 在当轮[无敌]，本轮其他玩家对撤离玩家的任何操作都会[无效化]（也不会花费金币）
 - 在玩家撤离当轮，会返还曾对此玩家夺血条的一半金币)EOF",

    R"EOF(「捡金币」在「撤离」后判定
 - 某一数值金币不够发放时，大于等于此数值都会判定[失败])EOF",
 
    R"EOF(「夺血条」在「撤离」后判定
 - 玩家每回合的伤害都会被记录，在爆金币时结算的是[所有回合的总伤害]，在撤离时同样是返还的是[所有回合的总伤害]一半的金币。)EOF",
 
    R"EOF(「守金币」在「撤离」后判定
 · [自守]只在抵挡到伤害时才触发。如果多名玩家同时守一名玩家，所有操作都会结算，例如：
 -【1号】捡金币成功，【2号】守【1号】，【3号】也守【1号】。【1号】获取金币[变为失败]，【2号】和【3号】都能获得【1号】捡的金币（而不是平分）。特别的：如果此时【4号】抢【1号】，因【1号】获取金币失败，所以【4号】抢会判定[失败]。)EOF",

    R"EOF(「抢金币」在「捡金币」「夺血条」「守金币」后判定
 · 仅看[目标当回合的操作是否能获得金币]，具体见以下例子：
 -【1号】抢【2号】，【2号】抢【3号】，【3号】捡金币成功。逐层递进，则【1号】、【2号】都判定[成功]。
 -【1号】抢【2号】，【2号】抢【3号】，【3号】抢【1号】。形成闭环，没有目标获得了金币，所以【1号】、【2号】、【3号】都判定[失败]。
 -【1号】守【2号】，【2号】抢【1号】。根据规则，先判定【1号】守[成功]（因此【1号】获得了金币），所以【2号】抢会判定[成功]。
 -【2号】在此轮对【1号】夺血条导致【1号】出局爆金币（因此【2号】本回合操作获得了金币），【3号】抢【2号】会判定[成功]。
 -【2号】在**前一轮**对【1号】夺血条；【2号】在此轮“捡 0”，【1号】在此轮爆金币（因此【2号】本回合操作并**没有获得金币**，即使金币因【1号】爆金币发生了增长），【3号】抢【2号】会判定[失败]。)EOF",
 
    R"EOF(“爆金币” 与 玩家出局 最后才判定
 - 玩家在爆金币时失金币归零并出局，同时按照记录的总伤害结算夺血条的金币收益。
 - 玩家爆金币时如果金币为负数，依然会归零，同时爆出的金币也会[变成负数]。
 - 玩家出局后不能继续行动，但是仍可能通过其他玩家爆金币来获得金币收益，且这些金币会保留到终局结算。)EOF",
};

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看某指令的具体判定细节（疑难解答汇总）",
            [](const int type) { return settlement_details[type]; },
            AlterChecker<int>({{"判定", 0}, {"撤离", 1}, {"捡金币", 2}, {"夺血条", 3}, {"守金币", 4}, {"抢金币", 5}, {"爆金币", 6}})),
};

std::string GameOption::StatusInfo() const { return ""; }

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 4) {
        reply() << "该游戏至少 4 人参加，当前玩家数为 " << PlayerNum();
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
        player_coins_(option.PlayerNum(), 10),
        player_last_hp_(option.PlayerNum(), 10),
        player_last_coins_(option.PlayerNum(), 10),
        player_out_(option.PlayerNum(), 0),
        player_alive_round_(option.PlayerNum(), 0),
        player_action_(option.PlayerNum(), 'P'),
        player_target_(option.PlayerNum(), -1),
        player_coinselect_(option.PlayerNum(), 0) {}

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }
    
    vector<int64_t> player_scores_;
    int round_;
    
    vector<int64_t> player_hp_;   // 生命值
    vector<int64_t> player_coins_;   // 金币数
    vector<int64_t> player_last_hp_;   // 上回合生命值
    vector<int64_t> player_last_coins_;   // 上回合金币数
    vector<int64_t> player_out_;   // 玩家淘汰参数 0未淘汰 1出局 2撤离

    // 运算辅助
    vector<int64_t> player_alive_round_;   // 玩家存活回合数
    vector<vector<int64_t>> player_total_damage_;   // 每个玩家对所有玩家的总伤害 [进攻][受伤]


    // 游戏阶段参数
    int round_coin;   // 本轮金币数
    vector<char> player_action_;   // 玩家行动 P S G T L
    vector<int64_t> player_target_;   // 指定目标
    vector<int64_t> player_coinselect_;   // 玩家金币选择

    string T_Board = "";   // 表头
    string Board = "";   // 赛况
    int alive_;   // 存活数

    // 颜色样式
    string HP_color = "9CCAF0";   // 血量底色
    string coins_color = "FDD12E";   // 金币底色
    string pickcoins_success_color = "FFEBA3";   // 捡金币成功颜色
    string snatchcoins_success_color = "BAFFA8";   // 抢金币成功颜色
    string guard_success_color = "A0FFFF";   // 守金币成功颜色
    string takehp_success_color = "FFCACA";   // 夺血条成功颜色
    
    string leave_color = "E5E5E5";   // 撤离颜色

    int image_width = option().PlayerNum() < 8 ? option().PlayerNum() * 80 + 70 : (option().PlayerNum() < 16 ? option().PlayerNum() * 60 + 50 : option().PlayerNum() * 40 + 30);

    string GetName(std::string x);
    string GetStatusBoard();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string status_Board = GetStatusBoard();

        string coin_Board = "";
        coin_Board += "<tr><td align=\"left\" colspan=" + to_string(option().PlayerNum() + 1) + "><font size=5>· 本轮金币数：" + to_string(round_coin) + "</font></td></tr>";

        reply() << Markdown(T_Board + status_Board + Board + coin_Board + "</table>", image_width);
        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand("捡金币", &RoundStage::Pick_Up_Coins_,
                                VoidChecker("捡"), ArithChecker<int64_t>(0, 5, "金币数")),
                MakeStageCommand("抢金币", &RoundStage::Snatch_Coins_,
                                VoidChecker("抢"), ArithChecker<int64_t>(1, main_stage.option().PlayerNum(), "对象号码"), ArithChecker<int64_t>(1, 5, "金币数")),
                MakeStageCommand("守金币", &RoundStage::Guard_Coins_,
                                VoidChecker("守"), ArithChecker<int64_t>(1, main_stage.option().PlayerNum(), "对象号码"), ArithChecker<int64_t>(1, 5, "金币数")),
                MakeStageCommand("夺血条", &RoundStage::Take_HP_,
                                VoidChecker("夺"), ArithChecker<int64_t>(1, main_stage.option().PlayerNum(), "对象号码"), ArithChecker<int64_t>(1, 5, "金币数")),
                MakeStageCommand("撤离", &RoundStage::Leave_,
                                VoidChecker("撤离"))) {}

  private:

    string Check_PublicIsReady(bool is_public, PlayerID pid)
    {
        if (is_public) {
            return "[错误] 请私信裁判进行行动";
        }
        if (IsReady(pid)) {
            if (main_stage().player_out_[pid] == 2) {
                return "[错误] 您已经撤离，无需行动";
            } else {
                return "[错误] 您本回合已经行动过了";
            }
        }
        return "OK";
    }

    AtomReqErrCode Pick_Up_Coins_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t coinselect)
    {
        string ret = Check_PublicIsReady(is_public, pid);
        if (ret != "OK") {
            reply() << ret;
            return StageErrCode::FAILED;
        }

        return Selected_(pid, reply, 'P', pid, coinselect);
    }

    AtomReqErrCode Snatch_Coins_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target, const int64_t coinselect)
    {
        string ret = Check_PublicIsReady(is_public, pid);
        if (ret != "OK") {
            reply() << ret;
            return StageErrCode::FAILED;
        }

        if (main_stage().player_out_[target - 1] > 0) {
            reply() << "[错误] 目标已经出局或撤离。";
            return StageErrCode::FAILED;
        }
        if (target == pid + 1) {
            reply() << "[错误] 〈抢金币〉不能指定自己。";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'S', target, coinselect);
    }

    AtomReqErrCode Guard_Coins_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target, const int64_t coinselect)
    {
        string ret = Check_PublicIsReady(is_public, pid);
        if (ret != "OK") {
            reply() << ret;
            return StageErrCode::FAILED;
        }

        if (main_stage().player_out_[target - 1] > 0) {
            reply() << "[错误] 目标已经出局或撤离。";
            return StageErrCode::FAILED;
        }
        if (main_stage().player_action_[pid] == 'G' && main_stage().player_target_[pid] == target - 1) {
            reply() << "[错误] 〈守金币〉不能连续两轮选择同一对象";
            return StageErrCode::FAILED;
        }
        if (main_stage().player_action_[pid] == 'T') {
            reply() << "[错误] 〈守金币〉与〈夺血条〉不能连续两轮选";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'G', target, coinselect);
    }

    AtomReqErrCode Take_HP_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t target, const int64_t coinselect)
    {
        string ret = Check_PublicIsReady(is_public, pid);
        if (ret != "OK") {
            reply() << ret;
            return StageErrCode::FAILED;
        }

        if (main_stage().player_out_[target - 1] > 0) {
            reply() << "[错误] 目标已经出局或撤离。";
            return StageErrCode::FAILED;
        }
        if (target == pid + 1) {
            reply() << "[错误] 〈夺血条〉不能指定自己。";
            return StageErrCode::FAILED;
        }
        if (main_stage().player_action_[pid] == 'T' && main_stage().player_target_[pid] == target - 1) {
            reply() << "[错误] 〈夺血条〉不能连续两轮选择同一对象";
            return StageErrCode::FAILED;
        }
        if (main_stage().player_action_[pid] == 'G') {
            reply() << "[错误] 〈夺血条〉与〈守金币〉不能连续两轮选";
            return StageErrCode::FAILED;
        }
        return Selected_(pid, reply, 'T', target, coinselect);
    }

    AtomReqErrCode Leave_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string ret = Check_PublicIsReady(is_public, pid);
        if (ret != "OK") {
            reply() << ret;
            return StageErrCode::FAILED;
        }
        
        return Selected_(pid, reply, 'L', pid, 0);
    }

    AtomReqErrCode Selected_(const PlayerID pid, MsgSenderBase& reply, char action, const int64_t target, const int64_t coinselect)
    {
        main_stage().player_action_[pid] = action;
        main_stage().player_target_[pid] = target - 1;
        main_stage().player_coinselect_[pid] = coinselect;
        reply() << "行动成功";
        // Returning |READY| means the player is ready. The current stage will be over when all surviving players are ready.
        return StageErrCode::READY;
    }

    virtual void OnStageBegin() override
    {
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_out_[i] == 2) {
                SetReady(i);
            }
        }
        Boardcast() << "本轮爆出了 " + to_string(main_stage().round_coin) + " 枚金币\n请玩家私信选择行动。";
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (IsReady(i) == false) {
                main_stage().player_action_[i] = 'L';
                main_stage().player_target_[i] = 0;
                main_stage().player_coinselect_[i] = 0;
            }
        }
        Boardcast() << "有玩家超时仍未行动，已自动撤离";
        RoundStage::calc();
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        main_stage().player_action_[pid] = 'L';
        main_stage().player_target_[pid] = 0;
        main_stage().player_coinselect_[pid] = 0;
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (main_stage().player_coins_[pid] >= 20) {
            Selected_(pid, reply, 'L', 0, 0);
        } else {
            Selected_(pid, reply, 'P', 0, rand() % 6);
        }
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        RoundStage::calc();
        return StageErrCode::CHECKOUT;
    }

    bool CheckPlayerGainCoins(int target, vector<int> &player_gaincoins_num, int count)
    {
        if (main_stage().player_action_[target] == 'P' || main_stage().player_action_[target] == 'G') {
            if (player_gaincoins_num[target] > 0) {
                return true;
            } else {
                return false;
            }
        }
        if (main_stage().player_action_[target] == 'T') {
            if (main_stage().player_hp_[main_stage().player_target_[target]] > 0) {
                return false;
            } else {
                int T_target = main_stage().player_target_[target];
                int coin_change = -main_stage().player_coinselect_[target] + main_stage().player_total_damage_[target][T_target] * main_stage().player_coins_[T_target] * 0.15;
                if (coin_change > 0) {
                    return true;
                } else {
                    return false;
                }
            }
        }
        if (main_stage().player_action_[target] == 'S') {
            if (player_gaincoins_num[main_stage().player_target_[target]] > 0) {
                return true;
            } else if (player_gaincoins_num[main_stage().player_target_[target]] < 0) {   // 获取标记
                return false;
            }
            if (count > option().PlayerNum()) {
                player_gaincoins_num[target] = -1;   // 标记抢夺失败
                return false;
            }
            return CheckPlayerGainCoins(main_stage().player_target_[target], player_gaincoins_num, count + 1);
        }
        return false;
    }

    void calc() {
        vector<int> player_gaincoins_num(option().PlayerNum(), 0);   // 玩家本回合获得的金币数
        vector<bool> pickcoins_is_guarded(option().PlayerNum(), false);   // 玩家捡金币是否被守金币
        vector<bool> pickcoins_success(option().PlayerNum(), false);   // 玩家执行捡金币是否显示成功
        vector<bool> snatchcoins_success(option().PlayerNum(), false);   // 玩家执行抢金币是否显示成功
        vector<bool> guard_success(option().PlayerNum(), false);   // 玩家执行守金币是否显示成功
        vector<bool> takehp_success(option().PlayerNum(), false);   // 玩家执行夺血条是否显示成功
        
        vector<int> pickCount = {0, 0, 0, 0, 0, 0};
        vector<char> action = main_stage().player_action_;
        vector<int64_t> target = main_stage().player_target_;
        vector<int64_t> coinselect = main_stage().player_coinselect_;

        // 撤离【L】
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (action[i] == 'L' && main_stage().player_out_[i] == 0) {
                Tell(i) << "您已选择撤离，不再参与后续游戏。";
                // 返还总伤害的一半
                for (int j = 0; j < option().PlayerNum(); j++) {
                    main_stage().player_coins_[j] += main_stage().player_total_damage_[j][i] * 0.5;
                }
            }
        }

        // 捡金币【P】
        int coins, pickStop;
        coins = main_stage().round_coin;
        pickStop = 6;
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (action[i] == 'P' && main_stage().player_out_[i] == 0) {
                pickCount[coinselect[i]]++;
            }
        }
        for (int i = 1; i <= 5; i++) {
			if(coins >= pickCount[i] * i) {
				coins -= pickCount[i] * i;
			} else {
                pickStop = i;
                break;
            }
		}
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (action[i] == 'P' && coinselect[i] < pickStop && coinselect[i] > 0 && main_stage().player_out_[i] == 0) {
                main_stage().player_coins_[i] += coinselect[i];
                player_gaincoins_num[i] = coinselect[i];
                pickcoins_success[i] = true;
            }
        }

        // 夺血条【T】（每个玩家依次判定）
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_out_[i] > 0 || action[i] == 'L') continue;

            int hp_taken = 0;
            int hp_guarded = 0;
            for (int j = 0; j < option().PlayerNum(); j++) {
                if (main_stage().player_out_[j] > 0 || action[target[j]] == 'L') continue;
                // 结算夺血条
                if (action[j] == 'T' && target[j] == i) {
                    main_stage().player_coins_[j] -= coinselect[j];
                    main_stage().player_hp_[j] += coinselect[j];
                    player_gaincoins_num[j] = -coinselect[j];
                    main_stage().player_total_damage_[j][target[j]] += coinselect[j];
                    hp_taken += coinselect[j];
                }
                // 记录守护总值
                if (action[j] == 'G' && target[j] == i) {
                    hp_guarded += coinselect[j];
                }
            }
            // 伤害被守护抵消
            if (hp_taken - hp_guarded > 0) {
                main_stage().player_hp_[i] = main_stage().player_hp_[i] - hp_taken + hp_guarded;
            }
            // 自守成功判定
            if (action[i] == 'G' && target[i] == i && hp_taken > 0) {
                int offset = min<int>(hp_taken, coinselect[i]);
                int gain_coins = offset * 2 > 8 ? 8 : offset * 2;
                int gain_hp = offset > 3 ? 3 : offset;
                main_stage().player_coins_[i] += gain_coins;
                main_stage().player_hp_[i] += gain_hp;
                guard_success[i] = true;
            }
        }

        // 守金币【G】
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_out_[i] > 0 || action[target[i]] == 'L') continue;

            if (action[i] == 'G') {
                main_stage().player_coins_[i] -= coinselect[i];
                player_gaincoins_num[i] = -coinselect[i];
                // 对方捡金币且成功
                if (action[target[i]] == 'P' && pickcoins_success[target[i]]) {
                    pickcoins_is_guarded[target[i]] = true;
                    main_stage().player_coins_[i] += coinselect[target[i]];
                    main_stage().player_coins_[target[i]] += coinselect[i];
                    player_gaincoins_num[i] += coinselect[target[i]];
                    guard_success[i] = true;
                }
                // 对方抢金币
                if (action[target[i]] == 'S') {
                    main_stage().player_coins_[i] += coinselect[i] * 2;
                    main_stage().player_coins_[target[i]] -= coinselect[i];
                    player_gaincoins_num[i] += coinselect[i] * 2;
                    guard_success[i] = true;
                }
                // 对方夺血条
                if (action[target[i]] == 'T') {
                    main_stage().player_hp_[target[i]] = 0;
                    int gain_coins = (main_stage().player_coinselect_[i] - 2) * main_stage().player_coins_[target[i]] * 0.3;
                    main_stage().player_coins_[i] += gain_coins;
                    player_gaincoins_num[i] += gain_coins;
                    guard_success[i] = true;
                }
            }
        }
        // 结算被守玩家捡金币的没收
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (action[i] == 'P' && main_stage().player_out_[i] == 0 && pickcoins_is_guarded[i]) {
                main_stage().player_coins_[i] -= coinselect[i];
                player_gaincoins_num[i] = 0;
                pickcoins_success[i] = false;
            }
        }

        // 抢金币【S】
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (action[i] == 'S' && main_stage().player_out_[i] == 0 && action[target[i]] != 'L') {
                if (CheckPlayerGainCoins(target[i], player_gaincoins_num, 0)) {
                    main_stage().player_coins_[i] += coinselect[i];
                    main_stage().player_coins_[target[i]] -= coinselect[i];
                    snatchcoins_success[i] = true;
                } else {
                    main_stage().player_coins_[i] -= coinselect[i];
                    main_stage().player_coins_[target[i]] += coinselect[i];
                }
            }
        }

        // 爆金币
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_hp_[i] <= 0 && main_stage().player_out_[i] == 0) {
                for (int j = 0; j < option().PlayerNum(); j++) {
                    // int gain_coins = main_stage().player_total_damage_[j][i] * main_stage().player_coins_[i] * 0.15;
                    main_stage().player_coins_[j] += main_stage().player_total_damage_[j][i] * main_stage().player_coins_[i] * 0.15;
                    // player_gaincoins_num[j] += gain_coins;
                    // 判定夺血条成功
                    if (action[j] == 'T' && target[j] == i) {
                        takehp_success[j] = true;
                    }
                }
                main_stage().player_coins_[i] = 0;
            }
        }

        // board
        string status_Board = main_stage().GetStatusBoard();

        string b = "<tr><td>R" + to_string(main_stage().round_) + "</td>";
        for (int i = 0; i < option().PlayerNum(); i++) {
            string color = "";
            if (pickcoins_success[i]) {
                color = " bgcolor=\"" + main_stage().pickcoins_success_color + "\"";
            } else if (snatchcoins_success[i]) {
                color = " bgcolor=\"" + main_stage().snatchcoins_success_color + "\"";
            } else if (guard_success[i]) {
                color = " bgcolor=\"" + main_stage().guard_success_color + "\"";
            } else if (takehp_success[i]) {
                color = " bgcolor=\"" + main_stage().takehp_success_color + "\"";
            } else if (action[i] == 'L' && main_stage().player_out_[i] == 0) {
                color = " bgcolor=\"" + main_stage().leave_color + "\"";
            }
            b += "<td" + color + ">";
            if (main_stage().player_out_[i] > 0) {
                b += " ";
            } else {
                if (action[i] == 'P') {
                    b += "捡 " + to_string(coinselect[i]);
                } else if (action[i] == 'S') {
                    b += "抢 " + to_string(target[i] + 1) + " " + to_string(coinselect[i]);
                } else if (action[i] == 'G') {
                    b += "守 " + to_string(target[i] + 1) + " " + to_string(coinselect[i]);
                } else if (action[i] == 'T') {
                    b += "夺 " + to_string(target[i] + 1) + " " + to_string(coinselect[i]);
                } else if (action[i] == 'L') {
                    b += "撤离";
                }
            }
            b += "</td>";
        }
        b += "</tr>";
        main_stage().Board += b;

        // 判定 出局&撤离
        for (int i = 0; i < option().PlayerNum(); i++) {
            if (main_stage().player_out_[i] == 0) {
                // 撤离【L】
                if (action[i] == 'L') {
                    main_stage().player_out_[i] = 2;
                }
                // 出局
                if (main_stage().player_hp_[i] <= 0) {
                    main_stage().player_out_[i] = 1;
                    Eliminate(i);
                }
                // 保存存活回合数
                if (main_stage().player_out_[i] > 0) {
                    main_stage().alive_--;
                    main_stage().player_alive_round_[i] = main_stage().round_;
                }
            }
        }

        string coin_Board = "";
        if (main_stage().alive_ > 0 && main_stage().round_ < GET_OPTION_VALUE(option(), 回合数)) {
            main_stage().round_coin = rand() % (main_stage().alive_ + 1) + main_stage().alive_ * 2;
            coin_Board += "<tr><td align=\"left\" colspan=" + to_string(option().PlayerNum() + 1) + "><font size=5>· 本轮金币数：" + to_string(main_stage().round_coin) + "</font></td></tr>";
        }

        Boardcast() << Markdown(main_stage().T_Board + status_Board + main_stage().Board + coin_Board + "</table>", main_stage().image_width);

        for (int i = 0; i < option().PlayerNum(); i++) {
            main_stage().player_last_hp_[i] = main_stage().player_hp_[i];
            main_stage().player_last_coins_[i] = main_stage().player_coins_[i];
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
    status_Board += "<tr bgcolor=\""+ HP_color +"\"><th>血量</th>";
    for (int i = 0; i < option().PlayerNum(); i++) {
        status_Board += "<td>";
        if (player_out_[i] == 0 && player_action_[i] != 'L') {
            if (player_hp_[i] > 0) {
                status_Board += to_string(player_last_hp_[i]);
                if (player_hp_[i] > player_last_hp_[i]) {
                    status_Board += "<font color=\"#1C8A3B\">+" + to_string(player_hp_[i] - player_last_hp_[i]);
                } else if (player_hp_[i] < player_last_hp_[i]) {
                    status_Board += "<font color=\"#FF0000\">-" + to_string(player_last_hp_[i] - player_hp_[i]);
                }
            } else {
                status_Board += "<font color=\"#FF0000\"><b>" + to_string(player_hp_[i]) + "</b>";
            }
            if (player_hp_[i] != player_last_hp_[i]) {
                status_Board += "</font>";
            }
        } else if (player_out_[i] == 1) {
            status_Board += "出局";
        } else {
            status_Board += "撤离";
        }
        status_Board += "</td>";
    }
    status_Board += "</tr><tr bgcolor=\""+ coins_color +"\"><th>金币</th>";
    for (int i = 0; i < option().PlayerNum(); i++) {
        status_Board += "<td>";
        status_Board += to_string(player_last_coins_[i]);
        if (player_coins_[i] > player_last_coins_[i]) {
            status_Board += "<font color=\"#1C8A3B\">+" + to_string(player_coins_[i] - player_last_coins_[i]);
        } else if (player_coins_[i] < player_last_coins_[i]) {
            status_Board += "<font color=\"#FF0000\">-" + to_string(player_last_coins_[i] - player_coins_[i]);
        }
        if (player_coins_[i] != player_last_coins_[i]) {
            status_Board += "</font>";
        }
        status_Board += "</td>";
    }
    status_Board += "</tr>";
    return status_Board;
}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    srand((unsigned int)time(NULL));
    alive_ = option().PlayerNum();
    player_total_damage_.resize(option().PlayerNum());

    for (int i = 0; i < option().PlayerNum(); i++) {
        player_hp_[i] = 10;
        player_last_hp_[i] = 10;
        player_coins_[i] = GET_OPTION_VALUE(option(), 金币);
        player_last_coins_[i] = GET_OPTION_VALUE(option(), 金币);
        player_total_damage_[i].resize(option().PlayerNum());
    }

    for (int i = 0; i < option().PlayerNum(); i++) {
        for (int j = 0; j < option().PlayerNum(); j++) {
            player_total_damage_[i][j] = 0;
        }
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
    T_Board += "</tr>";

    string status_Board = GetStatusBoard();

    string coin_Board = "";
    round_coin = rand() % (alive_ + 1) + alive_ * 2;
    coin_Board += "<tr><td align=\"left\" colspan=" + to_string(option().PlayerNum() + 1) + "><font size=5>· 本轮金币数：" + to_string(round_coin) + "</font></td></tr>";

    string PreBoard = "";
    PreBoard += "本局玩家序号如下：\n";
    for (int i = 0; i < option().PlayerNum(); i++) {
        PreBoard += to_string(i + 1) + " 号：" + PlayerName(i);

        if (i != (int)option().PlayerNum() - 1) {
        PreBoard += "\n";
        }
    }

    Boardcast() << PreBoard;
    Boardcast() << Markdown(T_Board + status_Board + coin_Board + "</table>", image_width);

    return std::make_unique<RoundStage>(*this, ++round_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    if ((++round_) <= GET_OPTION_VALUE(option(), 回合数) && alive_ > 0) {
        return std::make_unique<RoundStage>(*this, round_);
    }

    if (alive_ > 0) {
        Boardcast() << "达到最大回合数限制，剩余玩家自动撤离";
    } else {
        Boardcast() << "所有玩家均出局或撤离，游戏结束";
    }

    for(int i = 0; i < option().PlayerNum(); i++) {
        if (player_out_[i] > 0) {
            player_scores_[i] = player_coins_[i] - player_alive_round_[i];
        } else {
            player_scores_[i] = player_coins_[i] - round_ + 1;
        }
    }
    // Returning empty variant means the game will be over.
    return {};
}

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"
