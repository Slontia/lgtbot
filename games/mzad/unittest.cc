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

GAME_TEST(3, simple_score_test)   // 基础得分测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: 所有人选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 得分: 8, 8, 8

    // 回合2: 所有人受限(上回合=8)，选反制(3)
    ASSERT_PRI_MSG(OK, 0, "反制");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 得分: 11, 11, 11 → 全部≥10，游戏结束

    ASSERT_SCORE(11, 11, 11);
}

GAME_TEST(3, assassinate_success_test)   // 刺杀成功测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0刺杀P1猜8, P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 8");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P0猜中P1的8，P1淘汰，P0获得8分
    // 得分: 8, 0, 8. P1淘汰

    // 回合2: P0和P2 (P1已淘汰自动ready)
    ASSERT_PRI_MSG(OK, 0, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 得分: 11, 0, 11 → ≥10，游戏结束

    ASSERT_SCORE(11, 0, 11);
}

GAME_TEST(3, counter_success_test)   // 反制成功测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0刺杀P1猜5, P1选反制(3), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 5");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P1反制P0，P0淘汰，累计分清零
    // 得分: 0, 3, 8. P0淘汰

    // 回合2: P1和P2 (P0已淘汰)
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P1受限不能选巨型...wait P1上回合选的3不是8，不受限
    // 得分: 0, 11, 11 → ≥10

    ASSERT_SCORE(0, 11, 11);
}

GAME_TEST(3, destroy_test)   // 毁灭测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0选毁灭(5), P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 毁灭：最大数字8不得分 → P1和P2不得分
    // P0得5分
    // 得分: 5, 0, 0

    // 回合2: P0选毁灭(5), P1和P2受限选反制(3)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 毁灭：最大数字5不得分 → P0不得分
    // P1和P2得3分
    // 得分: 5, 3, 3

    // 回合3: P0选巨型(8), P1和P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 得分: 13, 11, 11 → ≥10

    ASSERT_SCORE(13, 11, 11);
}

GAME_TEST(4, shield_test)   // 圣盾测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0选圣盾(4), P1刺杀P0猜4, P2拼点P0, P3选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "圣盾");
    ASSERT_PRI_MSG(OK, 1, "刺杀 1 4");
    ASSERT_PRI_MSG(OK, 2, "拼点 1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "巨型");
    // P0被P1(刺杀)和P2(拼点)选择，≥2人 → 圣盾生效
    // P1和P2技能失效且不得分
    // P0得4分, P3得8分
    // 得分: 4, 0, 0, 8

    // 回合2: P0选巨型(8), P1选巨型(8), P2选巨型(8), P3受限选反制(3)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(OK, 2, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 3, "反制");
    // 得分: 12, 8, 8, 11 → P0和P3 ≥10

    ASSERT_SCORE(12, 8, 8, 11);
}

GAME_TEST(3, compete_test)   // 拼点测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0拼点P1, P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "拼点 2");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P0(6) vs P1(8): 6<8 → P0不得分, P1得8分
    // P2得8分
    // 得分: 0, 8, 8

    // 回合2: P0拼点P2, P1受限选反制(3), P2受限选反制(3)
    ASSERT_PRI_MSG(OK, 0, "拼点 3");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P0(6) vs P2(3): 6>3 → P2不得分, P0得6分
    // P1得3分
    // 得分: 6, 11, 8 → P1 ≥10

    ASSERT_SCORE(6, 11, 8);
}

GAME_TEST(3, giant_restriction_test)   // 巨型限制测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0选巨型(8), P1选反制(3), P2选反制(3)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 得分: 8, 3, 3

    // 回合2: P0受限 → 6/7/8应全部失败
    ASSERT_PRI_MSG(FAILED, 0, "拼点 2");
    ASSERT_PRI_MSG(FAILED, 0, "弃牌");
    ASSERT_PRI_MSG(FAILED, 0, "巨型");
    // P0选毁灭(5), P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 毁灭：最大8不得分
    // 得分: 13, 3, 3 → P0 ≥10

    ASSERT_SCORE(13, 3, 3);
}

GAME_TEST(3, elimination_zeros_score_test)   // 淘汰清零分数测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: 所有人选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 得分: 8, 8, 8

    // 回合2: 所有人受限，P0刺杀P1猜3，P1反制(3)，P2反制(3)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 3");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P0刺杀P1，P1反制，P0淘汰 → P0累计分清零（从8→0）
    // P1得3分，P2得3分
    // 得分: 0, 11, 11 → P1和P2 ≥10

    ASSERT_SCORE(0, 11, 11);
}

GAME_TEST(3, self_target_test)   // 不能选择自己为目标
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "刺杀 1 5");   // P0不能刺杀自己(1号)
    ASSERT_PRI_MSG(FAILED, 1, "拼点 2");      // P1不能拼点自己(2号)

    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");

    ASSERT_PRI_MSG(OK, 0, "反制");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");

    ASSERT_SCORE(11, 11, 11);
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
