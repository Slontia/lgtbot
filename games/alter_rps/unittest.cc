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

GAME_TEST(2, show_info)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(2, leave_default_choise)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();
    ASSERT_LEAVE(CONTINUE, 1);
    ASSERT_PRI_MSG(CHECKOUT, 0, "布1");
    ASSERT_PRI_MSG(CHECKOUT, 0, "布2");
    ASSERT_PRI_MSG(CHECKOUT, 0, "布1");

    ASSERT_PRI_MSG(CHECKOUT, 0, "布2");
    ASSERT_PRI_MSG(CHECKOUT, 0, "布4");
    ASSERT_PRI_MSG(CHECKOUT, 0, "布2");

    ASSERT_PRI_MSG(CHECKOUT, 0, "布4");
    ASSERT_PRI_MSG(CHECKOUT, 0, "剪刀4");
    ASSERT_PRI_MSG(CHECKOUT, 0, "布4");
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, forbid_public_choose)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "石头1");
}

GAME_TEST(2, forbid_public_alter)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(OK, 0, "剪刀1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "剪刀1");
    ASSERT_PUB_MSG(FAILED, 0, "石头1");
}

GAME_TEST(2, repeat_choose)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(OK, 0, "石头2");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(OK, 0, "石头1");
}

GAME_TEST(2, not_exist_card)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "啥玩意");
    ASSERT_PRI_MSG(NOT_FOUND, 1, "石头1 哈哈哈");
    ASSERT_PRI_MSG(FAILED, 1, "石头3");
}

GAME_TEST(2, choose_choosed_card)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "石头1");
    ASSERT_PRI_MSG(FAILED, 0, "石头1"); // has choosed in this round
}

GAME_TEST(2, choose_used_card)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
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
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
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

#define ONE_STATE(p1_card, p2_card) \
    ASSERT_PRI_MSG(OK, 0, p1_card); \
    ASSERT_PRI_MSG(CHECKOUT, 1, p2_card); \

#define ONE_ROUND(p1_card, p2_card) \
    ONE_STATE(p1_card, p2_card) \
    ONE_STATE(p1_final_card, p2_final_card) \
    ONE_STATE(p1_card, p2_card) \

#define ONE_ROUND_FIXED_LEFT_HAND(p1_card, p2_card) \
    ONE_STATE(p1_card, p2_card) \
    ONE_STATE(p1_card, p2_card) \

GAME_TEST(2, all_win_by_type)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
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
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
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
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();

    LAST_ROUND("布1", "布2");
    ONE_ROUND("剪刀1", "石头1");
    ONE_ROUND("剪刀2", "石头2");
    ONE_ROUND("剪刀4", "石头4");

    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, half_win_interrupt)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
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
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
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

GAME_TEST(2, half_win_with_blank_card)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头10 剪刀10 布10 空白2 空白5 空白8");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 关闭");
    START_GAME();

    LAST_ROUND("石头1", "布1");
    ONE_ROUND("剪刀1", "空白2");
    ONE_ROUND("空白5", "石头10");
    ONE_ROUND("空白2", "空白8");

    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, all_win_by_type_fixed_left_hand)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀1 布1 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 开启");
    START_GAME();

    ONE_STATE("剪刀1", "石头1");
    ONE_ROUND_FIXED_LEFT_HAND("剪刀2", "石头2");
    ONE_ROUND_FIXED_LEFT_HAND("剪刀4", "剪刀4");

    ONE_ROUND_FIXED_LEFT_HAND("石头1", "布2");
    ONE_ROUND_FIXED_LEFT_HAND("石头2", "布1");
    ONE_ROUND_FIXED_LEFT_HAND("石头4", "石头4");

    ONE_ROUND_FIXED_LEFT_HAND("布1", "剪刀1");
    ONE_ROUND_FIXED_LEFT_HAND("布2", "剪刀2");
    ONE_ROUND_FIXED_LEFT_HAND("布4", "布4");

    ASSERT_SCORE(0, 9);
}

GAME_TEST(2, negative_point_card)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头- 剪刀- 布- 石头2 剪刀2 布2 石头4 剪刀4 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 开启");
    START_GAME();

    ONE_STATE("剪刀-", "剪刀2"); // 0, -2
    ONE_ROUND_FIXED_LEFT_HAND("剪刀2", "剪刀4"); // 0, 2
    ONE_ROUND_FIXED_LEFT_HAND("剪刀4", "布-"); // -4, 0

    ONE_ROUND_FIXED_LEFT_HAND("布-", "布2"); // 0, -2
    ONE_ROUND_FIXED_LEFT_HAND("布2", "布4"); // 0, 2
    ONE_ROUND_FIXED_LEFT_HAND("布4", "石头-"); // -4, 0

    ONE_ROUND_FIXED_LEFT_HAND("石头-", "石头2"); // 0, -2
    ONE_ROUND_FIXED_LEFT_HAND("石头2", "石头4"); // 0, 2
    ONE_ROUND_FIXED_LEFT_HAND("石头4", "剪刀-"); // -4, 0

    ASSERT_SCORE(-12, 0);
}

GAME_TEST(2, player_has_more_star_win)
{
    ASSERT_PRI_MSG(OK, 0, "手牌 石头1 剪刀7 布1 石头2 剪刀9 布2 石头4 剪刀10 布4");
    ASSERT_PRI_MSG(OK, 0, "固定左拳 开启");
    ASSERT_PRI_MSG(OK, 0, "星卡 布1");
    START_GAME();

    ONE_STATE("剪刀7", "石头1"); // 0, 7
    ONE_ROUND_FIXED_LEFT_HAND("剪刀9", "石头2"); // 0, 9
    ONE_ROUND_FIXED_LEFT_HAND("剪刀10", "剪刀10"); // 0, 10

    ONE_ROUND_FIXED_LEFT_HAND("布2", "布1"); // 1★, 0
    ONE_ROUND_FIXED_LEFT_HAND("布1", "石头4"); // 4, 0

    ONE_ROUND_FIXED_LEFT_HAND("布4", "剪刀9"); // 0, 4
    ONE_ROUND_FIXED_LEFT_HAND("石头1", "布2"); // 0, 1
    ONE_ROUND_FIXED_LEFT_HAND("石头4", "剪刀7"); // 7, 0
    ONE_ROUND_FIXED_LEFT_HAND("石头2", "布4"); // 0, 2

    ASSERT_SCORE(1, 0);
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
