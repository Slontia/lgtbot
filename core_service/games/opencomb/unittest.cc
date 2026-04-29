// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, one_player)
{
    ASSERT_PUB_MSG(OK, 0, "种子 123");  // start with erase
    ASSERT_PUB_MSG(OK, 0, "地图 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(CHECKOUT, 0, "22");  // erase
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 0, "2");
    ASSERT_PUB_MSG(CHECKOUT, 0, "3");
    ASSERT_PUB_MSG(CHECKOUT, 0, "4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "5");
    ASSERT_PUB_MSG(CHECKOUT, 0, "6");
    ASSERT_PUB_MSG(CHECKOUT, 0, "24 10"); // move
    ASSERT_PUB_MSG(CHECKOUT, 0, "7");
    ASSERT_PUB_MSG(CHECKOUT, 0, "8");
    ASSERT_PUB_MSG(CHECKOUT, 0, "9");
    ASSERT_PUB_MSG(CHECKOUT, 0, "10");  // reshape
    ASSERT_PUB_MSG(CHECKOUT, 0, "10");  // wall
    ASSERT_PUB_MSG(CHECKOUT, 0, "11");
    ASSERT_PUB_MSG(CHECKOUT, 0, "12");
    ASSERT_PUB_MSG(CHECKOUT, 0, "13");
    ASSERT_PUB_MSG(CHECKOUT, 0, "14");
    ASSERT_PUB_MSG(CHECKOUT, 0, "15");
    ASSERT_PUB_MSG(CHECKOUT, 0, "16");
    ASSERT_PUB_MSG(CHECKOUT, 0, "17");
    ASSERT_PUB_MSG(CHECKOUT, 0, "18");
    ASSERT_PUB_MSG(CHECKOUT, 0, "19");
    ASSERT_PUB_MSG(CHECKOUT, 0, "20");
    ASSERT_PUB_MSG(CHECKOUT, 0, "22");
    ASSERT_PUB_MSG(CHECKOUT, 0, "24");
    ASSERT_SCORE(92);
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
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
    ASSERT_PUB_MSG(FAILED, 0, "10");
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_PUB_MSG(FAILED, 0, "1");
    ASSERT_PUB_MSG(FAILED, 0, "21");
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(FAILED, 1, "2 7");     // SelectStage
    ASSERT_PUB_MSG(CONTINUE, 0, "3 7");
    ASSERT_PUB_MSG(CHECKOUT, 1, "2 7");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(OK, 0, "8");
        ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    }
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(OK, 0, "8");
    ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    ASSERT_SCORE(0, 23);
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
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "2 1");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    ASSERT_SCORE(0, 23);
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
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 0, "2 1");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_SCORE(0, 23);
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_LEAVE(CONTINUE, 1);      // SelectStage
    ASSERT_PUB_MSG(CHECKOUT, 0, "1 7");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_SCORE(0, 23);
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_LEAVE(CONTINUE, 0);      // SelectStage
    ASSERT_PUB_MSG(FAILED, 1, "3 7");
    ASSERT_PUB_MSG(CHECKOUT, 1, "1 7");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    ASSERT_SCORE(0, 23);
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(CONTINUE, 0, "3 7");     // SelectStage
    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_PUB_MSG(CHECKOUT, 1, "2 7");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    }
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "8");
    ASSERT_SCORE(0, 23);
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(OK, 1, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 2, "pass");
    ASSERT_LEAVE(CONTINUE, 1);          // SelectStage
    ASSERT_PUB_MSG(CONTINUE, 0, "4 7");
    ASSERT_PUB_MSG(CHECKOUT, 2, "2 7");
    ASSERT_PUB_MSG(OK, 0, "8");
    ASSERT_PUB_MSG(CHECKOUT, 2, "8");
    ASSERT_LEAVE(CONTINUE, 2);
    for (int i = 0; i < 3; i++) {
        ASSERT_PUB_MSG(CHECKOUT, 0, "9");
    }
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 0, "9");
    for (int i = 0; i < 21; i++) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(0, 89, 74);
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
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_TIMEOUT(CONTINUE);      // SelectStage
    ASSERT_PUB_MSG(FAILED, 1, "3 7");
    ASSERT_PUB_MSG(CHECKOUT, 1, "1 7");
    for (int i = 0; i < 4; i++) {
        ASSERT_PUB_MSG(OK, 0, "1");
        ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    }
    ASSERT_PUB_MSG(OK, 0, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1, "1");
    ASSERT_SCORE(0, 23);
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
