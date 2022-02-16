// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, show_info)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(2, leave_default_choise)
{
    START_GAME();
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_PRI_MSG(CHECKOUT, 0, "石头1");
    ASSERT_PRI_MSG(CHECKOUT, 0, "石头2");
    ASSERT_PRI_MSG(CHECKOUT, 0, "石头1");

    ASSERT_PRI_MSG(CHECKOUT, 0, "石头2");
    ASSERT_PRI_MSG(CHECKOUT, 0, "石头4");
    ASSERT_PRI_MSG(CHECKOUT, 0, "石头2");

    ASSERT_PRI_MSG(CHECKOUT, 0, "石头4");
    ASSERT_PRI_MSG(CHECKOUT, 0, "剪刀4");
    ASSERT_PRI_MSG(CHECKOUT, 0, "石头4");
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, forbid_public_choose)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "石头1");
}

GAME_TEST(2, forbid_public_alter)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(OK, 0, "剪刀1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "剪刀1");
    ASSERT_PUB_MSG(FAILED, 0, "石头1");
}

GAME_TEST(2, repeat_choose)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(OK, 0, "石头2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(OK, 0, "石头1");
}

GAME_TEST(2, not_exist_card)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "啥玩意");
    ASSERT_PRI_MSG(NOT_FOUND, 1, "石头1 哈哈哈");
    ASSERT_PRI_MSG(FAILED, 1, "石头3");
}

GAME_TEST(2, choose_choosed_card)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(FAILED, 0, "石头1"); // has choosed in this round
}

GAME_TEST(2, choose_used_card)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(OK, 0, "剪刀1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "剪刀1");
    ASSERT_PRI_MSG(OK, 0, "剪刀1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "剪刀1");
    ASSERT_PRI_MSG(FAILED, 0, "剪刀1"); // has choosed in this round
    ASSERT_PRI_MSG(OK, 0, "石头1");
}

GAME_TEST(2, do_nothing)
{
    START_GAME();
    for (int i = 0; i < 3 * 8; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(0, 0);
}

std::array<const char*, 9> k_card_names {
    "石头1", "石头2", "石头4", "剪刀1", "剪刀2", "剪刀4", "布1", "布2", "布4",
};

#define LAST_ROUND(p1_card, p2_card) \
    const char* p1_final_card = p1_card; \
    const char* p2_final_card = p2_card; \

#define ONE_ROUND(p1_card, p2_card) \
    ASSERT_PRI_MSG(OK, 0, p1_card); \
    ASSERT_PRI_MSG(CHECKOUT, 1, p2_card); \
    ASSERT_PRI_MSG(OK, 0, p1_final_card); \
    ASSERT_PRI_MSG(CHECKOUT, 1, p2_final_card); \
    ASSERT_PRI_MSG(OK, 0, p1_card); \
    ASSERT_PRI_MSG(CHECKOUT, 1, p2_card);

GAME_TEST(2, all_win_by_type)
{
    START_GAME();

    LAST_ROUND("剪刀1", "石头1");
    ONE_ROUND("剪刀2", "石头2");
    ONE_ROUND("剪刀4", "剪刀4");

    ONE_ROUND("石头1", "布2");
    ONE_ROUND("石头2", "布1");
    ONE_ROUND("石头4", "石头4");

    ONE_ROUND("布1", "剪刀1");
    ONE_ROUND("布2", "剪刀2");
    ONE_ROUND("布4", "布4");

    ASSERT_SCORE(0, 9);
}

GAME_TEST(2, some_win_by_number)
{
    START_GAME();

    LAST_ROUND("剪刀1", "剪刀2");
    ONE_ROUND("剪刀2", "剪刀4");
    ONE_ROUND("剪刀4", "布1");

    ONE_ROUND("布1", "布2");
    ONE_ROUND("布2", "布4");
    ONE_ROUND("布4", "石头1");

    ONE_ROUND("石头1", "石头2");
    ONE_ROUND("石头2", "石头4");
    ONE_ROUND("石头4", "剪刀1");

    ASSERT_SCORE(3, 9);
}

GAME_TEST(2, half_win)
{
    START_GAME();

    LAST_ROUND("布1", "布2");
    ONE_ROUND("剪刀1", "石头1");
    ONE_ROUND("剪刀2", "石头2");
    ONE_ROUND("剪刀4", "石头4");

    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, half_win_interrupt)
{
    START_GAME();

    LAST_ROUND("布1", "石头4");
    ONE_ROUND("剪刀1", "布1");
    ONE_ROUND("剪刀2", "布2");
    ONE_ROUND("石头1", "布4"); // interrupt

    ONE_ROUND("剪刀4", "剪刀1");
    ONE_ROUND("石头2", "石头1");
    ONE_ROUND("石头4", "石头2");

    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, half_win_at_last)
{
    START_GAME();

    LAST_ROUND("剪刀1", "石头1");

    ONE_ROUND("剪刀4", "布4");
    ONE_ROUND("剪刀2", "剪刀2");

    ONE_ROUND("布4", "石头4");
    ONE_ROUND("布2", "布2");

    ONE_ROUND("石头4", "剪刀4");
    ONE_ROUND("石头2", "石头2");

    ONE_ROUND("石头1", "布1");
    ONE_ROUND("布1", "剪刀1");

    ASSERT_SCORE(0, 1);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
