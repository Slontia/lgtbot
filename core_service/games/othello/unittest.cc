// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// The first parameter is player number. It is a one-player game test.
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame()); // according to |GameOption::ToValid|, the mininum player number is 3
}

GAME_TEST(2, leave_game)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 0);
    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, one_timeout)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "C4");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, both_timeout)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(2, 2); // the current score
}

GAME_TEST(2, cannot_repeat_set)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 1, "C4");
    ASSERT_PRI_MSG(FAILED, 1, "C4");
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
