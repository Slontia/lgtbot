#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
  ASSERT_FALSE(StartGame());
}

GAME_TEST(2, invalid_max_round)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 2");
  ASSERT_PRI_MSG(OK, 0, "最大回合 1");
  ASSERT_FALSE(StartGame());
  ASSERT_PRI_MSG(OK, 0, "最大回合 2");
  ASSERT_FALSE(StartGame());
  ASSERT_PRI_MSG(OK, 0, "最大回合 3");
  START_GAME();
}

GAME_TEST(2, forbid_public_guess)
{
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "1");
  ASSERT_PUB_MSG(FAILED, 1, "1");
}

GAME_TEST(3, forbid_eliminated_guess)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分差 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分数 1");
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "1");
  ASSERT_PRI_MSG(OK, 1, "1");
  ASSERT_PRI_MSG(CHECKOUT, 2, "3");
  ASSERT_PRI_MSG(FAILED, 2, "1");
}

GAME_TEST(2, one_round_score)
{
  ASSERT_PRI_MSG(OK, 0, "最大数字 3");
  START_GAME();
  ASSERT_PRI_MSG(NOT_FOUND, 0, "4");
  ASSERT_PRI_MSG(OK, 0, "3");
}

GAME_TEST(2, achieve_max_round)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "最大回合 3");
  START_GAME();
  for (uint32_t i = 0; i < 3; ++i)
  {
    ASSERT_PRI_MSG(OK, 0, "1");
    ASSERT_PRI_MSG(CHECKOUT, 1, "1");
  }
  ASSERT_SCORE(3, 3);
}

GAME_TEST(3, cal_score)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰间隔 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分差 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分数 1");
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "1");
  ASSERT_PRI_MSG(OK, 1, "2");
  ASSERT_PRI_MSG(CHECKOUT, 2, "3");
  ASSERT_PRI_MSG(OK, 1, "2");
  ASSERT_PRI_MSG(CHECKOUT, 2, "1");
  ASSERT_SCORE(1, 2, 4);
}

GAME_TEST(2, change_select)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分差 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分数 1");
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "1");
  ASSERT_PRI_MSG(OK, 0, "2");
  ASSERT_PRI_MSG(CHECKOUT, 1, "1");
  ASSERT_SCORE(0, 1);
}

GAME_TEST(2, timeout)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分差 1");
  ASSERT_PRI_MSG(OK, 0, "淘汰分数 1");
  START_GAME();
  ASSERT_PRI_MSG(OK, 0, "1");
  TIMEOUT();
  ASSERT_SCORE(1, 0);
}

GAME_TEST(2, achieve_max_round_by_both_pass)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "最大回合 3");
  START_GAME();
  for (uint32_t i = 0; i < 3; ++i)
  {
    ASSERT_PRI_MSG(OK, 0, "pass");
    ASSERT_PRI_MSG(CHECKOUT, 1, "pass");
  }
  ASSERT_SCORE(0, 0);
}

GAME_TEST(2, achieve_max_round_by_both_leave)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "最大回合 3");
  START_GAME();
  ASSERT_LEAVE(OK, 0);
  ASSERT_LEAVE(CHECKOUT, 1);
  ASSERT_SCORE(0, 0);
}

GAME_TEST(2, leave_but_win)
{
  ASSERT_PRI_MSG(OK, 0, "淘汰回合 1");
  ASSERT_PRI_MSG(OK, 0, "最大回合 7");
  ASSERT_PRI_MSG(OK, 0, "淘汰分差 14");
  ASSERT_PRI_MSG(OK, 0, "淘汰分数 1");
  ASSERT_PRI_MSG(OK, 0, "最大数字 53");
  START_GAME();
  // round 1
  ASSERT_PRI_MSG(OK, 0, "2");
  ASSERT_PRI_MSG(CHECKOUT, 1, "51"); // sum = 53
  // round 2
  ASSERT_PRI_MSG(OK, 0, "4");
  ASSERT_PRI_MSG(CHECKOUT, 1, "pass"); // sum = 57
  // round 3
  ASSERT_PRI_MSG(OK, 0, "8");
  ASSERT_PRI_MSG(CHECKOUT, 1, "pass"); // sum = 65
  // round 4
  ASSERT_LEAVE(OK, 0);
  ASSERT_PRI_MSG(CHECKOUT, 1, "53"); // sum = 118 - 2 = 116
  // round 5
  ASSERT_PRI_MSG(CHECKOUT, 1, "pass"); // sum = 116 - 4 = 112
  // round 6
  ASSERT_PRI_MSG(CHECKOUT, 1, "pass"); // sum = 116 - 4 = 104
  ASSERT_SCORE(14, 0);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
