// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 单人无法开局（必须 2 人）
GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// pid 0 退出 → 判负，对手获胜
GAME_TEST(2, leave_p0_loses)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 0);
    ASSERT_SCORE(-1, 1);
}

// pid 1 退出 → 判负，对手获胜
GAME_TEST(2, leave_p1_loses)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 1);
    ASSERT_SCORE(1, -1);
}

// 当前行动方（先手 pid 0）超时 → 判负
GAME_TEST(2, timeout_loses)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(-1, 1);
}

// 认输 → 认输方判负
GAME_TEST(2, concede_loses)
{
    START_GAME();
    ASSERT_PUB_MSG(CHECKOUT, 0, "认输");
    ASSERT_SCORE(-1, 1);
}

// 非当前回合的玩家无法落子
GAME_TEST(2, cannot_act_out_of_turn)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 1, "55");   // 先手为 pid 0，pid 1 不能先落子
    ASSERT_PUB_MSG(CONTINUE, 0, "55"); // pid 0 正常落子
}

// 非法坐标被拒绝
GAME_TEST(2, invalid_coordinate)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "5");   // 长度不足
    ASSERT_PUB_MSG(FAILED, 0, "555"); // 长度过长
    ASSERT_PUB_MSG(FAILED, 0, "50");  // 小格号 0 非法
    ASSERT_PUB_MSG(FAILED, 0, "05");  // 大格号 0 非法
}

// 强制宫格限制：上一手落在第 5 格 → 对手被限定在第 5 宫格
GAME_TEST(2, forced_board_restriction)
{
    START_GAME();
    ASSERT_PUB_MSG(CONTINUE, 0, "15"); // O 落第 1 宫格第 5 格 → X 被限定第 5 宫格
    ASSERT_PUB_MSG(FAILED, 1, "11");   // X 试图落第 1 宫格 → 失败
    ASSERT_PUB_MSG(CONTINUE, 1, "53"); // X 落第 5 宫格 → 成功
}

// 赢得一个小棋盘后，对手被送往该已结束宫格时获得自由落子权
GAME_TEST(2, free_choice_after_subboard_win)
{
    START_GAME();
    ASSERT_PUB_MSG(CONTINUE, 0, "15"); // O 第1宫第5格 → X 限定第5宫
    ASSERT_PUB_MSG(CONTINUE, 1, "51"); // X 第5宫第1格 → O 限定第1宫
    ASSERT_PUB_MSG(CONTINUE, 0, "14"); // O 第1宫第4格 → X 限定第4宫
    ASSERT_PUB_MSG(CONTINUE, 1, "41"); // X 第4宫第1格 → O 限定第1宫
    ASSERT_PUB_MSG(CONTINUE, 0, "16"); // O 第1宫第6格，连成 4-5-6 → O 赢得第1宫；→ X 限定第6宫
    ASSERT_PUB_MSG(CONTINUE, 1, "61"); // X 第6宫第1格 → 本应限定第1宫，但第1宫已结束 → O 自由落子
    ASSERT_PUB_MSG(CONTINUE, 0, "25"); // O 自由落子于第2宫（若仍被限定第1宫则会失败）
}

// 正常连成大棋盘三宫获胜：胜者 1 分、负者 0 分（区别于认输/超时/退出的 -1）
GAME_TEST(2, normal_win_scores_1_and_0)
{
    START_GAME();
    ASSERT_PUB_MSG(CONTINUE, 0, "15");
    ASSERT_PUB_MSG(CONTINUE, 1, "51");
    ASSERT_PUB_MSG(CONTINUE, 0, "14");
    ASSERT_PUB_MSG(CONTINUE, 1, "41");
    ASSERT_PUB_MSG(CONTINUE, 0, "16"); // O 连成 4-5-6 → 赢得第 1 宫
    ASSERT_PUB_MSG(CONTINUE, 1, "61");
    ASSERT_PUB_MSG(CONTINUE, 0, "97");
    ASSERT_PUB_MSG(CONTINUE, 1, "79");
    ASSERT_PUB_MSG(CONTINUE, 0, "98");
    ASSERT_PUB_MSG(CONTINUE, 1, "89");
    ASSERT_PUB_MSG(CONTINUE, 0, "99"); // O 连成 7-8-9 → 赢得第 9 宫
    ASSERT_PUB_MSG(CONTINUE, 1, "75");
    ASSERT_PUB_MSG(CONTINUE, 0, "56");
    ASSERT_PUB_MSG(CONTINUE, 1, "65");
    ASSERT_PUB_MSG(CONTINUE, 0, "53");
    ASSERT_PUB_MSG(CONTINUE, 1, "35");
    ASSERT_PUB_MSG(CHECKOUT, 0, "59"); // O 连成 3-6-9 赢得第 5 宫 → 大棋盘 1·5·9 对角线，O 获胜
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
