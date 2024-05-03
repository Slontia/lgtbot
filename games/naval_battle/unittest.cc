// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// The first parameter is player number. It is a one-player game test.
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame()); // according to |GameOption::ToValid|, the mininum player number is 2
}

// The first parameter is player number. It is a five-player game test.
GAME_TEST(2, leave_test)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 0);

    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, timeout_test)
{
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-1, -1);
}

GAME_TEST(2, simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "侦察 0");
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "h11 下");
    for (int i = 0; i < 2; i++) {
        ASSERT_PRI_MSG(OK, i, "c1 上");
        ASSERT_PRI_MSG(OK, i, "C5 上");
        ASSERT_PRI_MSG(OK, i, "h1 上");
    }
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "确认");

    ASSERT_PRI_MSG(OK, 0, "c2");
    ASSERT_PRI_MSG(FAILED, 0, "c5");
    ASSERT_PRI_MSG(OK, 1, "h5");
    ASSERT_PRI_MSG(OK, 1, "h6");
    ASSERT_PRI_MSG(CHECKOUT, 1, "h7");

    ASSERT_PRI_MSG(OK, 0, "c1");
    ASSERT_PRI_MSG(OK, 0, "c5");
    ASSERT_PRI_MSG(OK, 0, "h1");
    ASSERT_PRI_MSG(OK, 1, "c1");
    ASSERT_PRI_MSG(OK, 1, "c5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "h1");

    ASSERT_SCORE(1, 1);
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
