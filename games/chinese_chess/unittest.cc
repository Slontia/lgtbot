// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(2, forbid_public_request)
{
    ASSERT_PUB_MSG(OK, 0, "阵营 1");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(FAILED, 0, "0 A0 B0");
}

GAME_TEST(2, ready_when_one_kingdom_each_player)
{
    ASSERT_PUB_MSG(OK, 0, "阵营 1");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "0 A0 B0");
    ASSERT_PRI_MSG(CONTINUE, 1, "1 A0 B0");
}

GAME_TEST(2, ready_when_two_kingdom_each_player)
{
    ASSERT_PUB_MSG(OK, 0, "阵营 2");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "0 A0 B0");
    ASSERT_PRI_MSG(OK, 0, "1 A0 B0");
    ASSERT_PRI_MSG(OK, 1, "2 A0 B0");
    ASSERT_PRI_MSG(CONTINUE, 1, "3 A0 B0");
}

GAME_TEST(2, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "阵营 1");
    ASSERT_PUB_MSG(OK, 0, "和平回合限制 1");
    ASSERT_PUB_MSG(OK, 0, "最小回合限制 10");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_TIMEOUT(CONTINUE);
    }
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(16, 16);
}

GAME_TEST(2, king_crash_game_over)
{
    ASSERT_PUB_MSG(OK, 0, "阵营 1");
    ASSERT_TRUE(StartGame());

    ASSERT_PRI_MSG(OK, 0, "0 A4 B4");
    ASSERT_PRI_MSG(CONTINUE, 1, "0 J4 I4");
    ASSERT_PRI_MSG(OK, 0, "蓝 pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "红 pass");

    ASSERT_PRI_MSG(OK, 0, "0 B4 B3");
    ASSERT_PRI_MSG(CONTINUE, 1, "0 I4 I3");
    ASSERT_PRI_MSG(OK, 0, "蓝 pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "红 pass");

    ASSERT_PRI_MSG(OK, 0, "0 B3 I3");
    ASSERT_PRI_MSG(CONTINUE, 1, "0 I3 I4");
    ASSERT_PRI_MSG(OK, 0, "蓝 pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "红 pass");

    ASSERT_PRI_MSG(OK, 0, "蓝 pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "0 I4 I5");
    ASSERT_PRI_MSG(OK, 0, "蓝 pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "红 pass");

    ASSERT_PRI_MSG(OK, 0, "0 I3 I4");
    ASSERT_PRI_MSG(CHECKOUT, 1, "0 I5 I4");

    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, cannot_double_move)
{
    ASSERT_PUB_MSG(OK, 0, "阵营 1");
    ASSERT_TRUE(StartGame());

    ASSERT_PRI_MSG(OK, 0, "0 A4 B4");
    ASSERT_PRI_MSG(FAILED, 0, "0 A4 B4");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
