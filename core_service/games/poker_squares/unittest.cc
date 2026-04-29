// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

GAME_TEST(1, invalid_input)
{
    START_GAME();
    ASSERT_PRI_MSG(FAILED, 0, "0");
    ASSERT_PRI_MSG(FAILED, 0, "我");
    ASSERT_PRI_MSG(FAILED, 0, "-");
    ASSERT_PRI_MSG(FAILED, 0, "z");
}

GAME_TEST(1, do_nothing)
{
    // c2 d2 h2 s2 d3
    // s3 c4 d4 h4 s4
    // c5 d5 h5 s5 c6
    // d6 h6 s6 c7 d7
    // h7 s7 c8 d8 h8

    ASSERT_PUB_MSG(OK, 0, "洗牌 关闭");
    START_GAME();

    for (int32_t i = 0; i < 24; ++i) {
        ASSERT_TIMEOUT(CONTINUE);
    }
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(5 * 16 * 1 /*row 1 FOUR_OF_A_KIND*/ + 5 * 16 * 1 /*row 2 FOUR_OF_A_KIND*/ + 5 * 16 * 1 /*row 3 FOUR_OF_A_KIND*/ +
            7 * 10 * 1 /*row 4 FULL_HOUSE*/ + 10 * 10 * 1 /*row 5 FULL_HOUSE*/ + 6 * 12 * 2 /*slash STRAIGHT*/);
}

GAME_TEST(1, set_skip_next_last)
{
    // c2 d2 h2 s7 d8
    // s2 c3 d3 h3 s3
    // c4 d4 h4 s4 c5
    // d5 h5 s5 c6 d6
    // h6 s6 c7 d7 h7

    ASSERT_PUB_MSG(OK, 0, "洗牌 关闭");
    START_GAME();

    for (int32_t i = 0; i < 3; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, std::string(1, static_cast<char>('a' + i)));
    }
    for (int32_t i = 0; i < 20; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 0, std::string(1, static_cast<char>('f' + i)));
    }
    ASSERT_PRI_MSG(CONTINUE, 0, "d");
    ASSERT_PRI_MSG(CHECKOUT, 0, "e");

    ASSERT_SCORE(7 * 6 * 1 /*row 1 THREE_OF_A_KIND*/ + 5 * 16 * 1 /*row 2 FOUR_OF_A_KIND*/ + 5 * 16 * 1 /*row 3 FOUR_OF_A_KIND*/ +
            5 * 10 * 1 /*row 4 FULL_HOUSE*/ + 8 * 10 * 1 /*row 5 FULL_HOUSE*/ + 5 * 1 * 1 /*col 1 ONE_PAIR*/ +
            5 * 12 * 1 /*col 2 STRAIGHT*/ + 7 * 1 * 1 /*row 4 ONE_PAIR*/ + 100 /*5 patterns*/);
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

// 最后一回合全员 skip next
