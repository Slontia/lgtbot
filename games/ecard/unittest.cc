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

// [人生模式]
GAME_TEST(2, LiveMode_simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "500");
    ASSERT_PRI_MSG(CHECKOUT, 1, "700");
    // 0号防守 1号进攻

    ASSERT_PUB_MSG(FAILED, 1, "31");
    ASSERT_PUB_MSG(FAILED, 1, "45");
    ASSERT_PUB_MSG(CHECKOUT, 1, "20");
    ASSERT_PRI_MSG(FAILED, 0, "皇帝");
    ASSERT_PRI_MSG(FAILED, 1, "奴隶");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CONTINUE, 1, "市民");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");

    ASSERT_PRI_MSG(CHECKOUT, 1, "25");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CONTINUE, 1, "市民");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "25");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(FAILED, 0, "奴隶");
    ASSERT_PRI_MSG(FAILED, 1, "皇帝");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CONTINUE, 1, "市民");
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PUB_MSG(CHECKOUT, 1, "20");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");

    ASSERT_PRI_MSG(CHECKOUT, 1, "15");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "15");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(CHECKOUT, 1, "38");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CONTINUE, 1, "市民");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CONTINUE, 1, "市民");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CONTINUE, 1, "市民");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");
    
    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, LiveMode_TargetStageTimeout_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "500");
    ASSERT_TIMEOUT(CHECKOUT);
    
    ASSERT_SCORE(0, -1);
}

GAME_TEST(2, LiveMode_BidStageTimeout_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "500");
    ASSERT_PRI_MSG(CHECKOUT, 1, "600");
    // 0号防守 1号进攻

    ASSERT_TIMEOUT(CHECKOUT);
    
    ASSERT_SCORE(0, -1);
}

GAME_TEST(2, LiveMode_GameStageLeave_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "500");
    ASSERT_PRI_MSG(CHECKOUT, 1, "600");
    // 0号防守 1号进攻

    ASSERT_PUB_MSG(CHECKOUT, 1, "20");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_LEAVE(CHECKOUT, 0);
    
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, LiveMode_GameStageTimeout_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "500");
    ASSERT_PRI_MSG(CHECKOUT, 1, "600");
    // 0号防守 1号进攻

    ASSERT_PUB_MSG(CHECKOUT, 1, "20");

    ASSERT_TIMEOUT(CHECKOUT);
    
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, LiveMode_LastRoundliveFail_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "500");
    ASSERT_PRI_MSG(CHECKOUT, 1, "700");
    // 0号防守 1号进攻

    ASSERT_PUB_MSG(CHECKOUT, 1, "20");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");

    ASSERT_PRI_MSG(CHECKOUT, 1, "25");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "25");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PUB_MSG(CHECKOUT, 1, "20");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");

    ASSERT_PRI_MSG(CHECKOUT, 1, "15");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "15");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(CHECKOUT, 1, "38");
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");
    
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, LiveMode_QuickWin_EnoughHealth_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "100");
    ASSERT_PRI_MSG(CHECKOUT, 1, "150");
    // 0号防守 1号进攻

    ASSERT_PUB_MSG(CHECKOUT, 1, "30");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 150 - 30
    
    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, LiveMode_QuickWin_InsufficientHealth_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 人生");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "100");
    ASSERT_PRI_MSG(CHECKOUT, 1, "150");
    // 0号防守 1号进攻

    ASSERT_PUB_MSG(CHECKOUT, 1, "25");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 125 - 30

    ASSERT_PUB_MSG(CHECKOUT, 1, "25");
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 125 - 5

    ASSERT_PUB_MSG(CHECKOUT, 1, "5");
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 150 - 5

    ASSERT_TIMEOUT(CHECKOUT);
    
    ASSERT_SCORE(0, -1);
}

// [经典模式]
GAME_TEST(2, DefaultMode_StartFailed_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 经典");
    ASSERT_PUB_MSG(OK, 0, "回合数 11");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, DefaultMode_simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 经典");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 0-1

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");
    
    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, DefaultMode_GameStageTimeout_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 经典");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_TIMEOUT(CHECKOUT);
    
    ASSERT_SCORE(0, -1);
}

GAME_TEST(2, DefaultMode_QuickWin0_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 经典");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 1-0

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 1-1

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 2-1
    
    ASSERT_SCORE(2, 1);
}

GAME_TEST(2, DefaultMode_QuickWin1_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 经典");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 0-1

    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 0-2
    
    ASSERT_SCORE(0, 2);
}

// [点球模式]
GAME_TEST(2, ShootoutMode_StartFailed_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 点球");
    ASSERT_PUB_MSG(OK, 0, "回合数 11");
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, ShootoutMode_simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 点球");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-0
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-1

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 1-1
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-2

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 2-2
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 2-3

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-3
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-4

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 4-4
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 4-5
    
    ASSERT_SCORE(4, 5);
}

GAME_TEST(2, ShootoutMode_OddQuickWin_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 点球");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-0
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-1

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 2-1
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 2-1

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-1
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 3-1

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 4-1
    
    ASSERT_SCORE(4, 1);
}

GAME_TEST(2, ShootoutMode_EvenQuickWin_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 点球");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-0
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-1

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 1-1
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-2

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 1-2
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-3

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 2-3
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 2-4
    
    ASSERT_SCORE(2, 4);
}

GAME_TEST(2, ShootoutMode_extra1_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 点球");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-0
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-1
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 1-1
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-2
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 2-2
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 2-3
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-3
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-4
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 4-4
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 4-4

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 5-4
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 5-4
    
    ASSERT_SCORE(5, 4);
}

GAME_TEST(2, ShootoutMode_extra2_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 点球");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-0
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-1
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 1-1
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 1-2
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 2-2
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 2-3
    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-3
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 3-4
    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 4-4
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 4-4

    ASSERT_PRI_MSG(OK, 0, "市民");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 5-4
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 5-5

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 5-5
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "皇帝");   // 5-5

    ASSERT_PRI_MSG(OK, 0, "皇帝");
    ASSERT_PRI_MSG(CHECKOUT, 1, "奴隶");   // 5-5
    ASSERT_PRI_MSG(OK, 0, "奴隶");
    ASSERT_PRI_MSG(CHECKOUT, 1, "市民");   // 5-6
    
    ASSERT_SCORE(5, 6);
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
