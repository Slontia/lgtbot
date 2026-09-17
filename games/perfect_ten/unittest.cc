// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "card.h"
#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(4, player_too_many)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, task_require_options)
{
    // 单任务区间反向
    ASSERT_PUB_MSG(OK, 0, "任务张数下限 4");
    ASSERT_PUB_MSG(OK, 0, "任务张数上限 3");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, task_require_sum_options)
{
    // 总张数区间反向
    ASSERT_PUB_MSG(OK, 0, "任务总张数下限 8");
    ASSERT_PUB_MSG(OK, 0, "任务总张数上限 6");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, task_require_no_intersection)
{
    // 单任务 2~3 张时两任务之和只可能是 4~6，与总张数区间 8~10 无交集
    ASSERT_PUB_MSG(OK, 0, "任务张数下限 2");
    ASSERT_PUB_MSG(OK, 0, "任务张数上限 3");
    ASSERT_PUB_MSG(OK, 0, "任务总张数下限 8");
    ASSERT_PUB_MSG(OK, 0, "任务总张数上限 10");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, task_require_valid_options)
{
    ASSERT_PUB_MSG(OK, 0, "任务张数下限 3");
    ASSERT_PUB_MSG(OK, 0, "任务张数上限 5");
    ASSERT_PUB_MSG(OK, 0, "任务总张数下限 6");
    ASSERT_PUB_MSG(OK, 0, "任务总张数上限 9");
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(2, parse_cards)
{
    ASSERT_TRUE(ParseCards("红2红4红10").has_value());
    ASSERT_TRUE(ParseCards("灰12").has_value());
    ASSERT_EQ(4u, ParseCards("红1蓝2黄3灰4")->size());
    ASSERT_FALSE(ParseCards("红13").has_value());   // 数字超出范围
    ASSERT_FALSE(ParseCards("红0").has_value());    // 数字不能为 0
    ASSERT_FALSE(ParseCards("红01").has_value());   // 不接受前导零
    ASSERT_FALSE(ParseCards("紫3").has_value());    // 颜色不存在
    ASSERT_FALSE(ParseCards("红").has_value());     // 缺少数字
    ASSERT_FALSE(ParseCards("3").has_value());      // 缺少颜色
    ASSERT_FALSE(ParseCards("").has_value());
}

GAME_TEST(2, match_rule)
{
    const std::vector<Card> same_point{{Color::RED, 5}, {Color::BLUE, 5}, {Color::GREY, 5}};
    const std::vector<Card> consecutive{{Color::RED, 6}, {Color::YELLOW, 7}, {Color::GREY, 8}};
    const std::vector<Card> duplicated{{Color::RED, 5}, {Color::BLUE, 5}, {Color::RED, 6}, {Color::RED, 7}};
    const std::vector<Card> same_color{{Color::RED, 1}, {Color::RED, 8}, {Color::RED, 12}};
    const std::vector<Card> odd{{Color::RED, 1}, {Color::BLUE, 7}, {Color::GREY, 11}};

    ASSERT_TRUE(MatchRule(TaskType::SAME_POINT, same_point));
    ASSERT_FALSE(MatchRule(TaskType::SAME_POINT, consecutive));
    ASSERT_TRUE(MatchRule(TaskType::CONSECUTIVE, consecutive));
    ASSERT_FALSE(MatchRule(TaskType::CONSECUTIVE, duplicated));  // 连续任务不允许数字重复
    ASSERT_TRUE(MatchRule(TaskType::SAME_COLOR, same_color));
    ASSERT_FALSE(MatchRule(TaskType::SAME_COLOR, same_point));
    ASSERT_TRUE(MatchRule(TaskType::ODD, odd));
    ASSERT_FALSE(MatchRule(TaskType::EVEN, odd));

    ASSERT_TRUE(IsQualified(Task{TaskType::SAME_POINT, 3}, same_point));
    ASSERT_FALSE(IsQualified(Task{TaskType::SAME_POINT, 4}, same_point));    // 张数不足
}

GAME_TEST(2, sort_hand)
{
    Player player;
    player.hand_ = {{Color::RED, 5}, {Color::GREY, 3}, {Color::BLUE, 2}, {Color::YELLOW, 3},
                    {Color::GREY, 2}, {Color::RED, 1}};
    player.SortHand();
    ASSERT_EQ("红1蓝2灰2黄3灰3红5", CardsName(player.hand_));
}

GAME_TEST(2, take_from_hand)
{
    Player player;
    player.hand_ = {{Color::RED, 5}, {Color::RED, 5}, {Color::BLUE, 2}};
    ASSERT_FALSE(player.TakeFromHand({{Color::RED, 5}, {Color::RED, 5}, {Color::RED, 5}}));
    ASSERT_EQ(3u, player.hand_.size());     // 手牌不足时不修改手牌
    ASSERT_TRUE(player.TakeFromHand({{Color::RED, 5}, {Color::RED, 5}}));
    ASSERT_EQ("蓝2", CardsName(player.hand_));
}

GAME_TEST(2, status_command)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 1, "赛况");
}

GAME_TEST(2, unknown_command)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "红13");     // 非法牌名不会被任何指令匹配
    ASSERT_PRI_MSG(NOT_FOUND, 0, "紫3");
    ASSERT_PRI_MSG(NOT_FOUND, 0, "随便聊两句");
    ASSERT_PRI_MSG(NOT_FOUND, 0, "4-1 红5");  // 玩家编号超出校验器范围
}

GAME_TEST(2, only_current_player_can_act)
{
    START_GAME();
    // 只有当前行动玩家可以抽牌，另一位玩家的相同指令必定失败
    const bool first_acted = CHECK_PRI_MSG(OK, 0, "P");
    if (first_acted) {
        ASSERT_PRI_MSG(FAILED, 1, "P");
    } else {
        ASSERT_PRI_MSG(FAILED, 0, "P");
        ASSERT_PRI_MSG(OK, 1, "P");
    }
}

GAME_TEST(2, draw_and_discard_are_not_skippable)
{
    START_GAME();
    // 抽牌阶段无法跳过
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 0, "跳过"));
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 1, "跳过"));
    ASSERT_TRUE(CHECK_PRI_MSG(OK, 0, "P") || CHECK_PRI_MSG(OK, 1, "P"));
    // 抽牌后进入任务阶段，可以跳过
    ASSERT_TRUE(CHECK_PRI_MSG(OK, 0, "跳过") || CHECK_PRI_MSG(OK, 1, "跳过"));
    // 未完成基础任务的玩家会直接进入弃牌阶段，此时无法再跳过
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 0, "跳过"));
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 1, "跳过"));
}

GAME_TEST(2, invalid_task_group)
{
    START_GAME();
    ASSERT_TRUE(CHECK_PRI_MSG(OK, 0, "P") || CHECK_PRI_MSG(OK, 1, "P"));
    // 每组仅一张牌，必定不满足任务的最少张数要求
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 0, "红1 蓝2"));
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 1, "红1 蓝2"));
}

GAME_TEST(2, chain_before_task_done)
{
    START_GAME();
    ASSERT_TRUE(CHECK_PRI_MSG(OK, 0, "P") || CHECK_PRI_MSG(OK, 1, "P"));
    // 双方均未完成基础任务，连出必定失败
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 0, "1-1 红5"));
    ASSERT_FALSE(CHECK_PRI_MSG(OK, 1, "1-1 红5"));
}

GAME_TEST(3, play_multiple_turns)
{
    ASSERT_PUB_MSG(OK, 0, "种子 perfect_ten_unittest");
    START_GAME();
    for (int i = 0; i < 15; ++i) {
        bool acted = false;
        for (uint64_t pid = 0; pid < 3 && !acted; ++pid) {
            if (!CHECK_PRI_MSG(OK, pid, "P")) {
                continue;
            }
            acted = true;
            CHECK_PRI_MSG(OK, pid, "跳过");    // 任务阶段
            CHECK_PRI_MSG(OK, pid, "跳过");    // 连出阶段，若已进入弃牌阶段则忽略
            bool discarded = false;
            for (int color = 0; color < k_color_num && !discarded; ++color) {
                for (int point = 1; point <= k_max_point && !discarded; ++point) {
                    discarded = CHECK_PRI_MSG(OK, pid, std::string(k_color_names[color]) + std::to_string(point));
                }
            }
            ASSERT_TRUE(discarded);
        }
        ASSERT_TRUE(acted);
    }
    ASSERT_FINISHED(false);
    ASSERT_SCORE(0, 0, 0);
}

GAME_TEST(3, game_continues_until_round_ends)
{
    ASSERT_PUB_MSG(OK, 0, "种子 perfect_ten_unittest");
    START_GAME();
    // 电脑推演至整场游戏结束，胜者的分数必定在本局结算时才达到目标分数
    for (int i = 0; i < 3000; ++i) {
        bool over = false;
        for (uint64_t pid = 0; pid < 3; ++pid) {
            this->ComputerActRequest_(pid);
            if (this->main_stage_->IsOver()) {
                over = true;
                break;
            }
        }
        if (over) {
            break;
        }
    }
    ASSERT_FINISHED(true);
    // 结束时必定有玩家达到目标分数，且所有玩家手牌均已在局末清空
    int64_t best = 0;
    for (uint64_t pid = 0; pid < 3; ++pid) {
        best = std::max(best, this->main_stage_->PlayerScore(PlayerID{static_cast<uint32_t>(pid)}));
    }
    ASSERT_GE(best, k_target_score);
}

GAME_TEST(2, timeout_eliminates_and_last_player_wins)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);   // 两人局中一人超时被淘汰，剩余玩家直接获胜
    ASSERT_FINISHED(true);
}

GAME_TEST(3, timeout_continues_with_three_players)
{
    START_GAME();
    ASSERT_TIMEOUT(CONTINUE);   // 三人局淘汰一人后游戏继续
    ASSERT_FINISHED(false);
    ASSERT_TIMEOUT(CHECKOUT);   // 再淘汰一人后仅剩一人，游戏结束
    ASSERT_FINISHED(true);
}

GAME_TEST(2, leave_test)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 0);
    ASSERT_FINISHED(true);
}

GAME_TEST(3, leave_continues_with_three_players)
{
    START_GAME();
    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_FINISHED(false);
    // 退出玩家不再参与行动，剩余玩家继续游戏
    ASSERT_TRUE(CHECK_PRI_MSG(OK, 1, "P") || CHECK_PRI_MSG(OK, 2, "P"));
    ASSERT_LEAVE(CHECKOUT, 1);
    ASSERT_FINISHED(true);
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
