// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(4, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, agree_team_up)
{
    START_GAME();
    ASSERT_PRI_MSG(CHECKOUT, 0, "1 0");
    ASSERT_PRI_MSG(OK, 0, "同意");
    ASSERT_PRI_MSG(OK, 1, "同意");
    ASSERT_PRI_MSG(OK, 2, "同意");
    ASSERT_PRI_MSG(OK, 3, "反对");
    ASSERT_PRI_MSG(CHECKOUT, 4, "反对");
    ASSERT_PRI_MSG(OK, 0, "成功");
}

GAME_TEST(5, only_member_can_act)
{
    START_GAME();
    ASSERT_PRI_MSG(CHECKOUT, 0, "1 0");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PRI_MSG(FAILED, 3, "成功");
    ASSERT_PRI_MSG(FAILED, 3, "失败");
    CHECK_PRI_MSG(OK, 0, "成功") || CHECK_PRI_MSG(OK, 0, "失败");
}

GAME_TEST(5, leader_cannot_hold_sword)
{
    ASSERT_PRI_MSG(OK, 0, "王者之剑 开启");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "0 1");
}

GAME_TEST(7, select_witch)
{
    START_GAME();

    ASSERT_PRI_MSG(CHECKOUT, 0, "1 0");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_PRI_MSG(CHECKOUT, 1, "3 2 1");
    ASSERT_TIMEOUT(CHECKOUT);
    for (uint32_t pid = 0; pid < 7; ++pid) {
        CHECK_PRI_MSG(OK, pid, "失败") || CHECK_PRI_MSG(OK, pid, "成功");
    }
    ASSERT_PRI_MSG(FAILED, 5, "0"); // 5 is not witch
    ASSERT_PRI_MSG(CHECKOUT, 6, "0");

    ASSERT_PRI_MSG(CHECKOUT, 2, "6 5 4");
    ASSERT_TIMEOUT(CHECKOUT);
    for (uint32_t pid = 0; pid < 7; ++pid) {
        CHECK_PRI_MSG(OK, pid, "失败") || CHECK_PRI_MSG(OK, pid, "成功");
    }
    ASSERT_PRI_MSG(FAILED, 5, "0"); // 5 is not witch
    ASSERT_PRI_MSG(FAILED, 0, "6"); // 6 has been witch
    ASSERT_PRI_MSG(CHECKOUT, 0, "1"); // detect stage
}

GAME_TEST(5, only_assassin_can_assassin)
{
    ASSERT_PRI_MSG(OK, 0, "测试模式 开启");
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "刺杀 0");
}

GAME_TEST(5, assassin_directly_failed)
{
    ASSERT_PRI_MSG(OK, 0, "测试模式 开启");
    START_GAME();

    ASSERT_PRI_MSG(CHECKOUT, 1, "刺杀 0");

    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, assassin_directly_succeed)
{
    ASSERT_PRI_MSG(OK, 0, "测试模式 开启");
    START_GAME();

    ASSERT_PRI_MSG(CHECKOUT, 1, "刺杀 2");

    ASSERT_SCORE(1, 1, 0, 0, 0);
}

GAME_TEST(5, assassin_last_round_failed)
{
    ASSERT_PRI_MSG(OK, 0, "测试模式 开启");
    ASSERT_PRI_MSG(OK, 0, "王者之剑 关闭");
    START_GAME();

    // round 1
    ASSERT_PRI_MSG(CHECKOUT, 0, "0 1");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 1, "成功");

    // round 2
    ASSERT_PRI_MSG(CHECKOUT, 1, "0 1 2");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "成功");
    ASSERT_PRI_MSG(OK, 1, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 2, "成功");

    // round 3
    ASSERT_PRI_MSG(CHECKOUT, 2, "0 1");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 1, "成功");

    // assassin
    ASSERT_PRI_MSG(CHECKOUT, 1, "刺杀 0");

    ASSERT_SCORE(0, 0, 1, 1, 1);
}

GAME_TEST(5, assassin_last_round_succeed)
{
    ASSERT_PRI_MSG(OK, 0, "测试模式 开启");
    ASSERT_PRI_MSG(OK, 0, "王者之剑 关闭");
    START_GAME();

    // round 1
    ASSERT_PRI_MSG(CHECKOUT, 0, "0 1");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 1, "成功");

    // round 2
    ASSERT_PRI_MSG(CHECKOUT, 1, "0 1 2");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "成功");
    ASSERT_PRI_MSG(OK, 1, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 2, "成功");

    // round 3
    ASSERT_PRI_MSG(CHECKOUT, 2, "0 1");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 1, "成功");

    // assassin
    ASSERT_PRI_MSG(CHECKOUT, 1, "刺杀 2");

    ASSERT_SCORE(1, 1, 0, 0, 0);
}

GAME_TEST(5, three_tasks_failed_need_not_assassin)
{
    ASSERT_PRI_MSG(OK, 0, "测试模式 开启");
    ASSERT_PRI_MSG(OK, 0, "王者之剑 关闭");
    START_GAME();

    // round 1
    ASSERT_PRI_MSG(CHECKOUT, 0, "0 1");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "失败");
    ASSERT_PRI_MSG(CHECKOUT, 1, "成功");

    // round 2
    ASSERT_PRI_MSG(CHECKOUT, 1, "0 1 2");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "失败");
    ASSERT_PRI_MSG(OK, 1, "成功");
    ASSERT_PRI_MSG(CHECKOUT, 2, "成功");

    // round 3
    ASSERT_PRI_MSG(CHECKOUT, 2, "0 1");
    ASSERT_TIMEOUT(CHECKOUT); // all agree
    ASSERT_PRI_MSG(OK, 0, "失败");
    ASSERT_PRI_MSG(CHECKOUT, 1, "成功");

    ASSERT_SCORE(1, 1, 0, 0, 0);
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
