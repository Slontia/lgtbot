// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

// The first parameter is player number. It is a one-player game test.
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame()); // according to |GameOption::ToValid|, the mininum player number is 3
}

GAME_TEST(5, first_round_min_apple_win)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "金");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_PRI_MSG(CHECKOUT, 4, "银");

    ASSERT_SCORE(-1, -1, -1, -1, 4);
}

GAME_TEST(5, first_round_gold_win)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_PRI_MSG(CHECKOUT, 4, "银");

    ASSERT_SCORE(-1, -1, -1, 4, -1);
}

GAME_TEST(5, red_win)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_PRI_MSG(CONTINUE, 4, "银");
    // score change: -1 -1 -1 4 -1

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "红");
    ASSERT_PRI_MSG(CHECKOUT, 4, "银");
    // score change: 1 1 1 1 -4

    ASSERT_SCORE(0, 0, 0, 5, -5);
}

GAME_TEST(5, gold_win)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "红");
    ASSERT_PRI_MSG(CONTINUE, 4, "红");

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_PRI_MSG(CHECKOUT, 4, "银");

    ASSERT_SCORE(-1, -1, -1, 4, -1);
}

GAME_TEST(5, silver_win)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "红");
    ASSERT_PRI_MSG(CONTINUE, 4, "红");

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "金");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_PRI_MSG(CHECKOUT, 4, "银");

    ASSERT_SCORE(-1, -1, -1, -1, 4);
}

GAME_TEST(5, reverse_score)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_PRI_MSG(CONTINUE, 4, "银");

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "红");
    ASSERT_PRI_MSG(OK, 3, "红");
    ASSERT_PRI_MSG(CHECKOUT, 4, "红");

    ASSERT_SCORE(1, 1, 1, -4, 1);
}

GAME_TEST(5, default_red)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "红");
    ASSERT_PRI_MSG(OK, 1, "红");
    ASSERT_PRI_MSG(OK, 2, "银");
    ASSERT_PRI_MSG(OK, 3, "金");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-1, -1, -1, 4, -1);
}

GAME_TEST(5, limit_gold)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 3");
    ASSERT_PUB_MSG(OK, 0, "金苹果 2");
    START_GAME();

    for (int i = 0; i < 2; ++i) {
        ASSERT_PRI_MSG(OK, 0, "红");
        ASSERT_PRI_MSG(OK, 1, "红");
        ASSERT_PRI_MSG(OK, 2, "红");
        ASSERT_PRI_MSG(OK, 3, "红");
        ASSERT_PRI_MSG(CONTINUE, 4, "金");
    }

    ASSERT_PRI_MSG(FAILED, 4, "金");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
