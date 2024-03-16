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

    ASSERT_SCORE(5, 12, 10, 9);
}

GAME_TEST(5, simultaneously_guard_takehp_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "夺 4 4");
    ASSERT_PRI_MSG(OK, 1, "守 1 3");
    ASSERT_PRI_MSG(OK, 2, "守 1 5");
    ASSERT_PRI_MSG(OK, 3, "夺 1 4");
    ASSERT_PRI_MSG(CHECKOUT, 4, "抢 4 5");

    ASSERT_SCORE(-1, 7, 9, 3, 14);
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

    ASSERT_PRI_MSG(FAILED, 0, "夺 2 4");

    ASSERT_PRI_MSG(OK, 0, "捡 2");
    ASSERT_PRI_MSG(OK, 1, "撤离");
    ASSERT_PRI_MSG(OK, 2, "抢 2 5");
    ASSERT_PRI_MSG(CHECKOUT, 3, "捡 2");

    ASSERT_SCORE(8, 10, 5, 12);
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
