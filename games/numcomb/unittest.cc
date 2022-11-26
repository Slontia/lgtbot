// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(1, one_player_ok)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(CHECKOUT, 0, "1");
    ASSERT_PUB_MSG(CHECKOUT, 0, "2");
}

GAME_TEST(1, one_finish)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 19");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "跳过非癞子 38");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 1; i <= 19; ++i) {
        ASSERT_PUB_MSG(CHECKOUT, 0, std::to_string(i).c_str());
    }
    ASSERT_SCORE(0);
}

GAME_TEST(2, two_finish)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 19");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "跳过非癞子 38");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 1; i <= 19; ++i) {
        ASSERT_PUB_MSG(OK, 0, std::to_string(i).c_str());
        ASSERT_PUB_MSG(CHECKOUT, 1, std::to_string(i).c_str());
    }
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, repeat_selection)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "1");
    ASSERT_PUB_MSG(FAILED, 0, "2");
}

GAME_TEST(2, timeout_eliminate)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "0");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(FAILED, 1, "0"); // is seq filled
    ASSERT_PUB_MSG(OK, 1, "1");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(FAILED, 0, "1"); // is seq filled
    ASSERT_PUB_MSG(OK, 0, "2");
}

GAME_TEST(2, timeout_hook)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "0");
    ASSERT_TIMEOUT(CHECKOUT); // player 1 seq fill 0
    ASSERT_PUB_MSG(CHECKOUT, 0, "1"); // player 1 is hooked, seq fill 1
    ASSERT_PUB_MSG(FAILED, 1, "0"); // is seq filled
    ASSERT_PUB_MSG(FAILED, 1, "1"); // is seq filled
    ASSERT_PUB_MSG(OK, 1, "2");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
