// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 双方初始局面呈中心对称，因此同一套走法在两种先手下互为镜像。
// 下列数组按玩家编号（即阵营）索引：0 为红方，1 为蓝方
// 1 号棋子离开原位的走法，红方向下、蓝方向上
static const char* const k_open[2] = {"1 下", "1 上"};
// 1 号棋子回到原位的走法
static const char* const k_back[2] = {"1 上", "1 下"};
// 取胜序列中对手的过渡走法
static const char* const k_idle[2] = {"2 右", "2 左"};
// 取胜序列中形成直线的走法
static const char* const k_finish[2] = {"2 下", "2 上"};

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(3, too_many_players)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, status_command)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PUB_MSG(OK, 1, "赛况");
    ASSERT_FINISHED(false);
}

GAME_TEST(2, win_by_line)
{
    START_GAME();
    uint32_t first = 0;
    if (PrivateRequest(0, k_open[0]) == StageErrCode::FAILED) {
        // 玩家 0 被拒绝，说明先手是玩家 1
        first = 1;
        ASSERT_PUB_MSG(CHECKOUT, 1, k_open[1]);
    }
    const uint32_t second = 1 - first;
    ASSERT_PUB_MSG(CHECKOUT, second, k_idle[second]);
    ASSERT_FINISHED(false);
    ASSERT_PUB_MSG(CHECKOUT, first, k_finish[first]);
    ASSERT_FINISHED(true);
    if (first == 0) {
        ASSERT_SCORE(1, 0);
    } else {
        ASSERT_SCORE(0, 1);
    }
}

GAME_TEST(2, cannot_move_in_opponent_turn)
{
    START_GAME();
    // 无论先手是谁，该探测之后都轮到玩家 1 行动
    PrivateRequest(0, k_open[0]);
    ASSERT_PUB_MSG(FAILED, 0, "3 上");
    ASSERT_FINISHED(false);
}

GAME_TEST(2, blocked_direction_rejected)
{
    START_GAME();
    PrivateRequest(0, k_open[0]);
    // 蓝方 1 号棋子位于底行，向下紧邻墙壁，无法滑动
    ASSERT_PUB_MSG(FAILED, 1, "1 下");
    ASSERT_FINISHED(false);
}

GAME_TEST(2, compact_format_accepted)
{
    START_GAME();
    PrivateRequest(0, k_open[0]);
    // 编号与方向连写的紧凑格式同样可用
    ASSERT_PUB_MSG(CHECKOUT, 1, "1上");
    ASSERT_FINISHED(false);
}

GAME_TEST(2, invalid_input_rejected)
{
    START_GAME();
    PrivateRequest(0, k_open[0]);
    // 紧凑格式中方向无法识别
    ASSERT_PUB_MSG(FAILED, 1, "1前");
    // 棋子编号超出范围
    ASSERT_PUB_MSG(FAILED, 1, "9上");
    // 分开书写时方向不匹配任何指令
    ASSERT_PUB_MSG(NOT_FOUND, 1, "1 前");
    ASSERT_FINISHED(false);
}

GAME_TEST(2, concede_lose)
{
    START_GAME();
    ASSERT_PUB_MSG(CHECKOUT, 0, "认输");
    ASSERT_FINISHED(true);
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, concede_in_opponent_turn_lose)
{
    START_GAME();
    PrivateRequest(0, k_open[0]);
    // 非行动方同样可以认输
    ASSERT_PUB_MSG(CHECKOUT, 0, "投降");
    ASSERT_FINISHED(true);
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, timeout_lose)
{
    START_GAME();
    PrivateRequest(0, k_open[0]);
    // 该探测之后轮到玩家 1 行动，超时的一定是玩家 1
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_FINISHED(true);
    ASSERT_SCORE(0, -1);
}

GAME_TEST(2, timeout_at_first_round)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_FINISHED(true);
}

GAME_TEST(2, leave_lose)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 0);
    ASSERT_FINISHED(true);
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, non_current_player_leave_lose)
{
    START_GAME();
    PrivateRequest(0, k_open[0]);
    // 非行动方中途退出同样判负
    ASSERT_LEAVE(CHECKOUT, 0);
    ASSERT_FINISHED(true);
    ASSERT_SCORE(-1, 0);
}

GAME_TEST(2, repetition_draw)
{
    START_GAME();
    uint32_t first = 0;
    if (PrivateRequest(0, k_open[0]) == StageErrCode::FAILED) {
        first = 1;
        ASSERT_PUB_MSG(CHECKOUT, 1, k_open[1]);
    }
    const uint32_t second = 1 - first;
    // 双方各自把 1 号棋子来回滑动，一个循环后回到初始局面
    ASSERT_PUB_MSG(CHECKOUT, second, k_open[second]);
    ASSERT_PUB_MSG(CHECKOUT, first, k_back[first]);
    ASSERT_PUB_MSG(CHECKOUT, second, k_back[second]);
    // 再走一个循环，初始局面第三次出现
    ASSERT_PUB_MSG(CHECKOUT, first, k_open[first]);
    ASSERT_PUB_MSG(CHECKOUT, second, k_open[second]);
    ASSERT_PUB_MSG(CHECKOUT, first, k_back[first]);
    ASSERT_FINISHED(false);
    ASSERT_PUB_MSG(CHECKOUT, second, k_back[second]);
    ASSERT_FINISHED(true);
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, computer_play_to_the_end)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 20");
    START_GAME();
    for (uint32_t i = 0; i < 500 && !this->main_stage_->IsOver(); ++i) {
        this->ComputerActRequest_(i % 2);
    }
    ASSERT_FINISHED(true);
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
