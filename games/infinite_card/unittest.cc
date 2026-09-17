// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "card.h"
#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 正常分出胜负时胜者 1、败者 0，平局双方均为 0；只有中途退出或超时淘汰才会出现 -1
#define ASSERT_NORMAL_END_SCORES()                                                  \
    do {                                                                           \
        const int64_t first = this->main_stage_->PlayerScore(PlayerID{0});          \
        const int64_t second = this->main_stage_->PlayerScore(PlayerID{1});         \
        ASSERT_TRUE((first == 1 && second == 0) || (first == 0 && second == 1) ||   \
                    (first == 0 && second == 0))                                    \
                << "unexpected scores: " << first << ", " << second;                \
    } while (0)

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(3, player_too_many)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, valid_options)
{
    ASSERT_PUB_MSG(OK, 0, "金币 50");
    ASSERT_PUB_MSG(OK, 0, "回合上限 10");
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(2, parse_card)
{
    ASSERT_EQ(1, ParseCard("1")->point_);
    ASSERT_EQ(10, ParseCard("10")->point_);
    ASSERT_TRUE(ParseCard("∞")->IsInfinite());
    ASSERT_TRUE(ParseCard("无")->IsInfinite());
    ASSERT_TRUE(ParseCard("无限")->IsInfinite());
    ASSERT_TRUE(ParseCard("无限牌")->IsInfinite());
    ASSERT_TRUE(ParseCard("inf")->IsInfinite());
    ASSERT_TRUE(ParseCard("INF")->IsInfinite());
    ASSERT_TRUE(ParseCard("♾️")->IsInfinite());     // 带变体选择符的 emoji
    ASSERT_TRUE(ParseCard("♾")->IsInfinite());      // 不带变体选择符的 emoji
    ASSERT_FALSE(ParseCard("0").has_value());       // 0 为无限牌的内部表示，不接受直接输入
    ASSERT_FALSE(ParseCard("11").has_value());      // 超出数字范围
    ASSERT_FALSE(ParseCard("01").has_value());      // 不接受前导零
    ASSERT_FALSE(ParseCard("x").has_value());
    ASSERT_FALSE(ParseCard("").has_value());
}

GAME_TEST(2, compare_card)
{
    const Card small{k_min_point};
    const Card large{k_max_point};
    const Card infinite{k_infinite_point};

    ASSERT_TRUE(CompareCard(large, small) == DuelResult::FIRST_WIN);
    ASSERT_TRUE(CompareCard(small, large) == DuelResult::SECOND_WIN);
    ASSERT_TRUE(CompareCard(small, Card{k_min_point}) == DuelResult::DRAW);
    ASSERT_TRUE(CompareCard(infinite, large) == DuelResult::FIRST_WIN);      // 无限牌胜过任意数字牌
    ASSERT_TRUE(CompareCard(large, infinite) == DuelResult::SECOND_WIN);
    ASSERT_TRUE(CompareCard(infinite, Card{k_infinite_point}) == DuelResult::DRAW);
}

GAME_TEST(2, base_and_bonus_schedule)
{
    // 基础投入：第 1~6 回合逐回合 +1，第 7~12 回合固定 8，第 13 回合起固定 10
    const int expected_base[] = { 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 10, 10, 10, 10, 10 };
    for (int round = 1; round <= static_cast<int>(std::size(expected_base)); ++round) {
        ASSERT_EQ(expected_base[round - 1], BaseOfRound(round)) << "round " << round;
    }
    ASSERT_EQ(k_base_high, BaseOfRound(100));       // 之后不再变化

    // 未使用无限牌的奖励与基础投入同档位切换
    ASSERT_EQ(k_bonus_low, BonusOfRound(1));
    ASSERT_EQ(k_bonus_low, BonusOfRound(6));
    ASSERT_EQ(k_bonus_flat, BonusOfRound(7));       // 基础投入升至 8 的同一回合
    ASSERT_EQ(k_bonus_flat, BonusOfRound(12));
    ASSERT_EQ(k_bonus_high, BonusOfRound(13));      // 基础投入升至 10 的同一回合
    ASSERT_EQ(k_bonus_high, BonusOfRound(100));
}

GAME_TEST(2, card_order)
{
    std::vector<Card> cards = { Card{k_infinite_point}, Card{7}, Card{1}, Card{10} };
    std::sort(cards.begin(), cards.end());
    ASSERT_EQ("1 7 10 ∞", CardsName(cards));    // 数字升序，无限牌排在最后
}

GAME_TEST(2, make_deal)
{
    std::mt19937 rng = MakeRng("infinite_card_deal");
    for (int i = 0; i < 200; ++i) {
        const DealResult deal = MakeDeal(rng);
        int sum[k_player_num] = { 0, 0 };
        for (int pid = 0; pid < k_player_num; ++pid) {
            for (int group = 0; group < k_group_num; ++group) {
                const std::vector<Card>& cards = deal[pid][group];
                ASSERT_EQ(static_cast<size_t>(k_group_size), cards.size());
                int infinite_num = 0;
                for (const Card& card : cards) {
                    if (card.IsInfinite()) {
                        ++infinite_num;
                        continue;
                    }
                    ASSERT_GE(card.point_, k_min_point);
                    ASSERT_LE(card.point_, k_max_point);
                    sum[pid] += card.point_;
                }
                ASSERT_EQ(1, infinite_num);         // 每组恰好附带一张无限牌
            }
        }
        ASSERT_EQ(sum[0], sum[1]);                  // 双方十二张数字牌之和相同
    }
}

GAME_TEST(2, deal_groups_are_different)
{
    const auto numbers = [](const std::vector<Card>& cards)
        {
            std::vector<int> points;
            for (const Card& card : cards) {
                if (!card.IsInfinite()) {
                    points.emplace_back(card.point_);
                }
            }
            std::sort(points.begin(), points.end());
            return points;
        };
    std::mt19937 rng = MakeRng("infinite_card_deal");
    for (int i = 0; i < 200; ++i) {
        const DealResult deal = MakeDeal(rng);
        // 任一方的任一组都不与对手的任一组完全相同
        for (int first = 0; first < k_group_num; ++first) {
            for (int second = 0; second < k_group_num; ++second) {
                ASSERT_NE(numbers(deal[0][first]), numbers(deal[1][second]));
            }
        }
    }
}

GAME_TEST(2, select_group_and_take)
{
    Player player;
    player.groups_[0] = { Card{3}, Card{5}, Card{7}, Card{9}, Card{k_infinite_point} };
    player.SelectGroup(0);
    ASSERT_EQ(static_cast<size_t>(k_group_size), player.hand_.size());
    ASSERT_TRUE(player.groups_[0].empty());     // 选定的组随即从持有的牌组中移出
    ASSERT_FALSE(player.TakeFromHand({ Card{4} }));
    ASSERT_EQ(static_cast<size_t>(k_group_size), player.hand_.size());   // 取牌失败时不修改手牌

    const std::vector<Card> order = { Card{9}, Card{3}, Card{7}, Card{5} };
    ASSERT_TRUE(player.TakeFromHand(order));
    player.PushOrder(order);
    ASSERT_EQ("9 3 7 5", CardsName(player.order_));
    ASSERT_FALSE(player.used_infinite_);
    ASSERT_EQ("∞", CardsName(player.hand_));    // 未被选中的一张将在回合结束时消失
    ASSERT_EQ(9, player.DuelCard(0)->point_);
    ASSERT_EQ(5, player.DuelCard(k_duel_num - 1)->point_);
    ASSERT_FALSE(player.DuelCard(k_duel_num).has_value());
}

GAME_TEST(2, push_infinite_marks_used)
{
    Player player;
    player.groups_[0] = { Card{1}, Card{2}, Card{3}, Card{4}, Card{k_infinite_point} };
    player.SelectGroup(0);
    const std::vector<Card> order = { Card{1}, Card{k_infinite_point}, Card{3}, Card{4} };
    ASSERT_TRUE(player.TakeFromHand(order));
    player.PushOrder(order);
    ASSERT_TRUE(player.used_infinite_);
    player.ResetRound();
    ASSERT_FALSE(player.used_infinite_);
    ASSERT_TRUE(player.order_.empty());
}

GAME_TEST(2, only_group_and_need_deal)
{
    Player player;
    player.groups_[0] = { Card{1} };
    player.groups_[2] = { Card{2} };
    ASSERT_EQ(2, player.RemainGroupNum());
    ASSERT_EQ(-1, player.OnlyGroup());          // 有两组可选，不会自动选定
    player.SelectGroup(0);
    ASSERT_EQ(2, player.OnlyGroup());           // 仅剩第 3 组，将自动选定
    player.SelectGroup(2);
    ASSERT_EQ(-1, player.OnlyGroup());
    ASSERT_TRUE(player.NeedDeal());             // 三组用尽后需要重新发牌
}

GAME_TEST(2, status_command)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 1, "赛况");
}

GAME_TEST(2, private_status_shows_own_order)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_unittest");
    START_GAME();
    this->ComputerActRequest_(0);
    this->ComputerActRequest_(1);       // 选组阶段
    this->ComputerActRequest_(0);
    this->ComputerActRequest_(1);       // 排序阶段
    // 私信赛况附带本人的出牌顺序，公屏赛况则不含任何私密信息
    ASSERT_PRI_MSG(OK, 0, "赛况");
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(2, unknown_command)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "4");            // 牌组编号超出校验器范围
    ASSERT_PRI_MSG(NOT_FOUND, 0, "0");
    ASSERT_PRI_MSG(NOT_FOUND, 0, "1 2");          // 选组只接受单个编号
    ASSERT_PRI_MSG(NOT_FOUND, 0, "随便聊两句");
}

GAME_TEST(2, select_group_is_private_and_once)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "1");          // 选组必须私信
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(FAILED, 0, "2");          // 本阶段已完成行动
}

GAME_TEST(2, order_command_needs_exactly_four_cards)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_unittest");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");        // 双方选定，进入排序阶段
    ASSERT_PRI_MSG(NOT_FOUND, 0, "1 2 3");        // 张数不足
    ASSERT_PRI_MSG(NOT_FOUND, 0, "1 2 3 4 5");    // 张数过多
    ASSERT_PRI_MSG(NOT_FOUND, 0, "1 2 3 11");     // 含非法牌名
    ASSERT_PUB_MSG(FAILED, 0, "1 2 3 4");         // 排序必须私信
}

GAME_TEST(2, only_current_player_can_act_in_duel)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_unittest");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");        // 双方选定，进入排序阶段
    this->ComputerActRequest_(0);
    this->ComputerActRequest_(1);                 // 排序由电脑代为完成，进入第 1 小局
    // 只有当前行动方可以行动，行动之后行动权立即交给对手
    if (CHECK_PRI_MSG(OK, 0, "观望")) {
        ASSERT_PRI_MSG(FAILED, 0, "观望");
    } else {
        ASSERT_PRI_MSG(OK, 1, "观望");
        ASSERT_PRI_MSG(FAILED, 1, "观望");
    }
}

GAME_TEST(2, raise_validation)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_unittest");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    this->ComputerActRequest_(0);
    this->ComputerActRequest_(1);
    // 超过自己剩余金币的加码必定失败，非行动方的加码同样失败
    ASSERT_PRI_MSG(FAILED, 0, "加码 99999");
    ASSERT_PRI_MSG(FAILED, 1, "加码 99999");
    const uint64_t actor = CHECK_PRI_MSG(OK, 0, "加码 4") ? 0 : 1;
    if (actor == 1) {
        ASSERT_PRI_MSG(OK, 1, "加码 4");
    }
    const uint64_t other = 1 - actor;
    ASSERT_PRI_MSG(FAILED, other, "观望");        // 对手投入更高时无法观望
    ASSERT_PRI_MSG(FAILED, other, "加码 4");      // 未超过差额，不构成追加
    ASSERT_PRI_MSG(CHECKOUT, other, "跟进");      // 跟进后立即揭示，本小局结束
    ASSERT_FINISHED(false);
}

GAME_TEST(2, fold_ends_duel)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_unittest");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    this->ComputerActRequest_(0);
    this->ComputerActRequest_(1);
    if (!CHECK_PRI_MSG(CHECKOUT, 0, "放弃")) {
        ASSERT_PRI_MSG(CHECKOUT, 1, "放弃");
    }
    ASSERT_FINISHED(false);                       // 放弃仅结束本小局，游戏继续
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(2, timeout_eliminates_both)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);                     // 选组阶段双方均超时，两人俱被淘汰
    ASSERT_FINISHED(true);
    ASSERT_SCORE(-1, -1);                         // 均视为中途退出
}

GAME_TEST(2, timeout_eliminates_one)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_TIMEOUT(CHECKOUT);                     // 1 号超时被淘汰，游戏结束
    ASSERT_FINISHED(true);
    ASSERT_ELIMINATED(1);
    ASSERT_SCORE(0, -1);                          // 淘汰方 -1，对手不因此得胜者分
}

GAME_TEST(2, leave_test)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 0);                    // 两人局中一方退出即结束
    ASSERT_FINISHED(true);
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, computer_plays_until_game_over)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_deduction");
    ASSERT_PUB_MSG(OK, 0, "回合上限 8");
    START_GAME();
    for (int i = 0; i < 20000 && !this->main_stage_->IsOver(); ++i) {
        for (uint64_t pid = 0; pid < 2 && !this->main_stage_->IsOver(); ++pid) {
            this->ComputerActRequest_(pid);
        }
    }
    ASSERT_FINISHED(true);
    ASSERT_NORMAL_END_SCORES();
}

GAME_TEST(2, computer_plays_through_all_base_tiers)
{
    // 金币充足时不会提前分出胜负，可推进到第 13 回合以后，覆盖基础投入的全部三个档位
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_tiers");
    ASSERT_PUB_MSG(OK, 0, "金币 500");
    ASSERT_PUB_MSG(OK, 0, "回合上限 20");
    START_GAME();
    for (int i = 0; i < 40000 && !this->main_stage_->IsOver(); ++i) {
        for (uint64_t pid = 0; pid < 2 && !this->main_stage_->IsOver(); ++pid) {
            this->ComputerActRequest_(pid);
        }
    }
    ASSERT_FINISHED(true);
    ASSERT_NORMAL_END_SCORES();
}

GAME_TEST(2, computer_plays_single_card_mode)
{
    ASSERT_PUB_MSG(OK, 0, "种子 infinite_card_single");
    ASSERT_PUB_MSG(OK, 0, "逐张 开启");
    ASSERT_PUB_MSG(OK, 0, "回合上限 8");
    START_GAME();
    for (int i = 0; i < 20000 && !this->main_stage_->IsOver(); ++i) {
        for (uint64_t pid = 0; pid < 2 && !this->main_stage_->IsOver(); ++pid) {
            this->ComputerActRequest_(pid);
        }
    }
    ASSERT_FINISHED(true);
    ASSERT_NORMAL_END_SCORES();
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
