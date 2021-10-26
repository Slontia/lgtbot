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
    ASSERT_PUB_MSG(OK, 0, "跳过非癞子 38");
    ASSERT_TRUE(StartGame());
    for (uint32_t i = 1; i <= 19; ++i) {
        ASSERT_PUB_MSG(CHECKOUT, 0, std::to_string(i).c_str());
    }
    ASSERT_SCORE(0);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
