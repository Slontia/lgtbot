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

GAME_TEST(2, leave_test1)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 0);

    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, leave_test2)
{
    START_GAME();

    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(0, -1);
}

GAME_TEST(2, sample_test)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "人质 17");
    ASSERT_PRI_MSG(OK, 0, "拿刀 19");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "17");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不开枪");  // [R1] 人质1/2

    ASSERT_PRI_MSG(OK, 0, "人质 14");
    ASSERT_PRI_MSG(OK, 0, "拿刀 15");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "15");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不开枪");  // [R2] 警察血量-1

    ASSERT_PRI_MSG(OK, 0, "人质 13");
    ASSERT_PRI_MSG(OK, 0, "烟雾弹");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "12");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不开枪");  // [R3] 没有接到

    ASSERT_PRI_MSG(OK, 0, "人质 11");
    ASSERT_PRI_MSG(OK, 0, "拿刀 10");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "9");       // [R4] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 7");
    ASSERT_PRI_MSG(OK, 0, "拿刀 8");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "6");       // [R5] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 4");
    ASSERT_PRI_MSG(OK, 0, "拿刀 5");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "3");       // [R6] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 2");
    ASSERT_PRI_MSG(OK, 0, "拿刀 1");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不开枪");  // [R7] 人质2/2

    ASSERT_PRI_MSG(OK, 0, "拿刀 1");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // [R8] 杀手血量-1

    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, zero_bullet_test1)
{
    ASSERT_PUB_MSG(OK, 0, "子弹 1");
    ASSERT_PUB_MSG(OK, 0, "警察血量 1");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "人质 19");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "19");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // 子弹归0

    ASSERT_PRI_MSG(OK, 0, "人质 18");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "18");      // 无子弹接人质

    ASSERT_PRI_MSG(OK, 0, "人质 16");
    ASSERT_PRI_MSG(OK, 0, "拿刀 17");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "17");      // 无子弹中刀

    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, zero_bullet_test2)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "人质 18");
    ASSERT_PRI_MSG(OK, 0, "拿刀 17");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "19");      // [R1] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 17");
    ASSERT_PRI_MSG(OK, 0, "拿刀 16");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "18");      // [R2] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 15");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "16");      // [R3] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 14");
    ASSERT_PRI_MSG(OK, 0, "拿刀 13");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "13");
    ASSERT_PRI_MSG(CHECKOUT, 1, "不开枪");  // [R4] 警察血量-1

    ASSERT_PRI_MSG(OK, 0, "人质 12");
    ASSERT_PRI_MSG(OK, 0, "烟雾弹");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "12");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // [R5] 误杀人质1/3

    ASSERT_PRI_MSG(OK, 0, "人质 10");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "11");      // [R6] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 8");
    ASSERT_PRI_MSG(OK, 0, "拿刀 9");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "10");      // [R7] 无事发生

    ASSERT_PRI_MSG(OK, 0, "人质 9");
    ASSERT_PRI_MSG(OK, 0, "拿刀 7");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // [R8] 杀手血量-1

    ASSERT_PRI_MSG(OK, 0, "人质 5");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // [R9] 误杀人质2/3

    ASSERT_PRI_MSG(OK, 0, "人质 3");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "4");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // [R10] 开枪未命中

    ASSERT_PRI_MSG(OK, 0, "人质 2");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "开枪");    // [R11] 开枪未命中（子弹耗尽）

    ASSERT_PRI_MSG(OK, 0, "人质 1");
    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2");       // [R12] 无子弹没接到

    ASSERT_PRI_MSG(OK, 0, "确认");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");       // [R13] 无子弹没接到

    ASSERT_SCORE(1, 0);
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
