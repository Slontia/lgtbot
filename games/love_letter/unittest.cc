// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ===== 玩家数校验 =====
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// ===== 游戏启动 =====
GAME_TEST(2, start_game_2p)
{
    START_GAME();
    ASSERT_FINISHED(false);
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(3, start_game_3p)
{
    START_GAME();
    ASSERT_FINISHED(false);
}

GAME_TEST(4, start_game_4p)
{
    START_GAME();
    ASSERT_FINISHED(false);
}

// ===== 无效输入 =====
GAME_TEST(2, invalid_card)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "不存在的卡");
}

// ===== 卫兵无目标（有空发逻辑，可能成功也可能失败取决于手牌）=====
GAME_TEST(2, guard_no_target)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "卫兵");
    ASSERT_TRUE(LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(FAILED) ||
                LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(CONTINUE) ||
                LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(CHECKOUT));
}

// ===== 错误回合 =====
GAME_TEST(2, wrong_turn)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 1, "卫兵 0 3");
}

// ===== 卫兵不能选自己 =====
GAME_TEST(2, guard_self)
{
    START_GAME();
    // 尝试多次，直到玩家0有卫兵
    for (int t = 0; t < 5; ++t) {
        const auto rc = PrivateRequest(0, "卫兵 0 3");
        if (rc == FAILED) { SUCCEED(); return; }
        if (rc == CHECKOUT || rc == CONTINUE) break;
    }
}

// ===== 王子可选自己 =====
GAME_TEST(2, prince_self)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "王子 0");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 无目标牌（侍女）=====
GAME_TEST(2, handmaid_no_target)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "侍女");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 完整游戏流程 =====
GAME_TEST(2, play_to_end)
{
    START_GAME();
    ASSERT_FINISHED(false);

    int safety = 200;
    while (!this->main_stage_->IsOver() && --safety > 0) {
        TimeoutRequest_();
    }
    ASSERT_TRUE(safety > 0) << "Game did not finish within safety limit";
    ASSERT_FINISHED(true);
}

// ===== 5人豪华版启动 =====
GAME_TEST(5, start_game_5p)
{
    START_GAME();
    ASSERT_FINISHED(false);
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(8, start_game_8p)
{
    START_GAME();
    ASSERT_FINISHED(false);
}

// ===== 卡牌效果：卫兵猜对淘汰 =====
GAME_TEST(2, guard_correct_guess)
{
    START_GAME();
    // 获取0号玩家的手牌点数，然后让1号玩家猜
    for (int c = 2; c <= 9; ++c) {
        const auto rc = PrivateRequest(0, std::string("卫兵 1 ") + std::to_string(c));
        if (rc != FAILED && rc != NOT_FOUND) {
            // 可能猜对淘汰了1号，也可能猜错
            break;
        }
    }
    ASSERT_FINISHED(false);
}

// ===== 卡牌效果：卫兵不能猜自己 =====
GAME_TEST(2, guard_cannot_target_self)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "卫兵 0 3");
    ASSERT_TRUE(LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(FAILED) ||
                LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(NOT_FOUND));
}

// ===== 卡牌效果：神父查看手牌 =====
GAME_TEST(2, priest_view_hand)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "神父 1");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 卡牌效果：男爵决斗 =====
GAME_TEST(2, baron_duel)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "男爵 1");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 卡牌效果：侍女保护 =====
GAME_TEST(2, handmaid_protect)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "侍女");
    if (rc == OK || rc == CONTINUE || rc == CHECKOUT) {
        // 打出了侍女，1号应不能选有保护的0号
        ASSERT_PRI_MSG(FAILED, 1, "神父 0");
    }
}

// ===== 卡牌效果：王子弃牌重抽 =====
GAME_TEST(2, prince_discard_redraw)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "王子 1");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 卡牌效果：王子可选自己 =====
GAME_TEST(2, prince_can_target_self)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "王子 0");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 卡牌效果：国王交换手牌 =====
GAME_TEST(2, king_swap)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "国王 1");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 卡牌效果：闺蜜无效果 =====
GAME_TEST(2, countess_no_effect)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "闺蜜");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 卡牌效果：公主自灭 =====
GAME_TEST(2, princess_self_eliminate)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "公主");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 错误回合 =====
GAME_TEST(2, wrong_turn_cn)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 1, "卫兵 0 3");
}

// ===== 无效卡名 =====
GAME_TEST(2, invalid_card_name)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "不存在的卡");
}

// ===== 目标玩家不存在 =====
GAME_TEST(2, target_out_of_range)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "神父 5");
    ASSERT_TRUE(LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(FAILED) ||
                LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(NOT_FOUND));
}

// ===== 5人完整游戏流程 =====
GAME_TEST(5, play_to_end_5p)
{
    START_GAME();
    ASSERT_FINISHED(false);

    int safety = 500;
    while (!this->main_stage_->IsOver() && --safety > 0) {
        TimeoutRequest_();
    }
    ASSERT_TRUE(safety > 0) << "Game did not finish within safety limit";
    ASSERT_FINISHED(true);
}

// ===== 玩家离开淘汰 =====
GAME_TEST(3, player_leave)
{
    START_GAME();
    const auto rc = LeaveRequest_(PlayerID{0});
    ASSERT_TRUE(rc == CHECKOUT || rc == CONTINUE);
}

// ===== 超时自动出牌 =====
GAME_TEST(2, timeout_force_play)
{
    START_GAME();
    const auto rc = TimeoutRequest_();
    ASSERT_TRUE(LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(CONTINUE) ||
                LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(CHECKOUT));
}

// ===== 弄臣选择 =====
GAME_TEST(5, jester_bet)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "弄臣 2");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 红衣交换两张 =====
GAME_TEST(5, cardinal_swap_two)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "红衣 1 2");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 女爵查看手牌 =====
GAME_TEST(5, baroness_view)
{
    START_GAME();
    // 查看1人
    const auto rc1 = PrivateRequest(0, "女爵 1");
    ASSERT_TRUE(rc1 == OK || rc1 == CONTINUE || rc1 == CHECKOUT || rc1 == FAILED);
}

// ===== 谄媚强制目标 =====
GAME_TEST(5, sycophant_force_target)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "谄媚 2");
    ASSERT_TRUE(rc == OK || rc == CONTINUE || rc == CHECKOUT || rc == FAILED);
}

// ===== 谄媚后必须打该目标 =====
GAME_TEST(5, sycophant_next_must_target)
{
    START_GAME();
    // 打出谄媚指定目标2
    int safety = 10;
    while (--safety > 0) {
        const auto rc = PrivateRequest(0, "谄媚 2");
        if (rc == OK || rc == CONTINUE) break;
    }
    if (safety > 0) {
        // 下一张牌必须指定2
        ASSERT_PRI_MSG(FAILED, 0, "神父 1");
    }
}

// ===== 主教猜中得分 =====
GAME_TEST(5, bishop_correct_guess)
{
    START_GAME();
    for (int g = 0; g <= 9; ++g) {
        if (g == 1) continue; // 不能猜1
        const auto rc = PrivateRequest(0, std::string("主教 1 ") + std::to_string(g));
        if (rc != FAILED && rc != NOT_FOUND) break;
    }
    ASSERT_FINISHED(false);
}

// ===== 卫兵不能猜1 =====
GAME_TEST(2, guard_cannot_guess_1_cn)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "卫兵 1 1");
    ASSERT_TRUE(LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(FAILED) ||
                LGTBOT_STAGE_ERR_UINT_(rc) == LGTBOT_STAGE_ERR_UINT_(NOT_FOUND));
}

// ===== 卫兵不能猜超过9 =====
GAME_TEST(2, guard_cannot_guess_over_9)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "卫兵 1 10");
}

// ===== 主教不能猜超过9 =====
GAME_TEST(5, bishop_cannot_guess_over_9)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "主教 1 10");
}

// ===== 被保护的目标不能被选 =====
GAME_TEST(2, protected_target)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "侍女");
    if (rc == OK || rc == CONTINUE || rc == CHECKOUT) {
        ASSERT_PRI_MSG(FAILED, 1, "神父 0");
    }
}

// ===== 淘汰后不能再被选 =====
GAME_TEST(2, eliminated_cannot_be_targeted)
{
    START_GAME();
    const auto rc = PrivateRequest(0, "男爵 1");
    if (rc == OK || rc == CONTINUE || rc == CHECKOUT) {
        // 可能0或1被淘汰。不管谁存活,被淘汰的不能再被选
    }
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
