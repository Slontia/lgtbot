#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
  ASSERT_FALSE(StartGame());
}

GAME_TEST(2, forbid_public_guess)
{
  START_GAME();
  bool first_hand = 0;
  for (int i = 0; i < 3; ++ i)
  {
    if (const auto ret = PRI_MSG(0, "1"); ret == EC_GAME_REQUEST_FAILED)
    {
      first_hand = 1;
      ASSERT_PRI_MSG(CHECKOUT, 1, "1");
    }
    else
    {
      first_hand = 0;
      ASSERT_EQ(ret, EC_GAME_REQUEST_CHECKOUT);
    }
    ASSERT_PUB_MSG(CHECKOUT, first_hand, "1");
    ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "相信");
  }
  if (first_hand == 0)
  {
    ASSERT_SCORE(0, 1);
  }
  else
  {
    ASSERT_SCORE(1, 0);
  }
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
