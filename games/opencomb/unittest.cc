// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, one_player)
{
    ASSERT_PUB_MSG(OK, 0, "种子 123");  // start with reshape
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // reshape
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 0, "2");
    ASSERT_PUB_MSG(CHECKOUT, 0, "3");
    ASSERT_PUB_MSG(CHECKOUT, 0, "4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "5");
    ASSERT_PUB_MSG(CHECKOUT, 0, "6");
    ASSERT_PUB_MSG(CHECKOUT, 0, "7");
    ASSERT_PUB_MSG(CHECKOUT, 0, "8");
    ASSERT_PUB_MSG(CHECKOUT, 0, "9");
    ASSERT_PUB_MSG(CHECKOUT, 0, "10");
    ASSERT_PUB_MSG(CHECKOUT, 0, "11");
    ASSERT_PUB_MSG(CHECKOUT, 0, "12");
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // wall_broken
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // erase
    ASSERT_PUB_MSG(CHECKOUT, 0, "13");
    ASSERT_PUB_MSG(CHECKOUT, 0, "14");
    ASSERT_PUB_MSG(CHECKOUT, 0, "15");
    ASSERT_PUB_MSG(CHECKOUT, 0, "16");
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // erase
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // move
    ASSERT_PUB_MSG(CHECKOUT, 0, "17");
    ASSERT_PUB_MSG(CHECKOUT, 0, "18");
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // wall
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // reshape
    ASSERT_PUB_MSG(CHECKOUT, 0, "19");
    ASSERT_PUB_MSG(CHECKOUT, 0, "20");
    ASSERT_SCORE(51);
    ASSERT_ACHIEVEMENTS(0);
}

GAME_TEST(2, two_player)
{
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    ASSERT_PUB_MSG(OK, 0, "2");
    ASSERT_PUB_MSG(CHECKOUT, 1, "2");
    ASSERT_PUB_MSG(OK, 0, "3");
    ASSERT_PUB_MSG(CHECKOUT, 1, "3");
    ASSERT_PUB_MSG(OK, 0, "4");
    ASSERT_PUB_MSG(CHECKOUT, 1, "4");
    ASSERT_PUB_MSG(OK, 0, "5");
    ASSERT_PUB_MSG(CHECKOUT, 1, "5");
    ASSERT_PUB_MSG(OK, 0, "6");
    ASSERT_PUB_MSG(CHECKOUT, 1, "6");
    ASSERT_PUB_MSG(OK, 0, "7");
    ASSERT_PUB_MSG(CHECKOUT, 1, "7");
    ASSERT_PUB_MSG(OK, 0, "8");
    ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    ASSERT_PUB_MSG(OK, 0, "9");
    ASSERT_PUB_MSG(CHECKOUT, 1, "9");
}

GAME_TEST(1, failed_selection)
{
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // reshape: skip
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");    // slot 1
    ASSERT_PUB_MSG(FAILED, 0, "1");      // slot already filled
    ASSERT_PUB_MSG(FAILED, 0, "21");     // slot 21 is a wall (in classic map)
}

GAME_TEST(2, timeout_eliminate)
{
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_TIMEOUT(CHECKOUT);       // pass special
    ASSERT_PUB_MSG(OK, 1, "1");
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_PUB_MSG(OK, 0, "2");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(FAILED, 1, "2"); // is seq filled
    ASSERT_PUB_MSG(OK, 1, "3");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(FAILED, 0, "3"); // is seq filled
    ASSERT_PUB_MSG(OK, 0, "4");
}

GAME_TEST(2, timeout_hook)
{
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");   // player 1 seq fill 1
    ASSERT_PUB_MSG(CHECKOUT, 0, "2");   // player 1 is hooked, seq fill 2
    ASSERT_PUB_MSG(FAILED, 1, "1");     // is seq filled
    ASSERT_PUB_MSG(FAILED, 1, "2");     // is seq filled
    ASSERT_PUB_MSG(OK, 1, "3");
}

GAME_TEST(2, yunding_two_player)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "2");
        ASSERT_PUB_MSG(CHECKOUT, 1, "2");
    }
    ASSERT_PUB_MSG(OK, 0, "7");         // round=7 normal card (was erase in old RNG)
    ASSERT_PUB_MSG(CHECKOUT, 1, "7");
    ASSERT_PUB_MSG(FAILED, 1, "2 7");   // SelectStage
    ASSERT_PUB_MSG(CONTINUE, 0, "3 7");
    ASSERT_PUB_MSG(CHECKOUT, 1, "2 7");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "8");
        ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    }
    ASSERT_PUB_MSG(OK, 0, "pass"); // wall_broken
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    // 2nd SelectStage (round_=15): timeout auto-selects
    ASSERT_TIMEOUT(CONTINUE); // player 0 auto-selects
    ASSERT_TIMEOUT(CHECKOUT); // player 1 auto-selects
    ASSERT_TIMEOUT(CHECKOUT); // round 16: erase auto-skip
    ASSERT_TIMEOUT(CHECKOUT); // round 17: normal
    ASSERT_TIMEOUT(CHECKOUT); // round 18: normal
    ASSERT_TIMEOUT(CHECKOUT); // round 19: normal → player 0 eliminated
    ASSERT_SCORE(0, 46);
    ASSERT_ACHIEVEMENTS(0);
}

GAME_TEST(2, yunding_timeout_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "2");
    ASSERT_PUB_MSG(CHECKOUT, 1, "2");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(CHECKOUT, 0, "3");
    ASSERT_PUB_MSG(FAILED, 1, "22");
}

GAME_TEST(2, yunding_leave_test1)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "1");   // round=7 normal card
    // 1st SelectStage: player 0 auto-handled in HandleStageBegin (PERMANENTLY_INACTIVE)
    ASSERT_PUB_MSG(CHECKOUT, 1, "2 1"); // player 1 selects
    for (int i = 0; i < 5; i++) {      // rounds 9-13
        ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass"); // round 14: wall_broken
    // 2nd SelectStage: player 0 auto-handled in HandleStageBegin
    ASSERT_TIMEOUT(CHECKOUT);            // player 1 auto-selects
    ASSERT_TIMEOUT(CHECKOUT);            // round 16: erase auto-skip
    ASSERT_TIMEOUT(CHECKOUT);            // round 17
    ASSERT_TIMEOUT(CHECKOUT);            // round 18
    ASSERT_TIMEOUT(CHECKOUT);            // round 19
    ASSERT_TIMEOUT(CHECKOUT);            // round 20
    ASSERT_TIMEOUT(CHECKOUT);            // round 21: erase auto-skip
    // 3rd SelectStage: player 0 auto-handled in HandleStageBegin
    ASSERT_TIMEOUT(CHECKOUT);            // player 1 auto-selects
    ASSERT_TIMEOUT(CHECKOUT);            // round 23: move auto-skip
    ASSERT_TIMEOUT(CHECKOUT);            // round 24
    ASSERT_TIMEOUT(CHECKOUT);            // round 25
    ASSERT_TIMEOUT(CHECKOUT);            // round 26: wall, player 1 eliminated → game over
    ASSERT_SCORE(48, 11);
}

GAME_TEST(2, yunding_leave_test2)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");   // round=7 normal card
    ASSERT_PUB_MSG(CHECKOUT, 0, "2 1"); // 1st SelectStage: player 0 selects (player 1 leave is SetReady)
    for (int i = 0; i < 5; i++) {      // rounds 9-13
        ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // round 14: wall_broken → player 0 hp ≤ 0, game over
    ASSERT_SCORE(0, 34);
}

GAME_TEST(2, yunding_SelectStage_leave_test1)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "2");
        ASSERT_PUB_MSG(CHECKOUT, 1, "2");
    }
    ASSERT_PUB_MSG(OK, 0, "7");         // round=7 normal card
    ASSERT_PUB_MSG(CHECKOUT, 1, "7");
    ASSERT_LEAVE(CONTINUE, 1);          // 1st SelectStage: player 1 leaves, player 0 selects alone
    ASSERT_PUB_MSG(CHECKOUT, 0, "1 7");
    for (int i = 0; i < 5; i++) {      // rounds 9-13
        ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass"); // round 14: wall_broken → player 1 hp ≤ 0, game over
    ASSERT_SCORE(0, 11);
}

GAME_TEST(2, yunding_SelectStage_leave_test2)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "1");
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(OK, 0, "7");         // round=7 normal card
    ASSERT_PUB_MSG(CHECKOUT, 1, "7");
    ASSERT_LEAVE(CONTINUE, 0);          // 1st SelectStage: player 0 leaves, player 1 alone
    ASSERT_PUB_MSG(FAILED, 1, "3 7");   // player 1 is now first (player 0 left during SelectStage)
    ASSERT_PUB_MSG(CHECKOUT, 1, "1 7");
    for (int i = 0; i < 5; i++) {      // rounds 9-13
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass"); // round 14: wall_broken → player 0 hp ≤ 0, game over
    ASSERT_SCORE(0, 11);
}

GAME_TEST(2, yunding_SelectStage_leave_test3)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "1");
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(OK, 0, "7");         // round=7 normal card
    ASSERT_PUB_MSG(CHECKOUT, 1, "7");
    ASSERT_PUB_MSG(CONTINUE, 0, "3 7"); // 1st SelectStage: player 0 selects first
    ASSERT_LEAVE(CONTINUE, 0);          // player 0 leaves during SelectStage
    ASSERT_PUB_MSG(CHECKOUT, 1, "2 7"); // player 1 selects
    for (int i = 0; i < 5; i++) {      // rounds 9-13
        ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass"); // round 14: wall_broken → player 0 hp ≤ 0, game over
    ASSERT_SCORE(23, 11);
}

GAME_TEST(3, yunding_SelectStage_leave_test4)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(OK, 1, "21");
    ASSERT_PUB_MSG(CHECKOUT, 2, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(OK, 1, "21");
    ASSERT_PUB_MSG(CHECKOUT, 2, "21");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "1");
        ASSERT_PUB_MSG(OK, 1, "1");
        ASSERT_PUB_MSG(CHECKOUT, 2, "1");
    }
    ASSERT_PUB_MSG(OK, 0, "7");         // round=7 normal card
    ASSERT_PUB_MSG(OK, 1, "7");
    ASSERT_PUB_MSG(CHECKOUT, 2, "7");
    ASSERT_LEAVE(CONTINUE, 1);          // 1st SelectStage: player 1 leaves
    ASSERT_PUB_MSG(CONTINUE, 0, "4 7");
    ASSERT_PUB_MSG(CHECKOUT, 2, "2 7");
    ASSERT_PUB_MSG(OK, 0, "8");
    ASSERT_PUB_MSG(CHECKOUT, 2, "8");
    ASSERT_LEAVE(CONTINUE, 2);          // player 2 leaves during round 9 → only player 0 left, game over
    ASSERT_SCORE(0, 11, 11);
}

GAME_TEST(2, yunding_SelectStage_tiemout_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 云顶");
    ASSERT_PUB_MSG(OK, 0, "种子 123");
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "21");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "21");
    for (int i = 0; i < 5; i++) {
        ASSERT_PUB_MSG(OK, 0, "1");
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(OK, 0, "7");         // round=7 normal card
    ASSERT_PUB_MSG(CHECKOUT, 1, "7");
    ASSERT_TIMEOUT(CONTINUE);           // 1st SelectStage: timeout → player 0 auto-selects
    ASSERT_PUB_MSG(FAILED, 1, "3 7");   // player 1 tries wrong order
    ASSERT_PUB_MSG(CHECKOUT, 1, "1 7"); // player 1 selects
    for (int i = 0; i < 5; i++) {      // rounds 9-13
        ASSERT_PUB_MSG(OK, 0, "1");
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(OK, 0, "pass");      // round 14: wall_broken
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_TIMEOUT(CONTINUE);           // 2nd SelectStage: player 0 auto-selects
    ASSERT_TIMEOUT(CHECKOUT);           // player 1 auto-selects
    ASSERT_TIMEOUT(CHECKOUT);           // round 16: erase auto-skip
    ASSERT_TIMEOUT(CHECKOUT);           // round 17
    ASSERT_TIMEOUT(CHECKOUT);           // round 18
    ASSERT_TIMEOUT(CHECKOUT);           // round 19 → player 0 eliminated, game over
    ASSERT_SCORE(0, 11);
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
