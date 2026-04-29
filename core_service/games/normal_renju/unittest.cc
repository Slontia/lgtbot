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

GAME_TEST(2, simple_test)
{
    START_GAME();
    PrivateRequest(1, "pass"); // 0 first, 1 second

    ASSERT_PUB_MSG(CONTINUE, 0, "H7 I7 H8");
    ASSERT_PUB_MSG(CONTINUE, 1, "I8 H9");
    ASSERT_PUB_MSG(CONTINUE, 0, "pass");
    ASSERT_PUB_MSG(CONTINUE, 1, "I10");
    ASSERT_PUB_MSG(CONTINUE, 0, "H10");
    ASSERT_PUB_MSG(CONTINUE, 1, "I11");
    ASSERT_PUB_MSG(CHECKOUT, 0, "H6");

    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, both_give_up_the_init_set)
{
    START_GAME();

    PrivateRequest(0, "pass");
    if (const auto ret = PrivateRequest(1, "pass"); ret == StageErrCode::CONTINUE) {
        ASSERT_PUB_MSG(CHECKOUT, 0, "pass");
    }

    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, init_must_choose_3_positions)
{
    START_GAME();
    PrivateRequest(1, "pass"); // 0 first, 1 second

    ASSERT_PUB_MSG(FAILED, 0, "H7 I7");
    ASSERT_PUB_MSG(FAILED, 0, "H7");
}

GAME_TEST(2, swap_1_must_choose_2_positions)
{
    START_GAME();
    PrivateRequest(1, "pass"); // 0 first, 1 second

    ASSERT_PUB_MSG(CONTINUE, 0, "H7 I7 H8");
    ASSERT_PUB_MSG(CONTINUE, 1, "I8 H9");
    ASSERT_PUB_MSG(FAILED, 0, "I8");
    ASSERT_PUB_MSG(FAILED, 0, "H9");
}

GAME_TEST(2, overline_not_win)
{
    START_GAME();
    PrivateRequest(1, "pass"); // 0 first, 1 second

    ASSERT_PUB_MSG(CONTINUE, 0, "H7 I7 H8");
    ASSERT_PUB_MSG(CONTINUE, 1, "I8 H9");
    ASSERT_PUB_MSG(CONTINUE, 0, "pass");
    ASSERT_PUB_MSG(CONTINUE, 1, "pass");
    ASSERT_PUB_MSG(CONTINUE, 0, "H10");
    ASSERT_PUB_MSG(CONTINUE, 1, "pass");
    ASSERT_PUB_MSG(CONTINUE, 0, "H5");
    ASSERT_PUB_MSG(CONTINUE, 1, "pass");
    ASSERT_PUB_MSG(CONTINUE, 0, "H6");
}

GAME_TEST(2, both_pass_will_end_game)
{
    START_GAME();
    PrivateRequest(1, "pass"); // 0 first, 1 second

    ASSERT_PUB_MSG(CONTINUE, 0, "H7 I7 H8");
    ASSERT_PUB_MSG(CONTINUE, 1, "I8 H9");
    ASSERT_PUB_MSG(CONTINUE, 0, "pass");
    ASSERT_PUB_MSG(CONTINUE, 1, "pass");
    ASSERT_PUB_MSG(CHECKOUT, 0, "pass");

    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, swap_1_use_white_cannot_set_two_point_later)
{
    START_GAME();
    PrivateRequest(1, "pass"); // 0 first, 1 second

    ASSERT_PUB_MSG(CONTINUE, 0, "H7 I7 H8");
    ASSERT_PUB_MSG(CONTINUE, 1, "I8");
    ASSERT_PUB_MSG(FAILED, 0, "H6 H5");
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
