#include "game_framework/unittest_base.h"

GAME_TEST(1, invalid_option)
{
    ASSERT_PUB_MSG(OK, 0, "颜色 2");
    ASSERT_PUB_MSG(OK, 0, "点数 2");
    ASSERT_PUB_MSG(OK, 0, "副数 3");

    ASSERT_PUB_MSG(OK, 0, "回合数 13");
    ASSERT_FALSE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "回合数 12");
    ASSERT_TRUE(StartGame());
}

GAME_TEST(1, check_coor)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(FAILED, 0, "asf");
    ASSERT_PUB_MSG(FAILED, 0, "a");
    ASSERT_PUB_MSG(FAILED, 0, "f3");
    ASSERT_PUB_MSG(FAILED, 0, "A6");
    ASSERT_PUB_MSG(CHECKOUT, 0, "C2");
    ASSERT_PUB_MSG(CHECKOUT, 0, "c4");
    ASSERT_PUB_MSG(CHECKOUT, 0, "b3");
    ASSERT_PUB_MSG(CHECKOUT, 0, "D3");
}

GAME_TEST(2, one_finish)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_PUB_MSG(OK, 0, "pass");
        ASSERT_PUB_MSG(CHECKOUT, 1, "pass");
    }
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 0; i < 10; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(0, 0);
}

GAME_TEST(2, one_do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 2");
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C2"); // y5
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "C4"); // p4
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "C5"); // p1
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "B3"); // g2
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "D3"); // b1
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "C1"); // y3
    ASSERT_TIMEOUT(CHECKOUT);
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }
    ASSERT_SCORE(2, 0);
}

// y5 p4 p1 g2 b1 y3 b1 g1 p4 o5
GAME_TEST(2, achieve_score)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "副数 2");
    ASSERT_PUB_MSG(OK, 0, "胜利分数 2");
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C2"); // y5
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "C4"); // p4
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "C5"); // p1
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "B3"); // g2
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "D3"); // b1
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PUB_MSG(OK, 0, "C1"); // y3
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_SCORE(2, 0);
}

GAME_TEST(2, repeat_selection)
{
    ASSERT_TRUE(StartGame());
    ASSERT_PUB_MSG(OK, 0, "C2");
    ASSERT_PUB_MSG(FAILED, 0, "C4");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
