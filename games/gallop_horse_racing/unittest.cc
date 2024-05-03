// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(4, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, simple_test)
{
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "9");
    ASSERT_PRI_MSG(OK, 1, "10");
    ASSERT_PRI_MSG(OK, 2, "9");
    ASSERT_PRI_MSG(OK, 3, "10");
    ASSERT_PRI_MSG(CHECKOUT, 4, "10");

    ASSERT_PRI_MSG(OK, 0, "10");
    ASSERT_PRI_MSG(OK, 1, "7");
    ASSERT_PRI_MSG(OK, 2, "6");
    ASSERT_PRI_MSG(OK, 3, "7");
    ASSERT_PRI_MSG(CHECKOUT, 4, "7");

    ASSERT_PRI_MSG(OK, 0, "6");
    ASSERT_PRI_MSG(OK, 1, "6");
    ASSERT_PRI_MSG(OK, 2, "10");
    ASSERT_PRI_MSG(OK, 3, "7");
    ASSERT_PRI_MSG(CHECKOUT, 4, "6");

    ASSERT_PRI_MSG(OK, 0, "12");
    ASSERT_PRI_MSG(OK, 1, "7");
    ASSERT_PRI_MSG(OK, 2, "7");
    ASSERT_PRI_MSG(OK, 3, "7");
    ASSERT_PRI_MSG(CHECKOUT, 4, "7");

    ASSERT_PRI_MSG(OK, 0, "11");
    ASSERT_PRI_MSG(OK, 1, "14");
    ASSERT_PRI_MSG(OK, 2, "13");
    ASSERT_PRI_MSG(OK, 3, "11");
    ASSERT_PRI_MSG(CHECKOUT, 4, "13");

    ASSERT_PRI_MSG(OK, 0, "11");
    ASSERT_PRI_MSG(OK, 1, "13");
    ASSERT_PRI_MSG(OK, 2, "13");
    ASSERT_PRI_MSG(OK, 3, "11");
    ASSERT_PRI_MSG(CHECKOUT, 4, "14");

    ASSERT_PRI_MSG(OK, 0, "15");
    ASSERT_PRI_MSG(OK, 1, "13");
    ASSERT_PRI_MSG(OK, 2, "13");
    ASSERT_PRI_MSG(OK, 3, "15");
    ASSERT_PRI_MSG(CHECKOUT, 4, "13");

    ASSERT_PRI_MSG(OK, 0, "13");
    ASSERT_PRI_MSG(OK, 1, "15");
    ASSERT_PRI_MSG(OK, 2, "16");
    ASSERT_PRI_MSG(OK, 3, "13");
    ASSERT_PRI_MSG(CHECKOUT, 4, "16");

    ASSERT_PRI_MSG(OK, 0, "15");
    ASSERT_PRI_MSG(OK, 1, "16");
    ASSERT_PRI_MSG(OK, 2, "14");
    ASSERT_PRI_MSG(OK, 3, "14");
    ASSERT_PRI_MSG(CHECKOUT, 4, "14");

    ASSERT_SCORE(20, 5, 5, -20, -10);
}

GAME_TEST(5, allsame_test)
{
    ASSERT_PUB_MSG(OK, 0, "目标 30");
    ASSERT_PUB_MSG(OK, 0, "上限 20");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "19");
    ASSERT_PRI_MSG(OK, 1, "19");
    ASSERT_PRI_MSG(OK, 2, "19");
    ASSERT_PRI_MSG(OK, 3, "19");
    ASSERT_PRI_MSG(CHECKOUT, 4, "19");

    ASSERT_PRI_MSG(FAILED, 0, "21");
    ASSERT_PRI_MSG(OK, 0, "11");
    ASSERT_PRI_MSG(OK, 1, "11");
    ASSERT_PRI_MSG(OK, 2, "1");
    ASSERT_PRI_MSG(OK, 3, "1");
    ASSERT_PRI_MSG(CHECKOUT, 4, "11");

    ASSERT_PRI_MSG(FAILED, 2, "21");
    ASSERT_PRI_MSG(OK, 2, "1");
    ASSERT_PRI_MSG(CHECKOUT, 3, "1");

    ASSERT_PRI_MSG(FAILED, 2, "21");
    ASSERT_PRI_MSG(OK, 2, "19");
    ASSERT_PRI_MSG(CHECKOUT, 3, "20");

    ASSERT_SCORE(10, 10, -20, -10, 10);
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
