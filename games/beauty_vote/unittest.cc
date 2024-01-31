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

    ASSERT_PRI_MSG(OK, 0, "16"); // we regard |READY| as |OK| here
    ASSERT_PRI_MSG(OK, 1, "22");
    ASSERT_PRI_MSG(OK, 2, "24");
    ASSERT_PRI_MSG(OK, 3, "21");
    // When the last player is ready, the stage will be over.
    ASSERT_PRI_MSG(CHECKOUT, 4, "28");

    ASSERT_PRI_MSG(FAILED, 0, "101");

    ASSERT_PRI_MSG(OK, 0, "10");
    ASSERT_PRI_MSG(OK, 1, "23");
    ASSERT_PRI_MSG(OK, 2, "13");
    ASSERT_PRI_MSG(OK, 3, "12");
    ASSERT_PRI_MSG(CHECKOUT, 4, "8");

    ASSERT_PRI_MSG(OK, 0, "7");
    ASSERT_PRI_MSG(OK, 1, "21");
    ASSERT_PRI_MSG(OK, 2, "7");
    ASSERT_PRI_MSG(OK, 3, "7");
    ASSERT_PRI_MSG(CHECKOUT, 4, "10");

    ASSERT_PRI_MSG(OK, 0, "14");
    ASSERT_PRI_MSG(OK, 1, "24");
    ASSERT_PRI_MSG(OK, 2, "4");
    ASSERT_PRI_MSG(OK, 3, "3");
    ASSERT_PRI_MSG(CHECKOUT, 4, "10");

    ASSERT_PRI_MSG(OK, 0, "11");
    ASSERT_PRI_MSG(OK, 1, "27");
    ASSERT_PRI_MSG(OK, 2, "11");
    ASSERT_PRI_MSG(OK, 3, "8");
    ASSERT_PRI_MSG(CHECKOUT, 4, "100");

    ASSERT_PRI_MSG(OK, 0, "14");
    ASSERT_PRI_MSG(OK, 1, "28");
    ASSERT_PRI_MSG(OK, 2, "13");
    ASSERT_PRI_MSG(OK, 3, "8");
    ASSERT_PRI_MSG(CHECKOUT, 4, "18");

    ASSERT_PRI_MSG(OK, 0, "9");
    ASSERT_PRI_MSG(OK, 1, "29");
    ASSERT_PRI_MSG(OK, 2, "9");
    ASSERT_PRI_MSG(OK, 3, "6");
    ASSERT_PRI_MSG(CHECKOUT, 4, "6");

    ASSERT_PRI_MSG(OK, 0, "3");
    ASSERT_PRI_MSG(OK, 1, "21");
    ASSERT_PRI_MSG(OK, 2, "13");
    ASSERT_PRI_MSG(OK, 3, "8");
    ASSERT_PRI_MSG(CHECKOUT, 4, "7");

    ASSERT_PRI_MSG(OK, 0, "8");
    ASSERT_PRI_MSG(OK, 1, "11");
    ASSERT_PRI_MSG(OK, 2, "6");
    ASSERT_PRI_MSG(CHECKOUT, 3, "5");

    // The game is over, now we check each player's score.
    ASSERT_SCORE(11, 10, 17, 11, 8);
}

GAME_TEST(8, red_test)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    ASSERT_PUB_MSG(OK, 0, "红心 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "21");
    ASSERT_PRI_MSG(OK, 1, "89");
    ASSERT_PRI_MSG(OK, 2, "22");
    ASSERT_PRI_MSG(OK, 3, "11");
    ASSERT_PRI_MSG(OK, 4, "15");
    ASSERT_PRI_MSG(OK, 5, "23");
    ASSERT_PRI_MSG(OK, 6, "23");
    ASSERT_PRI_MSG(CHECKOUT, 7, "11");

    ASSERT_SCORE(2, 1, 3, 1, 1, 1, 1, 1);
}

GAME_TEST(3, zero_test1)
{
    ASSERT_PUB_MSG(OK, 0, "撞车 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "100");
    ASSERT_PRI_MSG(OK, 1, "0");
    ASSERT_PRI_MSG(CHECKOUT, 2, "0");

    ASSERT_SCORE(5, 0, 0);
}

GAME_TEST(3, zero_test2)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    ASSERT_PUB_MSG(OK, 0, "撞车 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "100");
    ASSERT_PRI_MSG(OK, 1, "100");
    ASSERT_PRI_MSG(CHECKOUT, 2, "0");

    ASSERT_SCORE(1, 1, 2);
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
