// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 玩家不足，游戏无法开始
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// 2人全程超时测试
// 超时自动选择剩余最小数字，两人始终选相同数字（1,2,...,10），始终为一组
// 每轮 sum = 2*k, 2*k % k = 0, 全部得分
// R1: +1, R2: +2+2=4, R3: +3+2=5, ..., R10: +10+2=12
// 总分 = 1 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 = 73
// R10 自动提交，只需 9 次超时（R1~R9）
GAME_TEST(2, timeout_all_rounds)
{
    START_GAME();
    for (int i = 0; i < 9; i++) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(73, 73);
}

// 3人全程超时测试
// 同理，sum = 3*k, 3*k % k = 0, 全部得分，总分也为73
// R10 自动提交，只需 9 次超时（R1~R9）
GAME_TEST(3, timeout_three_players)
{
    START_GAME();
    for (int i = 0; i < 9; i++) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(73, 73, 73);
}

// 2人手动提交测试：验证基本得分、连续得分和分组提交
GAME_TEST(2, basic_scoring)
{
    START_GAME();

    // R1: P0=5, P1=5, sum=10, 10%5=0 → 各+5. Scores: 5, 5
    ASSERT_PRI_MSG(OK, 0, "5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "5");

    // R2: 两人上轮均出5，同一组。P0=3, P1=7, sum=10, 10%3!=0, 10%7!=0 → 均未得分
    // Scores: 5, 5
    ASSERT_PRI_MSG(OK, 0, "3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "7");

    // R3: 两人上轮不同(3,7)，分两组: [P0(3)] then [P1(7)]
    // Group 1: P0=2
    ASSERT_PRI_MSG(CHECKOUT, 0, "2");
    // Group 2: P1=8
    ASSERT_PRI_MSG(CHECKOUT, 1, "8");
    // sum=10, 10%2=0(P0+2), 10%8!=0(P1 no). P0 not consecutive(R2 no). Scores: 7, 5

    // R4: groups [P0(2)] then [P1(8)]
    // Group 1: P0=10
    ASSERT_PRI_MSG(CHECKOUT, 0, "10");
    // Group 2: P1=10
    ASSERT_PRI_MSG(CHECKOUT, 1, "10");
    // sum=20, 20%10=0 → both +10. P0 consecutive(R3 yes) +2=12, P1 not consecutive. Scores: 19, 15

    // Timeout remaining 6 rounds (不再 Hook 玩家，每个 stage 均需单独超时)
    // R5: groups [P0(10), P1(10)] → 1 group → 1 timeout
    //   auto: P0→1(used:{2,3,5,10}), P1→1(used:{5,7,8,10})
    //   sum=2, 2%1=0 → both +1+2(consecutive)=3. Scores: 22, 18

    // R6: both had 1 → 1 group → 1 timeout
    //   auto: P0→4(used:{1,2,3,5,10}), P1→2(used:{1,5,7,8,10})
    //   sum=6, 6%4!=0, 6%2=0. P0 no. P1+2+2(consecutive)=4. Scores: 22, 22

    // R7: groups [P1(2)] then [P0(4)] → 2 timeouts
    //   P1 auto→3, P0 auto→6, sum=9, 9%3=0(P1+3+2=5), 9%6!=0. Scores: 22, 27

    // R8: groups [P1(3)] then [P0(6)] → 2 timeouts
    //   P1 auto→4, P0 auto→7, sum=11, no score. Scores: 22, 27

    // R9: groups [P1(4)] then [P0(7)] → 2 timeouts
    //   P1 auto→6, P0 auto→8, sum=14, no score. Scores: 22, 27

    // R10: auto-submitted (每人仅剩一个数字)
    //   P1 auto→9, P0 auto→9, sum=18, 18%9=0 → both +9. Scores: 31, 36

    // Total timeouts for R5-R9: 1+1+2+2+2 = 8 (R10 auto-submitted)
    for (int i = 0; i < 8; i++) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    ASSERT_SCORE(31, 36);
}

// 验证第1轮公开消息被拒绝，第2轮起公开消息可用
GAME_TEST(2, public_message_test)
{
    START_GAME();
    // R1: 公开提交应失败
    ASSERT_PUB_MSG(FAILED, 0, "5");
    // R1: 私信提交正常
    ASSERT_PRI_MSG(OK, 0, "5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "5");

    // R2: 两人上轮均出5，同一组，公开提交应成功
    ASSERT_PUB_MSG(OK, 0, "3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "3");

    // 超时完成剩余回合（两人始终选相同数字，始终一组，R10自动提交）
    for (int i = 0; i < 7; i++) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    // R1:+5, R2:+3+2, R3~R10:连续得分, 总分=5+5+3+4+6+8+9+10+11+12=73
    ASSERT_SCORE(73, 73);
}

// 验证重复提交数字被拒绝
GAME_TEST(2, reject_used_number)
{
    START_GAME();
    // R1: 提交5
    ASSERT_PRI_MSG(OK, 0, "5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "5");
    // R2: 尝试再次提交5（应失败），然后提交3
    ASSERT_PRI_MSG(FAILED, 0, "5");
    ASSERT_PRI_MSG(OK, 0, "3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "3");
    // 超时完成剩余回合（两人始终选相同数字，始终一组，R10自动提交）
    for (int i = 0; i < 7; i++) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    // R1:+5, R2:+3+2, R3~R10:连续得分, 总分=5+5+3+4+6+8+9+10+11+12=73
    ASSERT_SCORE(73, 73);
}

// 2人手动提交全程测试：验证所有10轮得分计算
GAME_TEST(2, full_manual_game)
{
    START_GAME();

    // 两人每轮选相同数字，确保始终一组
    // R1: both 1, sum=2, +1 each. Scores: 1, 1
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    // R2: both 2, sum=4, +2+2(consecutive)=4 each. Scores: 5, 5
    ASSERT_PRI_MSG(OK, 0, "2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "2");
    // R3: both 3, sum=6, +3+2=5 each. Scores: 10, 10
    ASSERT_PRI_MSG(OK, 0, "3");
    ASSERT_PRI_MSG(CHECKOUT, 1, "3");
    // R4: both 4, sum=8, +4+2=6 each. Scores: 16, 16
    ASSERT_PRI_MSG(OK, 0, "4");
    ASSERT_PRI_MSG(CHECKOUT, 1, "4");
    // R5: both 5, sum=10, +5+2=7 each. Scores: 23, 23
    ASSERT_PRI_MSG(OK, 0, "5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "5");
    // R6: both 6, sum=12, +6+2=8 each. Scores: 31, 31
    ASSERT_PRI_MSG(OK, 0, "6");
    ASSERT_PRI_MSG(CHECKOUT, 1, "6");
    // R7: both 7, sum=14, +7+2=9 each. Scores: 40, 40
    ASSERT_PRI_MSG(OK, 0, "7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "7");
    // R8: both 8, sum=16, +8+2=10 each. Scores: 50, 50
    ASSERT_PRI_MSG(OK, 0, "8");
    ASSERT_PRI_MSG(CHECKOUT, 1, "8");
    // R9: both 9, sum=18, +9+2=11 each. Scores: 61, 61
    ASSERT_PRI_MSG(OK, 0, "9");
    ASSERT_PRI_MSG(CHECKOUT, 1, "9");
    // R10: auto-submitted, both 10, sum=20, +10+2=12 each. Scores: 73, 73

    ASSERT_SCORE(73, 73);
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
