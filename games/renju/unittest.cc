// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, cannot_one_player)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, first_round_cannot_set_center)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 0, "H7");
}

GAME_TEST(2, first_round_cannot_set_center_timeout)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 0, "H7");
    ASSERT_PRI_MSG(OK, 1, "H8");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "H7");
}

GAME_TEST(2, invalid_pos)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 0, "E7");
    ASSERT_PRI_MSG(FAILED, 0, "E");
    ASSERT_PRI_MSG(FAILED, 0, "14");
    ASSERT_PRI_MSG(FAILED, 0, "E20");
}

GAME_TEST(2, extend_board)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(FAILED, 0, "E7");
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(OK, 0, "E7");
}

GAME_TEST(2, extend_limit)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(OK, 0, "E8");
    ASSERT_PRI_MSG(CONTINUE, 1, "E7");
    ASSERT_PRI_MSG(OK, 0, "D7");
    ASSERT_PRI_MSG(CONTINUE, 1, "D8");
    ASSERT_PRI_MSG(OK, 0, "C8");
    ASSERT_PRI_MSG(CONTINUE, 1, "C7");
    ASSERT_PRI_MSG(OK, 0, "B7");
    ASSERT_PRI_MSG(CONTINUE, 1, "B8");
    ASSERT_PRI_MSG(OK, 0, "A8");
    ASSERT_PRI_MSG(CONTINUE, 1, "A7");
    ASSERT_PRI_MSG(FAILED, 0, "P7");
}

GAME_TEST(2, renju_win)
{
    ASSERT_PRI_MSG(OK, 0, "pass上限 0");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
    ASSERT_PRI_MSG(OK, 0, "G7");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
    ASSERT_PRI_MSG(OK, 0, "H7");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
    ASSERT_PRI_MSG(OK, 0, "I7");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
    ASSERT_PRI_MSG(OK, 0, "J7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "pass");
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, double_win)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(OK, 0, "G7");
    ASSERT_PRI_MSG(CONTINUE, 1, "G8");
    ASSERT_PRI_MSG(OK, 0, "H7");
    ASSERT_PRI_MSG(CONTINUE, 1, "H8");
    ASSERT_PRI_MSG(OK, 0, "I7");
    ASSERT_PRI_MSG(CONTINUE, 1, "I8");
    ASSERT_PRI_MSG(OK, 0, "J7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "J8");
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, round_limit)
{
    ASSERT_PUB_MSG(OK, 0, "回合上限 1");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "F8");
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, crash_not_count_before_extend)
{
    ASSERT_PUB_MSG(OK, 0, "碰撞上限 1");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F7");
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
}

GAME_TEST(2, crash_limit)
{
    ASSERT_PUB_MSG(OK, 0, "碰撞上限 1");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(OK, 0, "E7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "E7");
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, crash_limit_for_pass)
{
    ASSERT_PUB_MSG(OK, 0, "碰撞上限 1");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 1, "pass");
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, continuous_pass_also_crash)
{
    ASSERT_PUB_MSG(OK, 0, "碰撞上限 2");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 1, "pass");
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, cannot_set_after_crash)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F7");
    ASSERT_PRI_MSG(FAILED, 0, "F7");
}

GAME_TEST(2, cannot_set_after_set)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(FAILED, 0, "F7");
    ASSERT_PRI_MSG(FAILED, 0, "F8");
}

GAME_TEST(2, full_board)
{
    ASSERT_PUB_MSG(OK, 0, "碰撞上限 0");
    ASSERT_TRUE(StartGame());
    for (char a = 'F'; a <= 'J'; ++a) {
        for (char b = '5'; b <= '9'; ++b) {
            if (a == 'F' && b == '7') {
                continue;
            }
            std::string pos = {a, b};
            ASSERT_PRI_MSG(OK, 0, pos.c_str());
            ASSERT_PRI_MSG(CONTINUE, 1, pos.c_str());
        }
    }
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CHECKOUT, 1, "F7");
}

GAME_TEST(2, set_not_ready)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7 试下");
    ASSERT_PRI_MSG(OK, 1, "F8 试下");
    ASSERT_PRI_MSG(OK, 0, "F9 试下");
    ASSERT_PRI_MSG(OK, 1, "落子");
    ASSERT_PRI_MSG(CONTINUE, 0, "落子");
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(FAILED, 0, "F9");
}

GAME_TEST(2, cannot_ready_directly)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_PRI_MSG(CONTINUE, 1, "F8");
    ASSERT_PRI_MSG(FAILED, 0, "落子");
}

GAME_TEST(2, timeout_clear_both_ready)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "F7");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 1, "F8");
}

GAME_TEST(2, achieve_pass_limit)
{
    ASSERT_PRI_MSG(OK, 0, "pass上限 1");
    ASSERT_PRI_MSG(OK, 0, "模式 经典");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 1, "F7");
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, achieve_pass_limit_in_competitive_mode)
{
    ASSERT_PRI_MSG(OK, 0, "pass上限 1");
    ASSERT_PRI_MSG(OK, 0, "模式 竞技");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 1, "F7");
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, timeout_is_pass)
{
    ASSERT_PRI_MSG(OK, 0, "pass上限 1");
    ASSERT_PRI_MSG(OK, 0, "模式 竞技");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 1, "F7");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, pass_after_pass_crash_not_count)
{
    ASSERT_PRI_MSG(OK, 0, "pass上限 3");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "pass");
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CONTINUE, 1, "F7");
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
