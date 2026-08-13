// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 得分说明：ASSERT_SCORE 校验的是最终得分。
// 存活玩家的最终得分 = 获得分 + 存活分，淘汰玩家仅保留存活分。
// 目标分数只比较「获得分」，存活分不参与判定，因此下方注释中的得分
// 达到目标分数也不一定结束游戏。

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
    // 得分: 9, 9, 9 (8 + 存活1)

    // 回合2: 所有人受限(上回合=8)，选反制(3)
    ASSERT_PRI_MSG(OK, 0, "反制");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 得分: 13, 13, 13 → 全部≥10，游戏结束

    ASSERT_SCORE(13, 13, 13);
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
    // P1第1回合即被淘汰，存活分为0，仅保留0分
    // 得分: 9, 0, 9. P1淘汰

    // 回合2: P0和P2 (P1已淘汰自动ready)
    ASSERT_PRI_MSG(OK, 0, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 得分: 13, 0, 13 → ≥10，游戏结束

    ASSERT_SCORE(13, 0, 13);
}

GAME_TEST(3, counter_success_test)   // 反制成功测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0刺杀P1猜5, P1选反制(3), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 5");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P1反制成功，P0第1回合即被淘汰，仅保留存活分0分
    // 得分: 0, 4, 9. P0淘汰

    // 回合2: P1和P2 (P0已淘汰)
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P1上回合选的3不是8，不受限
    // 得分: 0, 13, 13 → ≥10

    ASSERT_SCORE(0, 13, 13);
}

GAME_TEST(3, destroy_test)   // 毁灭测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0选毁灭(5), P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 毁灭：最大数字8不得分 → P1和P2仅得存活1分
    // P0得5分 + 存活1分
    // 得分: 6, 1, 1

    // 回合2: P0选毁灭(5), P1和P2受限选反制(3)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 毁灭：最大数字5不得分 → P0仅得存活1分
    // P1和P2得3分 + 存活1分
    // 得分: 7, 5, 5

    // 回合3: P0选巨型(8), P1和P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 得分: 16, 14, 14 → ≥10

    ASSERT_SCORE(16, 14, 14);
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
    // P1和P2技能失效且不得分，仅得存活1分
    // P0得4分, P3得8分，均+存活1分
    // 得分: 5, 1, 1, 9

    // 回合2: P0选巨型(8), P1选巨型(8), P2选巨型(8), P3受限选反制(3)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(OK, 2, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 3, "反制");
    // 得分: 14, 10, 10, 13 → 全部 ≥10

    ASSERT_SCORE(14, 10, 10, 13);
}

GAME_TEST(3, compete_test)   // 拼点测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0拼点P1, P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "拼点 2");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P0(6) vs P1(8): 6<8 → P0仅得存活1分, P1得8分
    // P2得8分
    // 得分: 1, 9, 9

    // 回合2: P0拼点P2, P1受限选反制(3), P2受限选反制(3)
    ASSERT_PRI_MSG(OK, 0, "拼点 3");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P0(6) vs P2(3): 6>3 → P2仅得存活1分, P0得6分
    // P1得3分
    // 得分: 8, 13, 10 → P1和P2 ≥10

    ASSERT_SCORE(8, 13, 10);
}

GAME_TEST(3, giant_restriction_test)   // 巨型限制测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0选巨型(8), P1选反制(3), P2选反制(3)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // 得分: 9, 4, 4

    // 回合2: P0受限 → 6/7/8应全部失败
    ASSERT_PRI_MSG(FAILED, 0, "拼点 2");
    ASSERT_PRI_MSG(FAILED, 0, "弃牌");
    ASSERT_PRI_MSG(FAILED, 0, "巨型");
    // P0选毁灭(5), P1选巨型(8), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 毁灭：最大8不得分，P1和P2仅得存活1分
    // 得分: 15, 5, 5 → P0 ≥10

    ASSERT_SCORE(15, 5, 5);
}

GAME_TEST(3, elimination_keeps_survive_score_test)   // 淘汰仅保留存活分测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: 所有人选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // 得分: 9, 9, 9

    // 回合2: 所有人受限，P0刺杀P1猜5（非3，与「刺杀反制」配置无关），P1反制(3)，P2反制(3)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 5");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P0被P1反制淘汰 → 仅保留存活分（存活1回合，从9→1）
    // P1得3分，P2得3分，均+存活1分
    // 得分: 1, 13, 13 → P1和P2 ≥10

    ASSERT_SCORE(1, 13, 13);
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

    ASSERT_SCORE(13, 13, 13);
}

GAME_TEST(2, survive_score_test)   // 存活得分测试
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0选毁灭(5), P1选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "毁灭");
    ASSERT_PRI_MSG(CHECKOUT, 1, "巨型");
    // 毁灭：最大数字8不得分 → P1行动得0分，仍得存活1分
    // 得分: 6, 1

    // 回合2: P0拼点P1, P1受限选毁灭(5)
    ASSERT_PRI_MSG(OK, 0, "拼点 2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "毁灭");
    // 毁灭：最大数字6不得分 → P0行动得0分
    // 拼点：6>5 → P1行动得0分
    // 双方行动均得0分，但都存活，各得存活1分
    // 得分: 7, 2

    // 回合3: P0刺杀P1猜4（非3，与「刺杀反制」配置无关）, P1选反制(3)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 4");
    ASSERT_PRI_MSG(CHECKOUT, 1, "反制");
    // P0被反制淘汰，当回合无存活分，仅保留前2回合的存活分2分
    // P1得3分 + 存活1分，仅剩1人游戏结束
    // 得分: 2, 6

    ASSERT_SCORE(2, 6);
}

GAME_TEST(2, counter_beats_assassinate_test)   // 默认关闭「刺杀反制」，猜中3也无法淘汰反制
{
    START_GAME();

    // 回合1: P0刺杀P1猜3, P1选反制(3)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "反制");
    // 默认未开启「刺杀反制」，即使猜中3仍由反制生效，P0被淘汰
    // P0第1回合被淘汰，仅保留存活分0分；P1得3分 + 存活1分
    // 仅剩1人游戏结束

    ASSERT_SCORE(0, 4);
}

GAME_TEST(3, assassinate_counter_test)   // 开启「刺杀反制」：必须指定刺杀3
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    ASSERT_PUB_MSG(OK, 0, "刺杀反制 开启");
    START_GAME();

    // 回合1: P0刺杀P1猜3, P1选反制(3), P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 3");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P0猜中3，反制无效，P1被刺杀淘汰，P0改为获得3分
    // P1第1回合被淘汰，存活分为0
    // 得分: 4, 0, 9

    // 回合2: P2刺杀P0猜5, P0选反制(3)
    ASSERT_PRI_MSG(OK, 2, "刺杀 1 5");
    ASSERT_PRI_MSG(CHECKOUT, 0, "反制");
    // P2猜的不是3，反制生效，P2被淘汰，仅保留存活分1分
    // P0得3分 + 存活1分，仅剩1人游戏结束
    // 得分: 8, 0, 1

    ASSERT_SCORE(8, 0, 1);
}

GAME_TEST(3, target_score_preset_test)   // 快捷设置目标分数
{
    // 范围与 options.h 一致（10~100），超出范围的预设指令无效
    ASSERT_PUB_MSG(FAILED, 0, "目标分数 9");
    ASSERT_PUB_MSG(FAILED, 0, "目标分数 101");
    ASSERT_PUB_MSG(OK, 0, "目标分数 100");
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: 所有人选巨型(8) → 9分，未达目标分数
    ASSERT_PRI_MSG(OK, 0, "巨型");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");

    // 回合2: 所有人受限选反制(3) → 13分，达到目标分数游戏结束
    ASSERT_PRI_MSG(OK, 0, "反制");
    ASSERT_PRI_MSG(OK, 1, "反制");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");

    ASSERT_SCORE(13, 13, 13);
}

GAME_TEST(2, counter_preset_test)   // 快捷配置刺杀反制
{
    ASSERT_PUB_MSG(FAILED, 0, "刺杀反制 abc");   // 非法参数
    ASSERT_PUB_MSG(OK, 0, "刺杀反制 关闭");
    ASSERT_PUB_MSG(OK, 0, "刺杀反制");           // 省略参数视为开启
    START_GAME();

    // 回合1: P0刺杀P1猜3, P1选反制(3)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "反制");
    // 已开启「刺杀反制」，P0猜中3淘汰P1，改为获得3分 + 存活1分
    // 仅剩1人游戏结束

    ASSERT_SCORE(4, 0);
}

GAME_TEST(3, elimination_rank_test)   // 淘汰名次由存活分区分
{
    ASSERT_PUB_MSG(OK, 0, "目标分数 10");
    START_GAME();

    // 回合1: P0刺杀P1猜8, P1选巨型(8), P2选反制(3)
    ASSERT_PRI_MSG(OK, 0, "刺杀 2 8");
    ASSERT_PRI_MSG(OK, 1, "巨型");
    ASSERT_PRI_MSG(CHECKOUT, 2, "反制");
    // P0猜中，P1第1回合被淘汰，存活分为0
    // 得分: 9, 0, 4

    // 回合2: P0刺杀P2猜8, P2选巨型(8)
    ASSERT_PRI_MSG(OK, 0, "刺杀 3 8");
    ASSERT_PRI_MSG(CHECKOUT, 2, "巨型");
    // P0猜中，P2第2回合被淘汰，保留存活分1分
    // 仅剩1人游戏结束

    // P1与P2均被淘汰，但存活回合数不同，得分可区分名次: 0 < 1
    ASSERT_SCORE(18, 0, 1);
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
