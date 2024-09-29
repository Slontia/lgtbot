// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

#include "table.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "E卡", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "从皇帝、奴隶、市民中选择身份，在不公平的对局中取胜",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    if (GET_OPTION_VALUE(game_options, 模式) < 2 && GET_OPTION_VALUE(game_options, 回合数) % 2 != 0) {
        reply() << "[错误] 为保证游戏公平性，回合数仅能设置偶数（其中人生模式不支持修改回合数），请修正配置";
        return false;
    }
    if (GET_OPTION_VALUE(game_options, 模式) == 1 && GET_OPTION_VALUE(game_options, 回合数) == 12) {
        GET_OPTION_VALUE(game_options, 回合数) = 50;
        reply() << "[提示] 当前为点球模式，默认平局回合数为50";
    }
    if (GET_OPTION_VALUE(game_options, 模式) == 2 && GET_OPTION_VALUE(game_options, 回合数) != 12) {
        GET_OPTION_VALUE(game_options, 回合数) = 12;
        reply() << "[警告] 人生模式不支持修改回合数，回合数固定为12";
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("设定游戏模式",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t& mode, const string& single_mode)
            {
                if (mode == 100) {
                    if (single_mode == "经典") GET_OPTION_VALUE(game_options, 模式) = 0;
                    else if (single_mode == "点球") GET_OPTION_VALUE(game_options, 模式) = 1;
                    else GET_OPTION_VALUE(game_options, 模式) = 2;
                    generic_options.bench_computers_to_player_num_ = 2;
                    return NewGameMode::SINGLE_USER;
                } else {
                    GET_OPTION_VALUE(game_options, 模式) = mode;
                    return NewGameMode::MULTIPLE_USERS;
                }
            },
            AlterChecker<uint32_t>({{"经典", 0}, {"点球", 1}, {"人生", 2}, {"单机", 100}}), OptionalDefaultChecker<BasicChecker<string>>("人生", "单机游戏模式", "")),
};

// ========== GAME STAGES ==========

class TargetStage;
class RoundStage;

class MainStage : public MainGameStage<TargetStage, RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 回合数 
	int round_;
    // 表格及玩家信息
    Table table;

  private:
    void FirstStageFsm(SubStageFsmSetter setter)
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            table.name[pid] = Global().PlayerName(pid);
            if (table.name[pid][0] == '<') {
                table.name[pid] = table.name[pid].substr(1, table.name[pid].size() - 2);
            }
            table.playerPoint[pid] = 0;
        }
        table.Initialize(GAME_OPTION(模式), Global().ResourceDir());
        if (GAME_OPTION(模式) == 2) {
            // 进入[人生模式]设置目标分
            setter.Emplace<TargetStage>(*this);
        } else {
            if (GAME_OPTION(模式) == 0) {
                Global().Boardcast() << "本局游戏为经典模式规则，共进行 " << GAME_OPTION(回合数) << " 回合，先用奴隶胜利 " << GAME_OPTION(目标) << " 次的玩家获胜";
            }
            // 非人生模式直接进入回合阶段
            setter.Emplace<RoundStage>(*this, ++round_);
        }
    }

    void NextStageFsm(TargetStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
            return;
        }
        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
            return;
        }
        auto &point = table.playerPoint;
        auto &target = table.targetScore;
        auto &att = table.attacker;
        auto &def = table.defender;
        // 游戏结束判定
        bool game_end = (GAME_OPTION(模式) == 0 && round_ % 2 == 0 && (point[0] >= GAME_OPTION(目标) || point[1] >= GAME_OPTION(目标))) ||              // [经典]偶数回合提前达到目标分
                        (GAME_OPTION(模式) == 0 && round_ % 2 == 1 && point[0] >= GAME_OPTION(目标) && point[0] > point[1] + 1) ||                      // [经典]奇数回合提前达到目标分0
                        (GAME_OPTION(模式) == 0 && round_ % 2 == 1 && point[1] >= GAME_OPTION(目标) && point[1] > point[0] + 1) ||                      // [经典]奇数回合提前达到目标分1
                        (GAME_OPTION(模式) == 1 && round_ < 10 && point[0] > point[1] + table.GetPlayerLeftRound(1)) ||                                 // [点球]提前获胜0
                        (GAME_OPTION(模式) == 1 && round_ < 10 && point[1] > point[0] + table.GetPlayerLeftRound(0)) ||                                 // [点球]提前获胜1
                        (GAME_OPTION(模式) == 1 && round_ >= 10 && round_ % 2 == 0 && point[0] != point[1]) ||                                          // [点球]10回合后加赛
                        (GAME_OPTION(模式) == 2 && point[att] <= 0) ||                                                                                  // [人生]进攻方血量归零
                        (GAME_OPTION(模式) == 2 && point[def] >= target[att] && point[att] > 12 - round_ && Global().PlayerName(1) != "机器人0号");     // [人生]进攻方达到目标且血量充足
        if ((++round_) <= GAME_OPTION(回合数) && !game_end) {
            setter.Emplace<RoundStage>(*this, round_);
            return;
        }

        // [经典模式]胜负判定
        if (GAME_OPTION(模式) == 0) {
            player_scores_[0] = point[0];
            player_scores_[1] = point[1];
            if (round_ > GAME_OPTION(回合数) && player_scores_[0] < GAME_OPTION(目标) && player_scores_[1] < GAME_OPTION(目标)) {
                Global().Boardcast() << "回合数到达上限，游戏结束\n"
                                     << "最终比分：　" << point[0] << " : " << point[1];
            } else {
                if (player_scores_[0] >= GAME_OPTION(目标) && player_scores_[1] < GAME_OPTION(目标)) {
                    Global().Boardcast() << At(PlayerID(0)) << " 奴隶胜利次数达到目标，获得胜利！";
                } else if (player_scores_[1] >= GAME_OPTION(目标) && player_scores_[0] < GAME_OPTION(目标)) {
                    Global().Boardcast() << At(PlayerID(1)) << " 奴隶胜利次数达到目标，获得胜利！";
                } else {
                    Global().Boardcast() << "双方均达到目标分，游戏平局！";
                }
            }
        }
        // [点球模式]胜负判定
        if (GAME_OPTION(模式) == 1) {
            Global().Boardcast() << Markdown(table.GetShootoutModeTable(round_ < 10 ? round_ : round_ - 1));
            player_scores_[0] = point[0];
            player_scores_[1] = point[1];
            if (player_scores_[0] == player_scores_[1]) {
                Global().Boardcast() << "回合数到达上限，双方平局！";
            } else {
                if (player_scores_[0] > player_scores_[1]) {
                    Global().Boardcast() << "游戏结束，" << At(PlayerID(0)) << " 获得胜利！";
                } else {
                    Global().Boardcast() << "游戏结束，" << At(PlayerID(1)) << " 获得胜利！";
                }
            }
        }
        // [人生模式]胜负判定
        if (GAME_OPTION(模式) == 2) {
            Global().Boardcast() << Markdown(table.GetLiveModeTable());
            if (point[att] <= 0) {
                player_scores_[def] = 1;
                Global().Boardcast() << "游戏结束，防守方 " << At(def) << " 获胜！";
            } else {
                if (point[def] >= target[att]) {
                    player_scores_[att] = 1;
                    if (round_ <= 12) {
                        Global().Boardcast() << "进攻方 " << At(att) << " 达到了目标分且生命大于剩余回合数，提前获得胜利！";
                    } else {
                        Global().Boardcast() << "游戏结束，进攻方 " << At(att) << " 达到了目标分，获得胜利！";
                    }
                } else {
                    player_scores_[def] = 1;
                    Global().Boardcast() << "游戏结束，进攻方未达到目标分，防守方 " << At(def) << " 获胜！";
                }
            }
        }
    }
};

// 设置目标分阶段 [仅人生模式]
class TargetStage : public SubGameStage<>
{
   public:
    TargetStage(MainStage& main_stage)
            : StageFsm(main_stage, "设置目标分" ,
                MakeStageCommand(*this, "设置自己的目标分，分数高的一方为进攻方", &TargetStage::SetTargetScore_, ArithChecker<int64_t>(100, 2000, "目标分")))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请双方玩家设置本局的目标分（范围 100 - 2000），分数更高的一方为本局的进攻方。在游戏结束时，进攻方需达到目标分且生命大于0才能获胜\n"
                             << "·本局游戏不同阵营的获胜得分倍率如下：\n"
                             << "皇帝胜利倍率：" << GAME_OPTION(皇帝) << "倍\n"
                             << "奴隶胜利倍率：" << GAME_OPTION(奴隶) << "倍";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode SetTargetScore_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t score)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行设置";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经成功提交了目标分";
            return StageErrCode::FAILED;
        }
        Main().table.targetScore[pid] = score;
        reply() << "设置目标分为：" << score;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (!Global().IsReady(0) && !Global().IsReady(1)) {
            Global().Boardcast() << "双方均超时，游戏平局";
        } else if (!Global().IsReady(0)) {
            Main().player_scores_[0] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(0)) << " 超时判负";
        } else {
            Main().player_scores_[1] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(1)) << " 超时判负";
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        auto &t = Main().table;
        auto boardcast = Global().Boardcast();
        boardcast << At(PlayerID(0)) << "的目标分为：" << t.targetScore[0] << "\n"
                  << At(PlayerID(1)) << "的目标分为：" << t.targetScore[1] << "\n";
        if (t.targetScore[0] < t.targetScore[1]) {
            t.attacker = 1;
            t.defender = 0;
        } else if (t.targetScore[0] > t.targetScore[1]) {
            t.attacker = 0;
            t.defender = 1;
        } else {
            t.attacker = rand() % 2;
            t.defender = 1 - t.attacker;
            boardcast << "\n双方目标分相同，随机选取进攻方和防守方\n";
        }
        boardcast << "\n本局进攻方为 " << At(t.attacker) << "\n本局防守方为 " << At(t.defender);
        t.playerPoint[t.attacker] = 30;
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        Main().table.targetScore[pid] = 500;
        return StageErrCode::READY;
    }
};

class BitStage;
class GameStage;

class RoundStage : public SubGameStage<BitStage, GameStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
            MakeStageCommand(*this, "查看当前游戏进展情况", &RoundStage::Status_, VoidChecker("赛况")))
    {}

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (GAME_OPTION(模式) == 0) {
            reply() << "当前回合：" << Main().round_ << " / " << GAME_OPTION(回合数) << " 回合\n"
                    << "本局获胜需要的目标分：" << GAME_OPTION(目标) << "\n"
                    << "双方当前比分：　" << Main().table.playerPoint[0] << " : " << Main().table.playerPoint[1];
        } else if (GAME_OPTION(模式) == 1) {
            reply() << Main().table.GetShootoutModeTable(Main().round_);
        } else if (GAME_OPTION(模式) == 2) {
            reply() << Markdown(Main().table.GetLiveModeTable());
        }
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter)
    {
        if (GAME_OPTION(模式) == 2) {
            // [人生模式]进入下注阶段
            if (Main().round_ == 7) {
                Global().Boardcast() << "[提示] 当前进入第3阶段，在后续游戏进程中，进攻方可选择在剩余生命的基础上额外下注 15 点";
            }
            setter.Emplace<BitStage>(Main());
        } else {
            // [点球模式]赛况播报
            if (GAME_OPTION(模式) == 1) {
                Global().Boardcast() << Markdown(Main().table.GetShootoutModeTable(Main().round_));
                if (Main().round_ == 11) {
                    Global().Boardcast() << "[提示] 10回合结束，双方玩家比分相同，进入加赛阶段。从下回合起，若2回合内获胜结果不一致，游戏会立即结束";
                }
            }
            // 非人生模式直接进入游戏阶段
            setter.Emplace<GameStage>(Main(), Main().table.swapPlayer ? Main().round_ % 2 : 1 - Main().round_ % 2);
        }
    }

    // [人生模式]阶段切换
    void NextStageFsm(BitStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
            return;
        }
        Global().Boardcast() << Markdown(Main().table.GetLiveModeTable());
        PlayerID emperor = Main().round_ <= 3 || (Main().round_ >= 7 && Main().round_ <= 9) ? Main().table.attacker : Main().table.defender;
        setter.Emplace<GameStage>(Main(), emperor);
    }

    void NextStageFsm(GameStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        if (reason == CheckoutReason::BY_TIMEOUT || reason == CheckoutReason::BY_LEAVE) {
            return;
        }
    }
};

// 下注阶段 [仅人生模式]
class BitStage : public SubGameStage<>
{
   public:
    BitStage(MainStage& main_stage)
            : StageFsm(main_stage, "下注阶段" ,
                MakeStageCommand(*this, "选择本轮下注", &BitStage::SetBitting_, ArithChecker<int64_t>(1, 45, "下注")))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown(Main().table.GetLiveModeTable());
        Global().Boardcast() << "请 " << At(Main().table.attacker) << " 选择本轮下注";
        Global().SetReady(Main().table.defender);
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode SetBitting_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t bit)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本局为防守方，无需进行下注";
            return StageErrCode::FAILED;
        }
        if (Main().round_ <= 6 && (bit < 0 || bit > Main().table.playerPoint[pid]) ||
            Main().round_ >= 7 && ((bit < 0 || bit > Main().table.playerPoint[pid]) && bit != Main().table.playerPoint[pid] + 15))
        {
            reply() << "[错误] 下注失败：本轮支持的下注范围为 1 ~ " << Main().table.playerPoint[pid]
                    << (Main().round_ > 6 ? "、" + to_string(Main().table.playerPoint[pid] + 15) : "");
            return StageErrCode::FAILED;
        }
        Main().table.LiveModeRecord[1][Main().round_] = bit;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << At(Main().table.attacker) << " 超时判负。";
        Main().player_scores_[Main().table.attacker] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "本轮的下注为：" << Main().table.LiveModeRecord[1][Main().round_];
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto &t = Main().table;
        PlayerID emperor = Main().round_ <= 3 || (Main().round_ >= 7 && Main().round_ <= 9) ? t.attacker : t.defender;
        int bit;
        if (t.playerPoint[t.defender] >= t.targetScore[t.attacker]) {
            bit = 1;
        } else {
            if (pid == emperor) {
                if (t.playerPoint[pid] > 20) {
                    bit = rand() % 10 + 11;
                } else if (t.playerPoint[pid] > 10) {
                    bit = rand() % (t.playerPoint[pid] - 8) + 6;
                } else {
                    bit = rand() % t.playerPoint[pid] + 1;
                }
                if (Main().round_ > 6 && t.playerPoint[t.defender] <= t.targetScore[t.attacker] - 100 && t.playerPoint[pid] < 15 && rand() % 10 == 0) {
                    bit = t.playerPoint[pid] + 15;
                }
            } else {
                if (t.playerPoint[pid] > 15 && rand() % 5 == 0) {
                    bit = rand() % 6 + 5;
                } else if (t.playerPoint[pid] > 5) {
                    bit = rand() % 5 + 1;
                } else {
                    bit = rand() % t.playerPoint[pid] + 1;
                }
            }
        }
        t.LiveModeRecord[1][Main().round_] = bit;
        return StageErrCode::READY;
    }
};

// 游戏阶段
class GameStage : public SubGameStage<>
{
   public:
    GameStage(MainStage& main_stage, const PlayerID& emperor)
            : StageFsm(main_stage, "出牌阶段",
                MakeStageCommand(*this, "选择卡牌", &GameStage::ChooseCard_, AlterChecker<int>({{"市民", 0}, {"皇帝", 1}, {"奴隶", 2}, {"s", 0}, {"h", 1}, {"n", 2}})))
            , player_select_(Global().PlayerNum(), 0)
    {
        this->emperor = emperor;
    }

    // 本回合皇帝方
    PlayerID emperor;
    // 玩家剩余卡牌
    vector<vector<int>> player_cards_;
    // 玩家当回合选择
    vector<int> player_select_;
    // 电脑玩家行动回合
    int ComputerActRound;

    virtual void OnStageBegin() override
    {
        if (player_cards_.empty()) {
            Global().Boardcast() << "本回合 " << At(emperor) << " 为皇帝方，请双方玩家私信裁判出牌，时限 " << to_string(GAME_OPTION(时限)) << " 秒";
            player_cards_.push_back(vector<int>{(int)GAME_OPTION(市民数), emperor == 0, emperor == 1});
            player_cards_.push_back(vector<int>{(int)GAME_OPTION(市民数), emperor == 1, emperor == 0});
            ComputerActRound = rand() % (GAME_OPTION(市民数) + 1);
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode ChooseCard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int64_t card)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行出牌";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经出过牌了";
            return StageErrCode::FAILED;
        }
        if (player_cards_[pid][card] == 0) {
            reply() << "[错误] 您本回合为" << (pid == emperor ? "皇帝" : "奴隶") << "方，不能选择这张卡牌";
            return StageErrCode::FAILED;
        }
        player_select_[pid] = card;
        player_cards_[pid][card]--;
        reply() << "选择 " << CardName_ch[card] << " 成功";
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        if (!Global().IsReady(0) && !Global().IsReady(1)) {
            Global().Boardcast() << "双方均超时，游戏平局";
        } else if (!Global().IsReady(0)) {
            Main().player_scores_[0] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(0)) << " 超时判负";
        } else {
            Main().player_scores_[1] = -1;
            Global().Boardcast() << "玩家 " << At(PlayerID(1)) << " 超时判负";
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(PlayerID(pid)) << " 强退认输。";
        Main().player_scores_[pid] = -1;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        auto &t = Main().table;
        int left_card = player_cards_[0][0] + player_cards_[0][1] + player_cards_[0][2];

        Global().Boardcast() << Markdown(t.GetCardTable(player_select_[t.defender], player_select_[t.attacker], left_card, GAME_OPTION(市民数) + 1));
        
        auto boardcast = Global().Boardcast();
        boardcast << "本回合结果为：\n\n　　" << CardName_ch[player_select_[t.defender]] << " - " << CardName_ch[player_select_[t.attacker]] << "\n\n";
        if (player_select_[0] == player_select_[1]) {
            if (player_cards_[0][0] > 0) {
                boardcast << "双方平！剩余 " << (player_cards_[0][0] + 1) << " 张手牌\n请继续私信裁判出牌，时限 " << to_string(GAME_OPTION(时限)) << " 秒";
                Global().ClearReady(0); Global().ClearReady(1);
                Global().StartTimer(GAME_OPTION(时限));
                return StageErrCode::CONTINUE;
            } else {
                // 结算最后一张
                boardcast << "双方平！自动结算最后 1 张";
                player_select_[emperor] = 1;
                player_select_[1 - emperor] = 2;
                player_cards_[emperor][1] = 0;
                player_cards_[1 - emperor][2] = 0;
                return StageErrCode::CONTINUE;
            }
        }
        PlayerID winner = (player_select_[0] == player_select_[1] + 1 || player_select_[0] == player_select_[1] - 2) ? 0 : 1;
        boardcast << At(winner) << " 获得本回合胜利！\n";

        // 结算本回合分数
        if (GAME_OPTION(模式) == 0) {
            // [经典模式]
            if (winner == emperor) {
                boardcast << "皇帝方获胜，本轮比分不变";
            } else {
                t.playerPoint[winner]++;
                boardcast << "奴隶方获胜！" << At(winner) << " 获得 1 分\n"
                          << "当前比分为：　" << t.playerPoint[t.defender] << " : " << t.playerPoint[t.attacker];
            }
        }
        if (GAME_OPTION(模式) == 1) {
            // [点球模式]
            if (winner != emperor) {
                t.shootoutRecord[emperor][int(ceil(Main().round_ / 2.0))] = 0;
                boardcast << "奴隶方获胜！" << At(emperor) << " 本轮未能得分";
            } else {
                t.playerPoint[winner]++;
                t.shootoutRecord[emperor][int(ceil(Main().round_ / 2.0))] = 1;
                boardcast << "皇帝方获胜！" << At(emperor) << " 获得 1 分\n"
                          << "当前比分为：　" << t.playerPoint[t.defender] << " : " << t.playerPoint[t.attacker];
            }
        }
        if (GAME_OPTION(模式) == 2) {
            // [人生模式]
            int win_point;
            if (winner == t.attacker) {
                t.LiveModeRecord[0][Main().round_] = 1;
                if (t.attacker == emperor) {
                    win_point = t.LiveModeRecord[1][Main().round_] * GAME_OPTION(皇帝);
                    boardcast << "皇帝方获胜：" << GAME_OPTION(皇帝) << "倍分数，赢得了 " << win_point << " 分！";
                } else {
                    win_point = t.LiveModeRecord[1][Main().round_] * GAME_OPTION(奴隶);
                    boardcast << "奴隶方获胜：" << GAME_OPTION(奴隶) << "倍分数，赢得了 " << win_point << " 分！！";
                }
                t.playerPoint[t.defender] += win_point;
            } else {
                t.LiveModeRecord[0][Main().round_] = 0;
                t.playerPoint[t.attacker] -= t.LiveModeRecord[1][Main().round_];
                t.LiveModeRecord[2][Main().round_] = 30 - t.playerPoint[t.attacker];
                boardcast << At(t.attacker) << (t.playerPoint[t.attacker] < 0 ? " 失去了全部的生命！！" : " 失去了 " + to_string(t.LiveModeRecord[1][Main().round_]) + " 点生命，" +
                             (t.playerPoint[t.attacker] == 0 ? "生命值归零！" : "剩余 " + to_string(t.playerPoint[t.attacker]) + " 点生命"));
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        if (player_cards_[pid][0] == ComputerActRound) {
            if (pid == emperor) player_select_[pid] = 1;
            else player_select_[pid] = 2;
        } else {
            player_select_[pid] = 0;
        }
        player_cards_[pid][player_select_[pid]]--;
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

