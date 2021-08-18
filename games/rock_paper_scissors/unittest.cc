#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
  ASSERT_FALSE(StartGame());
}

GAME_TEST(2, forbid_public_guess)
{
  START_GAME();
  ASSERT_PUB_MSG(FAILED, 0, "石头");
}

GAME_TEST(2, win_1)
{
  ASSERT_PUB_MSG(OK, 0, "胜利局数 3");
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "石头");
  ASSERT_PRI_MSG(CHECKOUT, 1, "剪刀");
  ASSERT_PRI_MSG(OK, 0, "剪刀");
  ASSERT_PRI_MSG(CHECKOUT, 1, "布");
  ASSERT_PRI_MSG(OK, 1, "布");
  ASSERT_PRI_MSG(CHECKOUT, 0, "布");
  ASSERT_PRI_MSG(OK, 0, "剪刀");
  ASSERT_PRI_MSG(OK, 0, "布");
  ASSERT_PRI_MSG(CHECKOUT, 1, "石头");
  ASSERT_SCORE(1, 0);
}

GAME_TEST(2, win_2)
{
    ASSERT_PUB_MSG(OK, 0, "胜利局数 1");
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "石头");
    ASSERT_PRI_MSG(CHECKOUT, 1, "布");
    ASSERT_SCORE(0, 1);
}

GAME_TEST(2, timeout)
{
  ASSERT_PUB_MSG(OK, 0, "胜利局数 3");
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "石头");
  Timeout();
  ASSERT_PRI_MSG(OK, 0, "剪刀");
  Timeout();
  ASSERT_PRI_MSG(OK, 0, "布");
  Timeout();
  ASSERT_SCORE(1, 0);
}

GAME_TEST(2, all_timeout)
{
  START_GAME();
  Timeout();
  ASSERT_SCORE(0, 0);
}

GAME_TEST(2, leave)
{
  START_GAME();
  ASSERT_LEAVE(CHECKOUT, 1);
  ASSERT_SCORE(1, 0);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
