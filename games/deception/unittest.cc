// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

// 两名玩家持续出假卡且无人质疑，同回合弃光假卡，平局获胜
GAME_TEST(2, both_discard_to_win)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "假");
    ASSERT_PRI_MSG(CHECKOUT, 1, "假");
    ASSERT_PRI_MSG(OK, 0, "假");
    ASSERT_PRI_MSG(CHECKOUT, 1, "假");
    ASSERT_PRI_MSG(OK, 0, "假");
    ASSERT_PRI_MSG(CHECKOUT, 1, "假");

    ASSERT_SCORE(0, 0);
}

// 质疑命中出假卡玩家，对方出局，质疑者成为最后存活者获胜
GAME_TEST(2, challenge_catches_fake)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "假");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");

    // 0 号出局记 -7；1 号未出牌，保留 3 张假卡，按剩余假卡计 -3
    ASSERT_SCORE(-7, -3);
}

// 多人质疑同一出真卡玩家：每名质疑者各 +1 假卡，真卡玩家相应减少
GAME_TEST(3, multi_challenge_true_card)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "真");
    ASSERT_PRI_MSG(OK, 1, "1");
    ASSERT_PRI_MSG(CHECKOUT, 2, "1");
    // 0 号：真卡被 2 人质疑，假卡 3->1；1、2 号：各获得 1 张假卡，3->4

    ASSERT_PRI_MSG(OK, 1, "真");
    ASSERT_PRI_MSG(OK, 2, "真");
    ASSERT_PRI_MSG(CHECKOUT, 0, "假");
    // 0 号：出假卡未被质疑，1->0，弃光获胜，游戏结束
    // 1、2 号：出真卡未被质疑，4->5

    ASSERT_SCORE(0, -5, -5);
}

// 全员超时，统一判定出局
GAME_TEST(2, timeout_test)
{
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(-7, -7);
}

// 全员退出，统一判定出局
GAME_TEST(2, leave_test)
{
    START_GAME();

    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_LEAVE(CHECKOUT, 1);

    ASSERT_SCORE(-7, -7);
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
