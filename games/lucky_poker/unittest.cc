// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, not_allow_direct_prepare)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "准备");
}

GAME_TEST(2, not_allow_direct_prepare_second_bet)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "准备");
}

GAME_TEST(2, raise_exceed_score)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "A 41");
}

GAME_TEST(3, keep_fold)
{
    START_GAME();
    for (uint32_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(OK, 0, "A 弃牌");
        ASSERT_PRI_MSG(OK, 0, "B 弃牌");
        ASSERT_PRI_MSG(OK, 0, "C 弃牌");
        ASSERT_PRI_MSG(OK, 0, "准备");
        ASSERT_PRI_MSG(OK, 1, "D 弃牌");
        ASSERT_PRI_MSG(OK, 1, "E 弃牌");
        ASSERT_PRI_MSG(OK, 1, "F 弃牌");
        ASSERT_PRI_MSG(OK, 1, "准备");
        ASSERT_PRI_MSG(OK, 2, "G 弃牌");
        ASSERT_PRI_MSG(OK, 2, "H 弃牌");
        ASSERT_PRI_MSG(OK, 2, "I 弃牌");
        ASSERT_PRI_MSG(CHECKOUT, 2, "准备");
        ASSERT_PRI_MSG(OK, 0, "准备");
        ASSERT_PRI_MSG(OK, 1, "准备");
        ASSERT_PRI_MSG(CHECKOUT, 2, "准备");
    }
    ASSERT_SCORE(15 * 3 * 6, 15 * 3 * 6, 15 * 3 * 6);
}

GAME_TEST(2, second_bet_exceed_first_bet)
{
    ASSERT_PRI_MSG(OK, 0, "种子 ABC");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "A 40");
    ASSERT_PRI_MSG(OK, 0, "准备");
    ASSERT_PRI_MSG(OK, 1, "E 40");
    ASSERT_PRI_MSG(CHECKOUT, 1, "准备");
    ASSERT_PRI_MSG(FAILED, 0, "B 1 红1");
    ASSERT_PRI_MSG(OK, 0, "A 1 蓝5");
}

GAME_TEST(2, second_bet_raise_folded)
{
    ASSERT_PRI_MSG(OK, 0, "种子 ABC");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "A 弃牌");
    ASSERT_PRI_MSG(OK, 0, "B 弃牌");
    ASSERT_PRI_MSG(OK, 0, "C 弃牌");
    ASSERT_PRI_MSG(OK, 0, "D 弃牌");
    ASSERT_PRI_MSG(OK, 0, "准备");
    ASSERT_PRI_MSG(OK, 1, "E 弃牌");
    ASSERT_PRI_MSG(OK, 1, "F 弃牌");
    ASSERT_PRI_MSG(OK, 1, "G 弃牌");
    ASSERT_PRI_MSG(OK, 1, "H 弃牌");
    ASSERT_PRI_MSG(CHECKOUT, 1, "准备");
    ASSERT_PRI_MSG(FAILED, 0, "A 1 蓝5");
}

GAME_TEST(2, fold_exceed_remain_scores)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "A 40");
    ASSERT_PRI_MSG(FAILED, 0, "B 弃牌");
    ASSERT_PRI_MSG(OK, 0, "A 34");
    ASSERT_PRI_MSG(FAILED, 0, "B 弃牌");
    ASSERT_PRI_MSG(OK, 0, "A 30");
    ASSERT_PRI_MSG(OK, 0, "B 弃牌");
}

GAME_TEST(2, not_bet_skip_second_bet)
{
    ASSERT_PRI_MSG(OK, 0, "种子 ABC");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "A 40");
    ASSERT_PRI_MSG(OK, 0, "准备");
    ASSERT_PRI_MSG(OK, 1, "E 40");
    ASSERT_PRI_MSG(CHECKOUT, 1, "准备");
    ASSERT_PRI_MSG(OK, 0, "A 20 蓝5");
    ASSERT_PRI_MSG(OK, 0, "准备");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
