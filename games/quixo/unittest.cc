#include "game_framework/unittest_base.h"

GAME_TEST(2, win_line)
{
    bool first_hand = 0;
    ASSERT_PUB_MSG(OK, 0, "模式 简单");
    ASSERT_TRUE(StartGame());
    if (const auto ret = PrivateRequest(0, "4 0"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CONTINUE, 1, "4 0");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CONTINUE, ret);
    }
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CHECKOUT, first_hand, "4 0");
    if (first_hand == 0) {
        ASSERT_SCORE(1, 0);
    } else {
        ASSERT_SCORE(0, 1);
    }
}

GAME_TEST(2, lose_opp_line)
{
    bool first_hand = 0;
    ASSERT_PUB_MSG(OK, 0, "模式 简单");
    ASSERT_TRUE(StartGame());
    if (const auto ret = PrivateRequest(0, "4 0"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CONTINUE, 1, "4 0");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CONTINUE, ret);
    }
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "15 5");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "15 5");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "15 5");
    ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "4 8");
    if (first_hand == 0) {
        ASSERT_SCORE(1, 0);
    } else {
        ASSERT_SCORE(0, 1);
    }
}

GAME_TEST(2, lose_opp_line_both)
{
    bool first_hand = 0;
    ASSERT_PUB_MSG(OK, 0, "模式 简单");
    ASSERT_TRUE(StartGame());
    if (const auto ret = PrivateRequest(0, "4 0"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CONTINUE, 1, "4 0");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CONTINUE, ret);
    }
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "4 0");
    ASSERT_PUB_MSG(CHECKOUT, first_hand, "15 0");
    if (first_hand == 0) {
        ASSERT_SCORE(0, 1);
    } else {
        ASSERT_SCORE(1, 0);
    }
}

GAME_TEST(2, achieve_max_round)
{
    bool first_hand = 0;
    ASSERT_PUB_MSG(OK, 0, "模式 简单");
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    if (const auto ret = PrivateRequest(0, "4 0"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CONTINUE, 1, "4 0");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CONTINUE, ret);
    }
    for (uint32_t i = 0; i < 4; ++i) {
        ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
        ASSERT_PUB_MSG(CONTINUE, first_hand, "0 4");
        ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "15 5");
        ASSERT_PUB_MSG(CONTINUE, first_hand, "4 0");
    }
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(CONTINUE, first_hand, "0 4");
    ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "15 5");
    if (first_hand == 0) {
        ASSERT_SCORE(0, 1);
    } else {
        ASSERT_SCORE(1, 0);
    }
}

GAME_TEST(2, hard_cannot_move_other_type)
{
    bool first_hand = 0;
    ASSERT_PUB_MSG(OK, 0, "模式 困难");
    ASSERT_PUB_MSG(OK, 0, "回合数 10");
    ASSERT_TRUE(StartGame());
    if (const auto ret = PrivateRequest(0, "4 0"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CONTINUE, 1, "4 0");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CONTINUE, ret);
    }
    ASSERT_PUB_MSG(CONTINUE, 1 - first_hand, "5 15");
    ASSERT_PUB_MSG(FAILED, first_hand, "0 4");
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
