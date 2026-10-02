// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {
namespace game {
namespace GAME_MODULE_NAME {

// Game-level unit tests for test_game, exercising timeout and stage-checkout logic
// directly without the multi-process IPC layer.

// --- Timer / timeout tests ---

GAME_TEST(2, game_over_by_timeup)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);
}

GAME_TEST(2, checkout_substage_by_request)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "准备切换 1");
    ASSERT_PRI_MSG(CHECKOUT, 0, "结束子阶段");
    ASSERT_PRI_MSG(CHECKOUT, 0, "结束子阶段");
}

GAME_TEST(2, checkout_substage_by_timeout)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "准备切换 1");
    ASSERT_TIMEOUT(CHECKOUT);
}

GAME_TEST(2, substage_reset_timer)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "重新计时");
    // First timeout restarts the timer; second timeout ends the substage.
    ASSERT_TIMEOUT(OK);
    ASSERT_TIMEOUT(CHECKOUT);
}

// --- Computer action tests ---

GAME_TEST(2, force_exit_computer)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "电脑失败 0 3");
    ASSERT_COMPUTER_ACT(FAILED, 0);
    ASSERT_COMPUTER_ACT(FAILED, 0);
    ASSERT_COMPUTER_ACT(FAILED, 0);
    ASSERT_COMPUTER_ACT(OK, 0);
}

// --- Eliminate / Hook tests ---

GAME_TEST(2, eliminate_first)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "准备切换 1");
    // Player 1 eliminates themselves
    ASSERT_PRI_MSG(OK, 1, "淘汰");
    // Player 0 is the only one left -> becomes ready -> checkout
    ASSERT_PRI_MSG(CHECKOUT, 0, "准备");
    ASSERT_PRI_MSG(CHECKOUT, 0, "准备");
}

GAME_TEST(2, auto_set_ready_when_other_players_have_eliminated_should_checkout)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "准备");
    ASSERT_PRI_MSG(OK, 0, "别人重新准备");
    // Player 1 eliminates themselves; player 0 is already ready -> checkout
    ASSERT_PRI_MSG(CHECKOUT, 1, "淘汰");
}

// --- Score and achievement tests ---

GAME_TEST(2, record_score)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "分数 5");
    ASSERT_PRI_MSG(CHECKOUT, 1, "结束子阶段");
    ASSERT_SCORE(5, 0);
}

GAME_TEST(2, get_achievement)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "成就 1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "结束子阶段");
    ASSERT_ACHIEVEMENTS(0, "普通成就");
    ASSERT_ACHIEVEMENTS(1);
}

} // namespace GAME_MODULE_NAME
} // namespace game
} // namespace lgtbot
