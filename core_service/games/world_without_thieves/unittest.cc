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

// The first parameter is player number. It is a five-player game test.
GAME_TEST(5, simple_test)
{
    START_GAME();

    for (int pid = 0; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "贼 " + std::to_string(pid + 2)); // we regard |READY| as |OK| here
    }
    ASSERT_PRI_MSG(CHECKOUT, 4, "贼 1");

    // The game is over, now we check each player's score.
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
