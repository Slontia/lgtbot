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

GAME_TEST(2, simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 快速");
    ASSERT_PUB_MSG(OK, 0, "完美 关闭");
    START_GAME();

    ASSERT_PUB_MSG(FAILED, 0, "40");
    ASSERT_PRI_MSG(FAILED, 0, "丢");
    ASSERT_PRI_MSG(FAILED, 0, "回");

    ASSERT_PRI_MSG(OK, 0, "40");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "40");  // 179 165 0 0
    
    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 179 165 0 1

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 119 165 0 1

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 119 165 0 2

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 59 165 0 2

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "60");  // 59 145 0 2

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "0");   // 0 145 0 2

    ASSERT_SCORE(0, 145);
}

GAME_TEST(2, Timeout_test)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "0");
    ASSERT_TIMEOUT(CHECKOUT);
    
    ASSERT_SCORE(0, -1);
}

GAME_TEST(2, Leave_test)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "0");
    ASSERT_LEAVE(CHECKOUT, 0);
    
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, FaultLimit_fast_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 快速");
    ASSERT_PUB_MSG(OK, 0, "完美 关闭");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "40");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "40");  // 179 165 0 0
    
    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 179 165 0 1

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 119 165 0 1

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 119 165 0 2

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 59 165 0 2

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "20");  // 59 145 0 3

    ASSERT_SCORE(59, 0);
}

GAME_TEST(2, FaultLimit_senior_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 高级");
    ASSERT_PUB_MSG(OK, 0, "完美 关闭");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "1");   // 300 285 1 0
    
    ASSERT_PRI_MSG(OK, 0, "0");         // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "15");  // 300 270 1 0

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "1");   // 300 270 2 0

    ASSERT_PRI_MSG(OK, 0, "1");         // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 300 270 2 1

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "1");   // 300 270 3 1

    ASSERT_PRI_MSG(OK, 0, "0");         // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "60");  // 300 210 3 1

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");   // 300 210 4 1

    ASSERT_SCORE(0, 210);
}

GAME_TEST(2, PerfectLookback_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 高级");
    ASSERT_PUB_MSG(OK, 0, "完美 开启");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "40");        // 1丢0回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "40");  // 300 285 0 0

    ASSERT_SCORE(300, 0);
}

GAME_TEST(2, PerfectLookback_timeup_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 快速");
    ASSERT_PUB_MSG(OK, 0, "完美 开启");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "41");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "40");  // 179 165 0 0
    
    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 179 165 0 1

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 119 165 0 1

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 119 165 0 2

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 59 165 0 2

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "60");  // 59 145 0 2

    ASSERT_PRI_MSG(OK, 0, "58");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 1 145 0 2

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "60");  // 1 125 0 2

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "60");   // 1 125 0 2

    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, FaultLimit_Customfast_test)
{
    ASSERT_PUB_MSG(OK, 0, "生命 150");
    ASSERT_PUB_MSG(OK, 0, "完美 关闭");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "40");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "40");  // 149 135 0 0
    
    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 149 135 0 1

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 89 135 0 1

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "20");  // 89 135 0 2

    ASSERT_PRI_MSG(OK, 0, "60");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 29 135 0 2

    ASSERT_PRI_MSG(OK, 0, "40");        // 0丢1回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "20");  // 29 135 0 3

    ASSERT_SCORE(29, 0);
}

GAME_TEST(2, FaultLimit_Customsenior_test)
{
    ASSERT_PUB_MSG(OK, 0, "生命 600");
    ASSERT_PUB_MSG(OK, 0, "完美 关闭");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "1");   // 600 585 1 0
    
    ASSERT_PRI_MSG(OK, 0, "0");         // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "15");  // 600 570 1 0

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "1");   // 600 570 2 0

    ASSERT_PRI_MSG(OK, 0, "1");         // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "0");   // 600 570 2 1

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 1, "1");   // 600 570 3 1

    ASSERT_PRI_MSG(OK, 0, "0");         // 0丢1回头
    ASSERT_PRI_MSG(CONTINUE, 1, "60");  // 600 510 3 1

    ASSERT_PRI_MSG(OK, 0, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");   // 600 510 4 1

    ASSERT_SCORE(0, 510);
}

GAME_TEST(2, RealTimeMode_simple_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 实时");
    ASSERT_PUB_MSG(OK, 0, "完美 关闭");
    START_GAME();

    ASSERT_PRI_MSG(OK, 1, "丢");        // 1丢0回头
    ASSERT_PRI_MSG(FAILED, 1, "丢");
    ASSERT_PUB_MSG(CONTINUE, 0, "回");  // 179 165 0 0
    ASSERT_PRI_MSG(FAILED, 1, "丢");
    
    ASSERT_PUB_MSG(FAILED, 0, "丢");
    ASSERT_PRI_MSG(OK, 0, "丢");        // 0丢1回头
    ASSERT_TIMEOUT(CONTINUE);           // 179 105 0 0
    ASSERT_PRI_MSG(FAILED, 1, "回");

    ASSERT_PRI_MSG(OK, 1, "丢");        // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 0, "回");  // 178 125 0 0

    ASSERT_PRI_MSG(OK, 0, "丢");        // 0丢1回头
    ASSERT_TIMEOUT(CONTINUE);           // 178 45 0 0

    ASSERT_PRI_MSG(OK, 1, "0");         // 1丢0回头
    ASSERT_PRI_MSG(CONTINUE, 0, "0");   // 177 45 0 0

    ASSERT_PRI_MSG(OK, 0, "丢");        // 0丢1回头
    ASSERT_TIMEOUT(CHECKOUT);           // 177 0 0 0

    ASSERT_SCORE(177, 0);
}

GAME_TEST(2, RealTimeMode_FaultLimit_test)
{
    ASSERT_PUB_MSG(OK, 0, "模式 实时");
    ASSERT_PUB_MSG(OK, 0, "完美 开启");
    START_GAME();

    ASSERT_PRI_MSG(CONTINUE, 0, "回");  // 1丢0回头 180 165 1 0
    ASSERT_PRI_MSG(FAILED, 1, "丢");
    
    ASSERT_PRI_MSG(CONTINUE, 1, "回");  // 0丢1回头 180 105 1 1

    ASSERT_PRI_MSG(CONTINUE, 0, "回");  // 1丢0回头 180 165 2 1
    
    ASSERT_PRI_MSG(CONTINUE, 1, "回");  // 0丢1回头 180 105 2 2
    
    ASSERT_PRI_MSG(CHECKOUT, 0, "回");  // 1丢0回头 180 165 3 2

    ASSERT_SCORE(0, 165);
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
