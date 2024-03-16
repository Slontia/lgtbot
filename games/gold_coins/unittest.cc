// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(3, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(4, pickcoins_simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "捡 1");
    ASSERT_PRI_MSG(OK, 1, "捡 2");
    ASSERT_PRI_MSG(OK, 2, "捡 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "捡 5");

    // 0: 10+1 -1 = 10
    // 1: 10+2 -1 = 11
    // 2: 10 -1 = 9
    // 3: 10 -1 = 9
    ASSERT_SCORE(10, 11, 9, 9);
}

GAME_TEST(4, snatch_recursion_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "抢 2 5");
    ASSERT_PRI_MSG(OK, 1, "抢 3 4");
    ASSERT_PRI_MSG(OK, 2, "抢 4 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "捡 4");

    // 0: 10+5 -1 = 14
    // 1: 10+4-5 -1 = 8
    // 2: 10+5-4 -1 = 10
    // 3: 10+4-5 -1 = 9
    ASSERT_SCORE(14, 8, 10, 8);
}

GAME_TEST(4, snatch_loop_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "抢 2 5");
    ASSERT_PRI_MSG(OK, 1, "抢 1 4");
    ASSERT_PRI_MSG(OK, 2, "抢 4 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 1 4");

    // 0: 10-5+4+4 -1 = 12
    // 1: 10-4+5 -1 = 10
    // 2: 10-5 -1 = 4
    // 3: 10-4+5 -1 = 10
    ASSERT_SCORE(12, 10, 4, 10);
}

GAME_TEST(4, snatch_takehp_Eliminate_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "捡 5");
    ASSERT_PRI_MSG(OK, 1, "夺 1 5");
    ASSERT_PRI_MSG(OK, 2, "夺 1 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 2 4");

    // 0: 10+5 [15] → 0 -1 = -1                     [hp 10-5-5 = 0]
    // 1: 10-5+(15*5*15%)[11.25≈11]-4 -1 = 11       [hp 10+5 = 15]
    // 2: 10-5+(15*5*15%)[11.25≈11] -1 = 15         [hp 10+5 = 15]
    // 3: 10+4 -1 = 13
    ASSERT_SCORE(-1, 11, 15, 13);
}

GAME_TEST(4, simultaneously_guard_pickcoins_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "捡 4");
    ASSERT_PRI_MSG(OK, 1, "守 1 3");
    ASSERT_PRI_MSG(OK, 2, "守 1 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 1 4");

    // 0: 10+0+3+5+4 -1 = 21
    // 1: 10-3+4 -1 = 10
    // 2: 10-5+4 -1 = 8
    // 3: 10-4 -1 = 5
    ASSERT_SCORE(21, 10, 8, 5);
}

GAME_TEST(4, simultaneously_guard_snatch_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "抢 4 4");
    ASSERT_PRI_MSG(OK, 1, "守 1 3");
    ASSERT_PRI_MSG(OK, 2, "守 1 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 3 4");

    // 0: 10+4-3-5 -1 = 5
    // 1: 10+3 -1 = 12
    // 2: 10+5-4 -1 = 10
    // 3: 10+4-4 -1 = 9
    ASSERT_SCORE(5, 12, 10, 9);
}

GAME_TEST(6, simultaneously_guard_takehp_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "夺 4 4");
    ASSERT_PRI_MSG(OK, 1, "守 1 3");
    ASSERT_PRI_MSG(OK, 2, "守 1 1");
    ASSERT_PRI_MSG(OK, 3, "夺 1 4");
    ASSERT_PRI_MSG(OK, 4, "抢 4 5");
    ASSERT_PRI_MSG(CHECKOUT, 5, "抢 3 3");

    // 0: 10-4 [6] → 0 -1 = -1                      [hp 10 → 0]
    // 1: 10-3+(6*(3-2)*30%)[1.8≈1] -1 = 7
    // 2: 10-1+(6*(1-2)*30%)[-1.8≈-1]+3 -1 = 10
    // 3: 10-4+(6*4*15%)[3.6≈3]+5 -1 = 13           [hp 10+4 = 14]
    // 4: 10-5 -1 = 4
    // 5: 10-3 -1 = 6
    ASSERT_SCORE(-1, 7, 10, 13, 4, 6);
}

GAME_TEST(7, guard_withstand_takehp_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "夺 2 4");
    ASSERT_PRI_MSG(OK, 1, "守 2 3");
    ASSERT_PRI_MSG(OK, 2, "夺 4 2");
    ASSERT_PRI_MSG(OK, 3, "守 4 4");
    ASSERT_PRI_MSG(OK, 4, "捡 0");
    ASSERT_PRI_MSG(OK, 5, "夺 5 4");
    ASSERT_PRI_MSG(CHECKOUT, 6, "守 5 1");

    // 0: 10-4 -1 = 5
    // 1: 10-3+3*2 -1 = 12      [hp 10-4+3 = 9]
    // 2: 10-2 -1 = 7
    // 3: 10-4+2*2 -1 = 9       [hp 10-2+(4)[2] = 10]
    // 4: 10+0 -1 = 9           [hp 10-4+1 = 7]
    // 5: 10-4 -1 = 5
    // 6: 10-1 -1 = 8
    ASSERT_SCORE(5, 12, 7, 9, 9, 5, 8);
}

GAME_TEST(4, takehp_leave_returnhalfcoins_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "夺 2 4");
    ASSERT_PRI_MSG(OK, 1, "捡 2");
    ASSERT_PRI_MSG(OK, 2, "夺 2 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "捡 2");
    // 0: 10-4 = 6          [hp 10+4 = 14]
    // 1: 10+2 = 12         [hp 10-4-5 = 1]
    // 2: 10-5 = 5          [hp 10+5 = 15]
    // 3: 10+2 = 12

    ASSERT_PRI_MSG(FAILED, 0, "夺 2 4");

    ASSERT_PRI_MSG(OK, 0, "捡 2");
    ASSERT_PRI_MSG(OK, 1, "撤离");
    ASSERT_PRI_MSG(OK, 2, "抢 2 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "捡 2");

    // 0: 6+2 +(4/2)[2] -2 = 8
    // 1: 12 -2 = 10
    // 2: 5 +(5/2)[2.5≈2] -2 = 5
    // 3: 12+2 -2 = 12
    ASSERT_SCORE(8, 10, 5, 12);
}

GAME_TEST(4, snatch_explodecoins_pickcoins_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "抢 2 5");
    ASSERT_PRI_MSG(OK, 1, "捡 3");
    ASSERT_PRI_MSG(OK, 2, "夺 2 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "夺 2 5");

    // 0: 10+5 -1 = 14
    // 1: 10+3-5 [8] → 0 -1 = -1        [hp 10-5-5 = 0]
    // 2: 10-5+(8*5*15%)[6] -1 = 10     [hp 10+5 = 15]
    // 3: 10-5+(8*5*15%)[6] -1 = 10     [hp 10+5 = 15]
    ASSERT_SCORE(14, -1, 10, 10);
}

GAME_TEST(4, explodecoins_snatch_takehp_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "撤离");
    ASSERT_PRI_MSG(OK, 1, "捡 3");
    ASSERT_PRI_MSG(OK, 2, "夺 2 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "抢 1 5");
    // 0: 10 = 10
    // 1: 10+3 = 13         [hp 10-5 = 5]
    // 2: 10-5 = 5          [hp 10+5 = 15]
    // 3: 10+0 = 10

    ASSERT_PRI_MSG(OK, 1, "抢 4 3");
    ASSERT_PRI_MSG(OK, 2, "捡 2");
    ASSERT_PRI_MSG(CHECKOUT, 3, "夺 2 5");

    // 0: 10 -1 = 9
    // 1: 13+3 [16] → 0 -2 = -2               [hp 5-5 = 0]
    // 2: 5+2+(16*5*15%)[12] -2 = 17
    // 3: 10-5+(16*5*15%)[12]-3 -2 = 12       [hp 10+5 = 15]
    ASSERT_SCORE(9, -2, 17, 12);
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
