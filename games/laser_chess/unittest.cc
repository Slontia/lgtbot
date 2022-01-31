// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(2, win_line)
{
    bool first_hand = 0;
    ASSERT_PUB_MSG(OK, 0, "地图 天才");
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    if (const auto ret = PrivateRequest(0, "A2 逆"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CONTINUE, 1, "H5 逆");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CONTINUE, ret);
    }
    const std::array<std::string, 2> chess_coor{"A2", "H5"};
    for (uint32_t i = 0; i < 9; ++i) {
        ASSERT_PRI_MSG(CONTINUE, 1 - first_hand, chess_coor[1 - first_hand] + " 逆");
        ASSERT_PRI_MSG(CONTINUE, first_hand, chess_coor[first_hand] + " 逆");
    }
    ASSERT_PRI_MSG(CHECKOUT, 1 - first_hand, chess_coor[1 - first_hand] + " 逆");
    ASSERT_SCORE(0, 0);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
