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
    .name_ = "谁是牛头王", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "玩家照着大小顺序摆牌，尽可能获得更少牛头的游戏",
};
uint64_t MaxPlayerNum(const MyGameOptions& options) { return 10; } // 0 indicates no max-player limits
uint32_t Multiple(const MyGameOptions& options) {
    if (GET_OPTION_VALUE(options, 倍数) != vector<int32_t>{5, 10, 11, 55, 110}) {
        return 0;
    }
    switch (GET_OPTION_VALUE(options, 模式)) {
        case 0: return 1;
        case 1: return 1;
        case 2: return 2;
        case 3: return 4;
        case 4: return 0;
        default: return 0;
    }
}
const MutableGenericOptions k_default_generic_options{};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, MyGameOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    int cardLimit = generic_options_readonly.PlayerNum() * GET_OPTION_VALUE(game_options, 手牌) + GET_OPTION_VALUE(game_options, 行数);
    if (GET_OPTION_VALUE(game_options, 卡牌) < cardLimit) {
        GET_OPTION_VALUE(game_options, 卡牌) = cardLimit;
        reply() << "[警告] 总卡牌数少于最小限制，已自动调整上限为 " << cardLimit;
    }
    vector<int32_t> origin_multiple = {5, 10, 11, 55, 110};
    vector<int32_t> &modified = GET_OPTION_VALUE(game_options, 倍数);
    if (modified != origin_multiple) {
        if (modified.size() < 5) {
            size_t add = modified.size();
            modified.insert(modified.end(), origin_multiple.begin() + add, origin_multiple.end());
        } else if (modified.size() > 5) {
            modified.resize(5);
        }
        reply() << "[警告] 本局卡牌牛头倍数非默认配置\n"
                << "·当前配置为：\n"
                << "　" << modified[0] << "的倍数 -> 2个牛头\n"
                << "　" << modified[1] << "的倍数 -> 3个牛头\n"
                << "　" << modified[2] << "的倍数 -> 5个牛头\n"
                << "　" << modified[3] << "的倍数 -> 7个牛头\n"
                << "　" << modified[4] << "的倍数 -> 10个牛头\n"
                << "·需注意：倍数更大的会优先匹配，倍数相同会优先匹配数量更多的牛头";
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (MyGameOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 6;
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
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 回合数 
	int round_;
    // 表格
    Table table;
    // 玩家
    vector<Player> players;
    // 等待放置卡牌玩家临时列表
    vector<Player> current_players;
    
    
  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown(table.GetTable(true, players, current_players));
        } else {
            reply() << Markdown(table.GetHand(pid, players, current_players));
        }
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter)
    {
        if (GAME_OPTION(模式) == 1) Global().Boardcast() << "[提示] 本局为 22分 模式，当手牌打完时有玩家达到 22 个牛头时，游戏才会结束";
        if (GAME_OPTION(模式) == 2) Global().Boardcast() << "[提示] 本局为 33分 模式，当手牌打完时有玩家达到 33 个牛头时，游戏才会结束";
        if (GAME_OPTION(模式) == 3) Global().Boardcast() << "[提示] 本局为 66分 模式，当手牌打完时有玩家达到 66 个牛头时，游戏才会结束";
        if (GAME_OPTION(模式) == 4) Global().Boardcast() << "[提示] 本局为大胃王模式，每个牛头变更为加分，努力去获取更多的牛头吧！";
        for(int pid = 0; pid < Global().PlayerNum(); pid++) {
            Player newPlayer(pid, Global().PlayerName(pid), Global().PlayerAvatar(pid, 40));
            players.push_back(newPlayer);
        }
        table.Initialize(Global().ResourceDir(), Global().PlayerNum(), GAME_OPTION(手牌), GAME_OPTION(行数), GAME_OPTION(上限), GAME_OPTION(倍数));
        table.ShuffleCards(players, GAME_OPTION(卡牌));
        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        bool game_end = (GAME_OPTION(模式) == 0 && round_ == GAME_OPTION(手牌)) ||
                        (GAME_OPTION(模式) == 1 && table.CheckPlayerHead(22, players)) ||
                        (GAME_OPTION(模式) == 2 && table.CheckPlayerHead(33, players)) ||
                        (GAME_OPTION(模式) == 3 && table.CheckPlayerHead(66, players)) ||
                        (GAME_OPTION(模式) == 4 && round_ == GAME_OPTION(手牌));
        if (!players[0].hand.empty() || !game_end) {
            
            if (players[0].hand.empty()) {
                Global().Boardcast() << "[提示] 手牌用尽但未到达游戏结束条件，将重新洗牌继续游戏！";
                table.tableStatus.clear();
                table.tableStatus.resize(GAME_OPTION(行数));
                table.ShuffleCards(players, GAME_OPTION(卡牌));
            } else if (game_end) {
                Global().Boardcast() << "[提示] 已经有玩家达到目标分数！游戏将在本轮结束时结算分数";
            }
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }
        if (GAME_OPTION(模式) != 4) {
            Global().Boardcast() << "游戏结束！按照当前持有的牛头数结算扣分";
        } else {
            Global().Boardcast() << "游戏结束！本局为大胃王模式，按照牛头数结算加分";
        }
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (GAME_OPTION(模式) == 4) {
                player_scores_[pid] = players[pid].head;
            } else {
                player_scores_[pid] = -players[pid].head;
            }
        }
        Global().Boardcast() << Markdown(table.GetPlayerTable(players));

        if (count(player_scores_.begin(), player_scores_.end(), 0) > 0) {
            if (count(player_scores_.begin(), player_scores_.end(), 0) == 1 && GAME_OPTION(模式) == 0) {
                auto it = find(player_scores_.begin(), player_scores_.end(), 0);
                int pid = distance(player_scores_.begin(), it);
                Global().Achieve(pid, Achievement::不吃牛头);
            }
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (player_scores_[pid] == 0) {
                    if (GAME_OPTION(模式) == 1) Global().Achieve(pid, Achievement::根本不饿);
                    if (GAME_OPTION(模式) == 2) Global().Achieve(pid, Achievement::永远的0);
                    if (GAME_OPTION(模式) == 3) Global().Achieve(pid, Achievement::牛头王);
                }
            }
        }
    }
};

class CardStage;
class PlaceStage;

class RoundStage : public SubGameStage<CardStage, PlaceStage>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合")
    {}

  private:
   void FirstStageFsm(SubStageFsmSetter setter)
    {
        setter.Emplace<CardStage>(Main());
    }

    void NextStageFsm(CardStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        setter.Emplace<PlaceStage>(Main());
    }

    void NextStageFsm(PlaceStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
    {
        // End RoundStage
    }
    
};

class CardStage : public SubGameStage<>
{
   public:
    CardStage(MainStage& main_stage)
            : StageFsm(main_stage, "出牌阶段",
                MakeStageCommand(*this, "从手牌中选一张卡牌", &CardStage::PlayCard_, ArithChecker<int64_t>(1, 999, "编号")))
    {}

    virtual void OnStageBegin() override
    {
        if (Main().players[0].hand.size() > 1) {
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                Global().Tell(pid) << Markdown(Main().table.GetHand(pid, Main().players, Main().current_players));
            }
            Global().Group() << Markdown(Main().table.GetTable(false, Main().players, Main().current_players));
            Global().Boardcast() << "请所有玩家私信选择本回合出牌，时限 " + to_string(GAME_OPTION(时限)) + " 秒";
            Global().StartTimer(GAME_OPTION(时限));
        } else {
            Global().Boardcast() << "仅剩最后一张手牌，将自动出牌";
            HandleStageOver_();
        }
    }

   private:
    AtomReqErrCode PlayCard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int card)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判出牌";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经完成了出牌";
            return StageErrCode::FAILED;
        }
        if (card > GAME_OPTION(卡牌)) {
            reply() << "[错误] 喂，不许印卡！本局使用的卡牌总数：" + to_string(GAME_OPTION(卡牌));
            return StageErrCode::FAILED;
        }
        if (count(Main().players[pid].hand.begin(), Main().players[pid].hand.end(), card) == 0) {
            reply() << "[错误] 您的手牌中不包含此卡牌（" + to_string(card) + "），可通过「赛况」指令查看您的手牌";
            return StageErrCode::FAILED;
        }
        PlayerSelectCard(pid, card);
        reply() << "选择 " + to_string(card) + " 成功";
        return StageErrCode::READY;
    }

    void PlayerSelectCard(const PlayerID pid, const int card) {
        auto &p = Main().players[pid];
        p.current = card;
        p.hand.erase(remove(p.hand.begin(), p.hand.end(), card), p.hand.end());
    }

    void HandleStageOver_()
    {
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (!Global().IsReady(pid)) {
                PlayerSelectCard(pid, Main().players[pid].hand[0]);
                Global().SetReady(pid);
            }
        }
        Main().current_players.clear();
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            Main().current_players.push_back(Main().players[pid]);
        }
        sort(Main().current_players.begin(), Main().current_players.end(), [](const Player &a, const Player &b) {
            return a.current < b.current;
        });
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().HookUnreadyPlayers();
        HandleStageOver_();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleStageOver_();
        return StageErrCode::CHECKOUT;
    }
    
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        int rd = rand() % Main().players[pid].hand.size();
        PlayerSelectCard(pid, Main().players[pid].hand[rd]);
        return StageErrCode::READY;
    }
};

class PlaceStage : public SubGameStage<>
{
   public:
    PlaceStage(MainStage& main_stage)
            : StageFsm(main_stage, "放置阶段",
                MakeStageCommand(*this, "选择一行放置卡牌", &PlaceStage::PlaceCard_, ArithChecker<int64_t>(1, 8, "行号")))
    {}

    string once_BoardCast = "";

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << Markdown(Main().table.GetTable(true, Main().players, Main().current_players));
        PlayerID lastpid = AutoPlaceCard();
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (pid != lastpid) {
                Global().SetReady(pid);
            }
        }
        if (once_BoardCast != "") {
            Global().Boardcast() << once_BoardCast << Markdown(Main().table.GetTable(true, Main().players, Main().current_players));
        }
        if (lastpid != -1) {
            Global().Boardcast() << "请 [" << (lastpid + 1) << "号]" << Global().PlayerName(lastpid) << " 选择一行放置您的卡牌，时限 90 秒";
            Global().StartTimer(90);
        }
    }

   private:
    AtomReqErrCode PlaceCard_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int line)
    {
        if (line > GAME_OPTION(行数)) {
            reply() << "[错误] 行号不存在，本局游戏仅可在 1~" + to_string(GAME_OPTION(行数)) + " 中放置卡牌";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前并非是您的选择回合，或您本回合不需要操作";
            return StageErrCode::FAILED;
        }
        int gain_head = Main().table.PlaceCard(Main().players[pid], line - 1, Main().current_players);
        Global().Boardcast() << "[" << (pid + 1) << "号]" << Global().PlayerName(pid) << " 放置卡牌于第 " + to_string(line) + " 行，吃掉了该行所有的卡牌，增加了 " + to_string(gain_head) + " 个牛头。\n"
                             << Markdown(Main().table.GetTable(true, Main().players, Main().current_players));
        return StageErrCode::READY;
    }

    // 自动放置卡牌，返回下一个需要手动设置的玩家，-1为全部完成
    PlayerID AutoPlaceCard() {
        once_BoardCast = "";
        while(!Main().current_players.empty()) {
            PlayerID pid = Main().current_players[0].id;
            int line = Main().table.CheckPlayerNeedPlace(pid, Main().players);
            if (line != -1) {
                int gain_head = Main().table.PlaceCard(Main().players[pid], line, Main().current_players);
                once_BoardCast += "[" + to_string(pid + 1) + "号]将" + to_string(Main().players[pid].current) + "放置于" + to_string(line + 1) + "行" + (gain_head ? "，增加了" + to_string(gain_head) + "个牛头！" : "") + "\n";
            } else {
                return pid;
            }
        }
        return -1;
    }

    virtual CheckoutErrCode HandleStageOver_()
    {
        // 所有玩家行动完成
        if (Main().current_players.empty()) {
            return StageErrCode::CHECKOUT;
        }
        PlayerID pid = Main().current_players[0].id;
        if (!Global().IsReady(pid)) {
            int line = 0;
            for (int i = 1; i < GAME_OPTION(行数); i++) {
                if (Main().table.GetLineHead(i) < Main().table.GetLineHead(line)) {
                    line = i;
                }
            }
            int gain_head = Main().table.PlaceCard(Main().players[pid], line, Main().current_players);
            Global().Boardcast() << "[" << (pid + 1) << "号]" << Global().PlayerName(pid) << " 行动超时，自动放置于第 " << (line + 1) << " 行" << (gain_head ? "，增加了 " + to_string(gain_head) + " 个牛头。" : "");
            Global().Hook(pid);
            Global().SetReady(pid);
        }
        // 继续下一个玩家行动
        PlayerID lastpid = AutoPlaceCard();
        if (once_BoardCast != "") {
            Global().Boardcast() << once_BoardCast << Markdown(Main().table.GetTable(true, Main().players, Main().current_players));
        } else {
            Global().Boardcast() << Markdown(Main().table.GetTable(true, Main().players, Main().current_players));
        }
        if (lastpid != -1) {
            Global().ClearReady(lastpid);
            Global().Boardcast() << "请 [" << (lastpid + 1) << "号]" << Global().PlayerName(lastpid) << " 选择一行放置您的卡牌，时限 60 秒";
            Global().StartTimer(60);
            return StageErrCode::CONTINUE;
        } else {
            return StageErrCode::CHECKOUT;
        }
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        return HandleStageOver_();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return HandleStageOver_();
    }
    
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        int line = 0;
        for (int i = 1; i < GAME_OPTION(行数); i++) {
            if (Main().table.GetLineHead(i) < Main().table.GetLineHead(line)) {
                line = i;
            }
        }
        int gain_head = Main().table.PlaceCard(Main().players[pid], line, Main().current_players);
        Global().Boardcast() << "[" << (pid + 1) << "号]" << Global().PlayerName(pid) << " 放置卡牌于第 " << (line + 1) << " 行，吃掉了该行所有的卡牌，增加了 " << gain_head << " 个牛头。\n"
                             << Markdown(Main().table.GetTable(true, Main().players, Main().current_players));
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

