// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, leave_test1)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 0);

    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, leave_test2)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(0, -1);
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
