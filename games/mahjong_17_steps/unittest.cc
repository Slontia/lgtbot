#include "game_framework/unittest_base.h"

GAME_TEST(1, cannot_one_player)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(4, four_player_must_without_dora)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 1");
    ASSERT_FALSE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_TRUE(StartGame());
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
    ASSERT_PUB_MSG(FAILED, 0, "添加 224455p1133577z");
}

GAME_TEST(2, repeat_riichi)
{
    ASSERT_PUB_MSG(OK, 0, "宝牌 0");
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_PRI_MSG(OK, 0, "添加 224455p1133577z");
    ASSERT_PRI_MSG(OK, 0, "立直");
    ASSERT_PRI_MSG(FAILED, 0, "立直");
    ASSERT_PRI_MSG(FAILED, 0, "移除 2z");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
