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


// 多BOSS：配置项可一次配置多个，同时生效
GAME_TEST(2, multi_boss_option)
{
    ASSERT_PRI_MSG(OK, 0, "BOSS 米诺陶斯 邦邦");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");
}

// 多BOSS：一键指令分别配置不再互相覆盖
GAME_TEST(2, multi_boss_init_command)
{
    ASSERT_PRI_MSG(OK, 0, "米诺陶斯 邦邦");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");
}

// 多BOSS：同类型可重复配置生成多只，[无]会被剔除
GAME_TEST(2, multi_boss_duplicate)
{
    ASSERT_PRI_MSG(OK, 0, "BOSS 邦邦 无 邦邦 米诺陶斯");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");
}

// 多BOSS：一键指令重复配置同一BOSS也会叠加生成
GAME_TEST(2, multi_boss_init_duplicate)
{
    ASSERT_PRI_MSG(OK, 0, "米诺陶斯 米诺陶斯 邦邦");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");
}

// 多BOSS：超出数量上限时丢弃多余BOSS，仍正常开局
GAME_TEST(2, multi_boss_exceed_limit)
{
    ASSERT_PRI_MSG(OK, 0, "BOSS 邦邦 邦邦 邦邦 米诺陶斯 米诺陶斯 米诺陶斯 邦邦");
    START_GAME();

    ASSERT_PUB_MSG(CONTINUE, 0, "停止");
    ASSERT_PUB_MSG(CHECKOUT, 1, "停止");
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
