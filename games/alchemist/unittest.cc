// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(1, invalid_option)
{
    ASSERT_PUB_MSG(OK, 0, "颜色 2");
    ASSERT_PUB_MSG(OK, 0, "点数 2");
    ASSERT_PUB_MSG(OK, 0, "副数 3");

    ASSERT_PUB_MSG(OK, 0, "回合数 13");
    ASSERT_FALSE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "回合数 12");
    ASSERT_TRUE(StartGame());
}

GAME_TEST(1, check_coor)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 5");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(FAILED, 0, "asf");
    ASSERT_PUB_MSG(FAILED, 0, "a");
    ASSERT_PUB_MSG(FAILED, 0, "f3");
    ASSERT_PUB_MSG(FAILED, 0, "A6");
    ASSERT_PUB_MSG(CHECKOUT, 0, "C4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "a4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "b5");
    ASSERT_PUB_MSG(CHECKOUT, 0, "B3");
}

GAME_TEST(2, one_finish)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_PUB_MSG(OK, 0, "pass");
        ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    }
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_TIMEOUT(CHECKOUT); // all hook
    }
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, one_do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 5");
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C4");
    ASSERT_TIMEOUT(CHECKOUT); // hook player_1
    ASSERT_PUB_MSG(CHECKOUT, 0, "D4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A3");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A2");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A1");
    for (uint32_t i = 6; i < 10; ++i) {
        ASSERT_TIMEOUT(CHECKOUT); // all hook
    }
    ASSERT_SCORE(2, 0);
}

GAME_TEST(2, restore_hook)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 5");
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C4");
    ASSERT_TIMEOUT(CHECKOUT); // hook player_1
    ASSERT_PUB_MSG(OK, 1, "A4"); // restore hook player_1
    ASSERT_PUB_MSG(CHECKOUT, 0, "D4");
}

// y5 p4 p1 g2 b1 y3 b1 g1 p4 o5
GAME_TEST(2, DISABLED_achieve_score) // we cannot set 胜利分数 option
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 5");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C4");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(CHECKOUT, 0, "D4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A3");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A2");
    ASSERT_PUB_MSG(CHECKOUT, 0, "A1");
    ASSERT_SCORE(2, 0);
}

GAME_TEST(2, repeat_selection)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 5");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C4");
    ASSERT_PUB_MSG(FAILED, 0, "B3");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
