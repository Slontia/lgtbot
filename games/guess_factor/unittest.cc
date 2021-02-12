#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
  ASSERT_FALSE(StartGame());
}

GAME_TEST(2, not_eliminate_continue)
{
  PUB_MSG(OK, 0, "淘汰回合 1");
  START_GAME();
  PRI_MSG(OK, 0, "1");
  PUB_MSG(FAILED, 1, "1");
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
