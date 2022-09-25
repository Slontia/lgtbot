// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(1, cannot_one_player)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(4, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    for (uint32_t game_idx = 0; game_idx < 4; ++game_idx) {
        ASSERT_TIMEOUT(CHECKOUT);
        for (uint32_t i = 0; i < 16; ++i) {
            ASSERT_TIMEOUT(CONTINUE);
        }
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(25000, 25000, 25000, 25000);
}

GAME_TEST(4, do_show_game_status)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(FAILED, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "赛况");
    ASSERT_TIMEOUT(CHECKOUT); // into kiri stage
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "赛况");
}

GAME_TEST(2, cannot_public_add_hand)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(FAILED, 0, "添加 456m456456s456p4m");
}

GAME_TEST(2, repeat_riichi)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "添加 456m456456s456p4m");
    ASSERT_PRI_MSG(OK, 0, "立直");
    ASSERT_PRI_MSG(FAILED, 0, "立直");
    ASSERT_PRI_MSG(FAILED, 0, "移除 2z");
}

GAME_TEST(3, hook_not_skip_when_others_computer)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_COMPUTER_ACT(OK, 1);
    ASSERT_COMPUTER_ACT(OK, 2);
    ASSERT_PRI_MSG(OK, 0, "添加 456m456456s456p4m");
    ASSERT_PRI_MSG(CHECKOUT, 0, "立直");
    for (uint32_t i = 0; i < 16; ++i) {
        ASSERT_COMPUTER_ACT(OK, 1);
        ASSERT_COMPUTER_ACT(OK, 2);
        ASSERT_TIMEOUT(CONTINUE);
    }
    ASSERT_COMPUTER_ACT(OK, 1);
    ASSERT_COMPUTER_ACT(OK, 2);
    ASSERT_TIMEOUT(CHECKOUT);
}

GAME_TEST(3, double_kiri_failed)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(FAILED, 0, "1z");
}

GAME_TEST(3, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "添加 456m456456s456p4m");
    ASSERT_PRI_MSG(OK, 1, "添加 778s1123356p112z");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PRI_MSG(OK, 0, "4z");
    ASSERT_PRI_MSG(OK, 1, "7m");
    ASSERT_TIMEOUT(CHECKOUT);
    for (uint32_t game_idx = 0; game_idx < 3; ++game_idx) {
        ASSERT_TIMEOUT(CHECKOUT);
        for (uint32_t i = 0; i < 16; ++i) {
            ASSERT_TIMEOUT(CONTINUE);
        }
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(33000, 17000, 25000);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
