// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// The first parameter is player number. It is a one-player game test.
GAME_TEST(1, player_not_enough) {
  ASSERT_FALSE(StartGame());  // according to |GameOption::ToValid|, the mininum player number is 3
}

GAME_TEST(3, alive0_test1)
{
    START_GAME();

    for (int i = 0; i < 5; i++) {
      ASSERT_PRI_MSG(OK, 0, "2 5");
      ASSERT_PRI_MSG(OK, 1, "1 5");
      ASSERT_PRI_MSG(CHECKOUT, 2, "2 5");
    }

    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(OK, 1, "1");
    ASSERT_PRI_MSG(CHECKOUT, 2, "1");

    ASSERT_PRI_MSG(OK, 0, "3 5");
    ASSERT_PRI_MSG(OK, 1, "3");
    ASSERT_PRI_MSG(CHECKOUT, 2, "4");

    ASSERT_SCORE(6, 6, 11);
}

GAME_TEST(3, alive0_test2)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "2 5");
    ASSERT_PRI_MSG(OK, 1, "5");
    ASSERT_PRI_MSG(CHECKOUT, 2, "5");

    ASSERT_PRI_MSG(OK, 0, "3 4");
    ASSERT_PRI_MSG(CHECKOUT, 2, "5");

    for (int i = 0; i < 5; i++) {
      ASSERT_PRI_MSG(OK, 0, "3 5");
      ASSERT_PRI_MSG(CHECKOUT, 2, "1 5");
    }

    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 2, "1");

    ASSERT_PRI_MSG(OK, 0, "3");
    ASSERT_PRI_MSG(CHECKOUT, 2, "4");

    ASSERT_SCORE(8, 0, 13);
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

// The first parameter is player number. It is a five-player game test.
// GAME_TEST(5, simple_test)
// {
//     START_GAME();

//     for (int pid = 0; pid < 4; ++pid) {
//         ASSERT_PRI_MSG(OK, pid, "贼 " + std::to_string(pid + 2)); // we regard |READY| as |OK|
//         here
//     }
//     ASSERT_PRI_MSG(CHECKOUT, 4, "贼 1");

//     // The game is over, now we check each player's score.
// }

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
