// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(2, leave_test1)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 0);

    ASSERT_SCORE(-300, 0);
}

GAME_TEST(2, leave_test2)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(0, -300);
}

GAME_TEST(3, all_active_stop1)
{
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CONTINUE, 1, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 2, "停止");

    ASSERT_SCORE(0, 0, 0);
}

GAME_TEST(3, all_active_stop2)
{
    START_GAME();

    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PUB_MSG(CHECKOUT, 2, "停止");

    ASSERT_SCORE(0, 0, 0);
}

GAME_TEST(3, all_active_stop3)
{
    START_GAME();

    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(0, 0, 0);
}

// 幻变模式正常开局
GAME_TEST(2, twist_mode_start)
{
    ASSERT_PRI_MSG(OK, 0, "模式 幻变");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");

    ASSERT_SCORE(0, 0);
}

// 巨大的心房区块铺满地图可正常开局
GAME_TEST(2, heart_blocks_full_map)
{
    ASSERT_PRI_MSG(OK, 0, "区块 51 51 51 51 51 51 51 51 51");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");

    ASSERT_SCORE(0, 0);
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
