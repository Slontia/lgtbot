// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
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

GAME_TEST(3, leave_test)
{
    START_GAME();

    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_LEAVE(CHECKOUT, 2);

    ASSERT_SCORE(0, 0, 0);
}

GAME_TEST(3, timeout_test)
{
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(0, 0, 0);
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
