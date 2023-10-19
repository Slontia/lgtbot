// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
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

GAME_TEST(2, all_do_nothing_will_break_game)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, all_pass_will_break_game)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "B1");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "C1");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "D1");
    ASSERT_TIMEOUT(CONTINUE);
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(18, 0);
}

GAME_TEST(2, forbid_public_request)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "B1");
}

GAME_TEST(2, forbid_repeated_set_or_pass_after_set)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "B1");
    ASSERT_PRI_MSG(FAILED, 0, "C1");
    ASSERT_PRI_MSG(FAILED, 0, "pass");
}

GAME_TEST(2, forbid_repeated_set_or_pass_after_pass)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(FAILED, 0, "C1");
    ASSERT_PRI_MSG(FAILED, 0, "pass");
}

GAME_TEST(2, leave_not_break_game)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "B1");
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_PRI_MSG(CONTINUE, 0, "C1");
    ASSERT_PRI_MSG(CONTINUE, 0, "D1");
    ASSERT_PRI_MSG(CHECKOUT, 0, "pass");

    ASSERT_SCORE(18, 0);
}

GAME_TEST(3, fill_all_space)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    ASSERT_PUB_MSG(OK, 0, "回合数 100");
    ASSERT_PUB_MSG(OK, 0, "尺寸 6");
    START_GAME();

    for (uint32_t i = 0; i < 11; ++i) {
        for (uint32_t player_id = 0; player_id < 3; ++player_id) {
            const std::string coor = std::string(1, 'A' + i % 6) + std::string(1, '0' + player_id + 3 * (i / 6));
            if (player_id == 2) {
                ASSERT_PRI_MSG(CONTINUE, player_id, coor);
            } else {
                ASSERT_PRI_MSG(OK, player_id, coor);
            }
        }
    }
    ASSERT_PRI_MSG(OK, 0, "F3");
    ASSERT_PRI_MSG(OK, 1, "F4");
    ASSERT_PRI_MSG(CHECKOUT, 2, "F5");

    ASSERT_SCORE(24, 18, 6);
}

GAME_TEST(2, set_invalid_place)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    ASSERT_PUB_MSG(OK, 0, "尺寸 6");
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "A7");
    ASSERT_PRI_MSG(FAILED, 0, "G1");
}

GAME_TEST(2, set_placed_place)
{
    ASSERT_PUB_MSG(OK, 0, "奖励格百分比 0");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "A1");
    ASSERT_PRI_MSG(CONTINUE, 1, "B2");
    ASSERT_PRI_MSG(FAILED, 0, "A1");
    ASSERT_PRI_MSG(FAILED, 0, "B2");
}

GAME_TEST(2, leave_then_timeout)
{
    START_GAME();
    ASSERT_LEAVE(CONTINUE, 0);
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(0, 0);
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
