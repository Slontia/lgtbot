// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <algorithm>
#include <chrono>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

#include "board.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "寻宝猎人", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "深入秘境搜集金币，在被怪物吞噬前及时撤离的冒险游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 5; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

const uint32_t k_min_player_num = 4;

bool AdaptOptions(ChildMsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < k_min_player_num) {
        reply() << "该游戏至少 " << k_min_player_num << " 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = k_min_player_num;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class ChoiceStage;

class MainStage : public MainGameStage<ChoiceStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前赛况", &MainStage::Status_, VoidChecker("赛况")))
        , board_(Global().ResourceDir())
        , round_(0)
    {
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override
    {
        return static_cast<size_t>(pid) < board_.players.size() ? board_.players[pid].gold : 0;
    }

    Board board_;
    // 已经进行的选择回合数
    int round_;
    // 最近一次公布的赛况，「赛况」指令直接重发该图
    string board_status_;

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        reply() << Markdown(board_status_, board_.TableWidth());
        return StageErrCode::OK;
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        board_.Initialize();
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board_.players.emplace_back(ExtractNickname(Global().PlayerName(pid)),
                                        Global().PlayerAvatar(pid, k_avatar_size));
        }

        Global().Boardcast() << "探险开始！队伍进入秘境，最初的 " << k_leading_wealth_num << " 张卡牌将自动翻开，无需任何操作。";
        for (int i = 0; i < k_leading_wealth_num; ++i) {
            DrawCard_();
        }
        board_status_ = board_.GetTableMarkdown();

        setter.Emplace<ChoiceStage>(*this, ++round_);
    }

    virtual void NextStageFsm(ChoiceStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        SettleRetreat_();
        if (board_.ExploringCount() == 0) {
            Finish_();
            return;
        }
        DrawCard_();
        if (board_.ExploringCount() == 0 || board_.pool.empty()) {
            Finish_();
            return;
        }
        board_status_ = board_.GetTableMarkdown();

        setter.Emplace<ChoiceStage>(*this, ++round_);
    }

    // 结算本回合选择撤离的玩家：平分沿途遗留的金币，独自撤离时还能带走全部珍宝
    void SettleRetreat_()
    {
        vector<PlayerID> leavers;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (board_.players[pid].IsExploring() && board_.players[pid].choice == Choice::RETREAT) {
                leavers.emplace_back(pid);
            }
        }

        auto sender = Global().Boardcast();
        sender << "选择完毕，以下玩家选择了撤离：";
        if (leavers.empty()) {
            sender << "\n无";
        } else {
            for (const PlayerID pid : leavers) {
                sender << "\n" << At(pid);
            }
            const int left = static_cast<int>(leavers.size());
            int distribution = 0;
            int treasure_taken = 0;
            int best_treasure = 0;
            for (Card& card : board_.past) {
                if (card.IsWealth()) {
                    distribution += card.real_value / left;
                    card.real_value %= left;
                } else if (card.IsTreasure() && left == 1) {
                    if (card.real_value > 0) {
                        ++treasure_taken;
                        best_treasure = max(best_treasure, card.real_value);
                    }
                    distribution += card.real_value;
                    card.real_value = 0;
                }
            }
            for (Card& card : board_.events) {
                if (card.value == QUIT_REWARD) {
                    distribution += card.real_value / left;
                    card.real_value %= left;
                }
            }
            for (const PlayerID pid : leavers) {
                Player& player = board_.players[pid];
                player.gold += distribution;
                player.state = PlayerState::RETREATED;
                player.record.retreat_round = round_;
                player.record.retreat_cards = static_cast<int>(board_.past.size());
                player.record.retreat_alone = left == 1;
                player.record.treasure_taken = treasure_taken;
                player.record.best_treasure = best_treasure;
            }
            sender << "\n\n" << (left > 1 ? "撤离者每人分得" : "撤离者独自获得了") << distribution << "枚金币";
        }
        EraseEvents_({QUIT_REWARD});
    }

    // 翻开牌堆顶部的卡牌，公布卡面并结算它带来的效果
    void DrawCard_()
    {
        Card card = board_.pool.front();
        board_.pool.erase(board_.pool.begin());
        const string card_markdown = board_.GetCardMarkdown(card);

        string msg = "探险继续，发现了：" + card.name + "\n" + card.description;
        switch (card.type) {
            case CardType::WEALTH:   msg += SettleWealth_(card);   break;
            case CardType::TREASURE: msg += SettleTreasure_(card); break;
            case CardType::MONSTER:  msg += SettleMonster_(card);  break;
            case CardType::EVENT:    msg += SettleEvent_(card);    break;
        }

        Global().Boardcast() << Markdown(card_markdown, k_card_image_width) << msg;
        board_.past.emplace_back(card);
    }

    string SettleWealth_(Card& card)
    {
        string msg;
        int value = card.value;
        for (const Card& event : board_.events) {
            if (event.value == EXTRA_WEALTH) {
                value += event.real_value;
                msg += "\n奇遇卡生效，额外获得" + to_string(event.real_value) + "枚金币";
                break;
            }
            if (event.value == DOUBLE_WEALTH) {
                value *= 2;
                msg += "\n奇遇卡生效，获得的金币数翻倍";
                break;
            }
        }
        EraseEvents_({EXTRA_WEALTH, DOUBLE_WEALTH});

        const int explorer_num = board_.ExploringCount();
        const int distribution = explorer_num > 0 ? value / explorer_num : 0;
        const int remain = explorer_num > 0 ? value % explorer_num : value;
        for (Player& player : board_.players) {
            if (player.IsExploring()) {
                player.gold += distribution;
                if (player.record.saw_sudden_death) {
                    player.record.gold_after_sudden_death += distribution;
                }
            }
        }
        card.real_value = remain;
        msg += "\n每人分得" + to_string(distribution) + "枚金币，留下" + to_string(remain) + "枚金币";
        return msg;
    }

    string SettleTreasure_(Card& card)
    {
        string msg;
        for (const Card& event : board_.events) {
            if (event.value == MAX_TREASURE) {
                card.value = k_max_treasure_value_;
                card.real_value = card.value;
                msg += "\n奇遇卡生效，获得了价值最高的宝物";
                break;
            }
            if (event.value == DOUBLE_TREASURE) {
                card.value *= 2;
                card.real_value = card.value;
                msg += "\n奇遇卡生效，获得的宝物价值翻倍";
                break;
            }
        }
        EraseEvents_({MAX_TREASURE, DOUBLE_TREASURE});
        return msg + "\n只有独自撤离的玩家能得到这件宝物。";
    }

    string SettleMonster_(Card& card)
    {
        for (Player& player : board_.players) {
            if (player.IsExploring()) {
                player.record.met_monster[card.value] = true;
            }
        }
        const auto has_event = [this](const int value)
            {
                return any_of(board_.events.begin(), board_.events.end(),
                        [value](const Card& event) { return event.value == value; });
            };
        if (has_event(SUDDEN_DEATH)) {
            DevourExplorers_();
            return "\n怪物突袭！未撤离的玩家全部被吞噬。";
        }
        EraseEvents_({SUDDEN_DEATH});

        const bool met_before = any_of(board_.past.begin(), board_.past.end(),
                [&card](const Card& past_card) { return past_card.IsMonster() && past_card.value == card.value; });
        if (met_before) {
            DevourExplorers_();
            return "\n遭遇相同怪物两次，未撤离的玩家全部被怪物吞噬。";
        }
        return "\n遭遇相同怪物两次时，仍未撤离的玩家将被怪物吞噬，失去所有金币。";
    }

    string SettleEvent_(Card& card)
    {
        for (Player& player : board_.players) {
            if (!player.IsExploring()) {
                continue;
            }
            ++player.record.event_seen;
            if (card.value == SUDDEN_DEATH) {
                player.record.saw_sudden_death = true;
            }
        }

        string msg;
        switch (card.value) {
            case EXTRA_WEALTH:
                card.real_value = board_.Rand(7, 9);
                msg += "\n将增加" + to_string(card.real_value) + "枚金币";
                board_.events.emplace_back(card);
                break;
            case DOUBLE_WEALTH:
            case MAX_TREASURE:
            case DOUBLE_TREASURE:
            case SUDDEN_DEATH:
                board_.events.emplace_back(card);
                break;
            case QUIT_REWARD:
                card.real_value = board_.Rand(5, 20);
                msg += "\n将平分" + to_string(card.real_value) + "枚金币";
                board_.events.emplace_back(card);
                break;
            case WEALTH_INCREASE:
                for (Card& past_card : board_.past) {
                    if (past_card.IsWealth()) {
                        past_card.real_value += board_.Rand(1, 2);
                    }
                }
                break;
            case WEALTH_DISTRIBUTE: {
                const vector<size_t> wealth_indexes = PastWealthIndexes_();
                if (!wealth_indexes.empty()) {
                    int total = board_.Rand(9, 13);
                    msg += "\n总计增加" + to_string(total) + "枚金币";
                    for (size_t i = 0; total > 0; i = (i + 1) % wealth_indexes.size()) {
                        ++board_.past[wealth_indexes[i]].real_value;
                        --total;
                    }
                }
                break;
            }
            case WEALTH_RESET: {
                const vector<size_t> wealth_indexes = PastWealthIndexes_();
                if (!wealth_indexes.empty()) {
                    Card& target = board_.past[wealth_indexes[board_.Rand(0, static_cast<int>(wealth_indexes.size()) - 1)]];
                    target.real_value = target.value;
                }
                break;
            }
            case TREASURE_CONVERT: {
                const vector<size_t> wealth_indexes = PastWealthIndexes_();
                if (!wealth_indexes.empty()) {
                    board_.past[wealth_indexes[board_.Rand(0, static_cast<int>(wealth_indexes.size()) - 1)]] =
                        MakeTreasureCard(board_.Rand(10, 20));
                }
                break;
            }
            default:
                break;
        }
        return msg;
    }

    // 公布未翻开的剩余卡牌并结算名次
    void Finish_()
    {
        board_.past.insert(board_.past.end(), board_.pool.begin(), board_.pool.end());
        board_.pool.clear();
        board_status_ = board_.GetTableMarkdown();
        Global().Boardcast() << Markdown(board_status_, board_.TableWidth());

        vector<PlayerID> order;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            order.emplace_back(pid);
        }
        stable_sort(order.begin(), order.end(),
                [this](const PlayerID a, const PlayerID b) { return board_.players[a].gold > board_.players[b].gold; });

        Global().Boardcast() << "探险结束，感谢大家的参与！";

        VerdictateAchievements_();
    }

    void VerdictateAchievements_()
    {
        int best_gold = 0;
        int worst_gold = numeric_limits<int>::max();
        for (const Player& player : board_.players) {
            best_gold = max(best_gold, player.gold);
            worst_gold = min(worst_gold, player.gold);
        }
        // 全员金币相同时无人算作最高，避免开局全体撤离这种平局白送成就
        const bool has_richest = best_gold > 0 && best_gold > worst_gold;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            const Player& player = board_.players[pid];
            const Record& record = player.record;
            if (player.gold >= k_rich_gold_) {
                Global().Achieve(pid, Achievement::满载而归);
            }
            if (record.retreat_alone && record.treasure_taken >= k_solo_treasure_num_) {
                Global().Achieve(pid, Achievement::珠光宝气);
            }
            if (record.escaped_before_wipe) {
                Global().Achieve(pid, Achievement::虎口脱险);
            }
            const bool retreated = player.state == PlayerState::RETREATED;
            if (retreated && record.retreat_cards >= k_deep_cards_) {
                Global().Achieve(pid, Achievement::深不可测);
            }
            if (retreated && record.gold_after_sudden_death >= k_sudden_death_gold_) {
                Global().Achieve(pid, Achievement::与狼共舞);
            }
            if (record.retreat_alone && record.best_treasure >= k_precious_treasure_) {
                Global().Achieve(pid, Achievement::稀世珍宝);
            }
            if (retreated && record.event_seen >= k_event_num) {
                Global().Achieve(pid, Achievement::见多识广);
            }
            const bool is_richest = has_richest && player.gold == best_gold;
            if (is_richest && record.MetMonsterTypes() == k_monster_type_count) {
                Global().Achieve(pid, Achievement::破釜沉舟);
            }
            if (is_richest && record.retreat_round == 1) {
                Global().Achieve(pid, Achievement::急流勇退);
            }
        }
    }

    void DevourExplorers_()
    {
        for (Player& player : board_.players) {
            if (player.IsExploring()) {
                player.gold = 0;
                player.state = PlayerState::DEVOURED;
            } else if (player.state == PlayerState::RETREATED && player.record.retreat_round == round_) {
                // 本回合刚刚撤离，恰好躲过了这次团灭
                player.record.escaped_before_wipe = true;
            }
        }
    }

    void EraseEvents_(const initializer_list<int> values)
    {
        board_.events.erase(remove_if(board_.events.begin(), board_.events.end(),
                    [&values](const Card& card)
                    {
                        return find(values.begin(), values.end(), card.value) != values.end();
                    }),
                board_.events.end());
    }

    vector<size_t> PastWealthIndexes_() const
    {
        vector<size_t> indexes;
        for (size_t i = 0; i < board_.past.size(); ++i) {
            if (board_.past[i].IsWealth()) {
                indexes.emplace_back(i);
            }
        }
        return indexes;
    }

    static constexpr int k_max_treasure_value_ = 20;
    static constexpr int k_rich_gold_ = 100;         // 「满载而归」所需的金币数
    static constexpr int k_solo_treasure_num_ = 3;   // 「珠光宝气」所需的珍宝件数
    static constexpr int k_deep_cards_ = 25;         // 「深不可测」所需的已翻开卡牌数
    static constexpr int k_sudden_death_gold_ = 30;  // 「与狼共舞」在突袭生效后于路上分得的金币
    static constexpr int k_precious_treasure_ = 30;  // 「价值连城」所需的单件珍宝价值
};

class ChoiceStage : public SubGameStage<>
{
  public:
    ChoiceStage(MainStage& main_stage, const int round)
        : StageFsm(main_stage, "第 " + to_string(round) + " 回合",
                MakeStageCommand(*this, "继续深入秘境", CommandFlag::PRIVATE_ONLY, &ChoiceStage::Continue_,
                                 VoidChecker("继续", "Y", "y")),
                MakeStageCommand(*this, "返回营地，带走沿途遗留的金币", CommandFlag::PRIVATE_ONLY, &ChoiceStage::Retreat_,
                                 VoidChecker("撤离", "N", "n")),
                MakeStageCommand(*this, "开启或关闭自动探险，挂机期间每回合自动继续", &ChoiceStage::AutoExplore_, VoidChecker("自动探险", "自动")))
    {
    }

    virtual void OnStageBegin() override
    {
        // 仍在探险的玩家是否已经全部挂机，决定本回合采用哪种等待时间
        bool all_hooked = true;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            const Player& player = Main().board_.players[pid];
            if (player.IsExploring() && !player.hook_status) {
                all_hooked = false;
                break;
            }
        }

        // 已经离开秘境的玩家不再行动；挂机玩家默认继续探险，同样无需等待
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().board_.players[pid];
            player.choice = Choice::NONE;
            if (!player.IsExploring() || (player.hook_status && !all_hooked)) {
                Global().SetReady(pid);
            }
        }

        short_round_ = all_hooked;
        grace_used_ = false;
        begin_time_ = chrono::steady_clock::now();

        const uint32_t sec = all_hooked ? k_hook_timeout : GAME_OPTION(时限);
        auto sender = Global().Boardcast();
        sender << Markdown(Main().board_status_, Main().board_.TableWidth())
               << "请仍在探险的玩家私信选择【继续】或【撤离】，撤离者将平分路上遗留的金币，"
                  "随后退出探险，不再获得任何金币。";
        if (all_hooked) {
            sender << "\n\n[提示] 所有玩家正在自动探险，等待时间将缩短至 " << to_string(k_hook_timeout)
                   << " 秒，发送「自动探险」解除挂机可恢复正常时限";
        } else {
            sender << "时限 " << sec << " 秒，超时未选择将自动继续探险。";
        }
        Global().StartTimer(sec);
    }

    // 存在挂机玩家时，即使其余玩家已行动完毕也要把回合撑满最短等待时间，留出挂机玩家发送指令重新参与本回合的机会
    virtual CheckoutErrCode OnStageOver() override
    {
        const uint32_t remain = GraceRemainSeconds_();
        if (remain == 0) {
            return StageErrCode::CHECKOUT;
        }
        grace_used_ = true;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            const Player& player = Main().board_.players[pid];
            if (player.IsExploring() && player.hook_status) {
                Global().ClearReady(pid);  // 暂不认可挂机玩家的准备状态，本回合继续等待
            }
        }
        // 剩余时间过短时静默等待，避免发出玩家根本来不及响应的提示
        if (remain >= k_grace_notice_seconds) {
            Global().Boardcast() << "[提示] 其余玩家已行动完毕，本回合将在 " << remain << " 秒后自动结算，正在自动探险的玩家此时仍可发送指令参与";
        }
        Global().StartTimer(remain);
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        vector<PlayerID> hooked_players;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().board_.players[pid];
            if (!player.IsExploring() || player.choice != Choice::NONE || player.hook_status) {
                continue;
            }
            player.hook_status = true;
            hooked_players.emplace_back(pid);
            Global().Tell(pid) << "您超时未选择，已进入挂机状态，之后的回合将自动继续探险且不再等待您，发送行动指令即可解除";
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Player& player = Main().board_.players[pid];
        if (player.IsExploring()) {
            player.gold = 0;
            player.state = PlayerState::DEVOURED;
            player.choice = Choice::NONE;
            Global().Boardcast() << At(pid) << " 退出了游戏，失去全部金币并离开探险队";
        }
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        Player& player = Main().board_.players[pid];
        if (!player.IsExploring()) {
            return StageErrCode::OK;
        }
        player.choice = ShouldRetreat_(player) ? Choice::RETREAT : Choice::CONTINUE;
        return StageErrCode::READY;
    }

  private:
    // 比较「继续一张牌的期望收益」与「被吞噬时的期望损失」，据此决定去留
    bool ShouldRetreat_(const Player& player)
    {
        Board& board = Main().board_;

        bool met[k_monster_type_count + 1] = {};
        int monster_num = 0;
        int armed_num = 0;  // 已经出现过一次、再出现就会吞噬全员的怪物种类数
        for (const Card& card : board.past) {
            if (!card.IsMonster()) {
                continue;
            }
            ++monster_num;
            if (!met[card.value]) {
                met[card.value] = true;
                ++armed_num;
            }
        }
        const bool sudden_death = any_of(board.events.begin(), board.events.end(),
                [](const Card& card) { return card.value == SUDDEN_DEATH; });
        // 「怪物突袭」生效时任何一张怪物都会终结游戏，否则只有已经出现过的种类才致命
        const int lethal_num = sudden_death ? k_monster_num - monster_num : armed_num;
        const int remain_num = static_cast<int>(board.pool.size());
        if (lethal_num <= 0 || remain_num <= 0) {
            return false;  // 下一张牌不可能被吞噬时绝不撤离
        }
        const double risk = min(1.0, static_cast<double>(lethal_num) / remain_num);

        // 被吞噬会一并失去已到手的金币和沿途本可带走的遗留
        const int explorer_num = max(1, board.ExploringCount());
        int pot = 0;
        for (const Card& card : board.past) {
            if (card.IsWealth()) {
                pot += card.real_value;
            } else if (card.IsTreasure()) {
                pot += card.real_value / explorer_num;
            }
        }
        const double loss = player.gold + pot;
        const double gain = k_coins_per_card_ / explorer_num;

        // 收益风险比越高越倾向撤离，保留随机性避免所有电脑在同一回合一起撤离
        const double ratio = risk * loss / max(0.01, (1.0 - risk) * gain);
        const int chance = min(k_max_retreat_chance_, static_cast<int>(ratio * k_retreat_chance_scale_));
        return board.Rand(1, 100) <= chance;
    }

    static constexpr double k_coins_per_card_ = 4.5;   // 牌堆中每张牌平均带来的金币
    static constexpr int k_retreat_chance_scale_ = 45; // 收益风险比换算成撤离概率的系数
    static constexpr int k_max_retreat_chance_ = 92;   // 撤离概率上限

    AtomReqErrCode Continue_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        return MakeChoice_(pid, reply, Choice::CONTINUE);
    }

    AtomReqErrCode Retreat_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        return MakeChoice_(pid, reply, Choice::RETREAT);
    }

    // 开启后每回合自动继续探险，其他玩家行动完毕即进入下一回合；再次执行可关闭
    AtomReqErrCode AutoExplore_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        Player& player = Main().board_.players[pid];
        if (!player.IsExploring()) {
            reply() << "[错误] 您已经离开秘境，无需继续操作";
            return StageErrCode::FAILED;
        }
        if (player.hook_status) {
            player.hook_status = false;
            reply() << "已关闭自动探险，请私信选择【继续】或【撤离】";
            RestoreNormalRound_(pid);
            if (player.choice == Choice::NONE) {
                Global().ClearReady(pid);  // 本回合尚未做出选择，重新等待该玩家
            }
            return StageErrCode::OK;
        }
        player.hook_status = true;
        reply() << "已开启自动探险，之后的回合将自动继续探险且不再等待您，再次执行该指令或做出选择即可解除";
        if (player.choice != Choice::NONE) {
            return StageErrCode::OK;  // 本回合已经做出选择，无需再次标记
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode MakeChoice_(const PlayerID pid, ChildMsgSenderBase& reply, const Choice choice)
    {
        Player& player = Main().board_.players[pid];
        if (!player.IsExploring()) {
            reply() << "[错误] 您已经离开秘境，无需继续操作";
            return StageErrCode::FAILED;
        }
        // 挂机玩家已被提前标记为准备完毕，因此用本回合的选择判断是否重复行动
        if (player.choice != Choice::NONE) {
            reply() << "[错误] 您已经做出过本回合的选择";
            return StageErrCode::FAILED;
        }
        player.choice = choice;
        auto sender = reply();
        if (player.hook_status) {
            player.hook_status = false;
            // 撤离后即将离开秘境，无需再提示挂机状态的变化
            if (choice != Choice::RETREAT) {
                sender << "已解除挂机状态，";
            }
        }
        sender << (choice == Choice::RETREAT ? "您选择了返回营地，将与其他撤离者平分路上遗留的金币"
                                             : "您选择了继续探险");
        return StageErrCode::READY;
    }

    // 本回合还需要为挂机玩家保留多少秒；返回 0 表示可以立即结算
    uint32_t GraceRemainSeconds_() const
    {
        if (grace_used_ || short_round_) {
            return 0;
        }
        const bool any_hooked = any_of(Main().board_.players.begin(), Main().board_.players.end(),
                [](const Player& player) { return player.IsExploring() && player.hook_status; });
        if (!any_hooked) {
            return 0;
        }
        const auto elapsed = chrono::duration_cast<chrono::seconds>(
                chrono::steady_clock::now() - begin_time_).count();
        if (elapsed >= static_cast<int64_t>(k_min_round_seconds)) {
            return 0;
        }
        return static_cast<uint32_t>(k_min_round_seconds - elapsed);
    }

    // 全员挂机的短回合中有玩家解除挂机，恢复正常时限，同时让仍在挂机的玩家不再阻塞本回合
    void RestoreNormalRound_(const PlayerID pid)
    {
        if (!short_round_) {
            return;
        }
        short_round_ = false;
        grace_used_ = false;
        for (PlayerID other = 0; other < Global().PlayerNum(); ++other) {
            const Player& player = Main().board_.players[other];
            if (player.IsExploring() && player.hook_status) {
                Global().SetReady(other);
            }
        }
        begin_time_ = chrono::steady_clock::now();
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << At(pid) << " 解除了自动探险，本回合等待时间恢复至 " << GAME_OPTION(时限) << " 秒";
    }

    // 本回合开始的时刻，用于计算最短等待时间
    chrono::steady_clock::time_point begin_time_;
    // 本回合开始时仍在探险的玩家是否全部挂机
    bool short_round_ = false;
    // 本回合是否已经延长过一次最短等待时间
    bool grace_used_ = false;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
