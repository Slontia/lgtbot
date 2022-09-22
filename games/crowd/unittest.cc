// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

// The first parameter is player number. It is a one-player game test.
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame()); // according to |GameOption::ToValid|, the mininum player number is 3
}

// The first parameter is player number. It is a five-player game test.
GAME_TEST(5, simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); // set game option here
    START_GAME();

    for (int pid = 0; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, std::to_string(pid * 100)); // we regard |READY| as |OK| here
    }
    // When the last player is ready, the stage will be over.
    ASSERT_PRI_MSG(CHECKOUT, 4, "400");

    // The game is over, now we check each player's score.
    ASSERT_SCORE(0, 100, 200, 300, 400);
}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
