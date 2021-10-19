#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(2, forbid_public_guess)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "1 1");
    ASSERT_PUB_MSG(FAILED, 1, "1 1");
}

GAME_TEST(2, loser_guess_doubt_failed)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();
    bool first_hand = 0;
    if (const auto ret = PrivateRequest(0, "1 1"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CHECKOUT, 1, "1 1");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CHECKOUT, ret);
    }
    ASSERT_PRI_MSG(FAILED, first_hand, "质疑");
    ASSERT_PRI_MSG(CHECKOUT, 1 - first_hand, "质疑");
    ASSERT_PRI_MSG(CHECKOUT, 1 - first_hand, "1 1");
}

GAME_TEST(2, loser_guess_believe_failed)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();
    bool first_hand = 0;
    if (const auto ret = PrivateRequest(0, "1 2"); ret == StageErrCode::FAILED) {
        first_hand = 1;
        ASSERT_PRI_MSG(CHECKOUT, 1, "1 2");
    } else {
        first_hand = 0;
        ASSERT_ERRCODE(StageErrCode::CHECKOUT, ret);
    }
    ASSERT_TIMEOUT(CHECKOUT); // default is believe
    ASSERT_PRI_MSG(CHECKOUT, 1 - first_hand, "1 1");
}

GAME_TEST(2, all_doubt_success_collect)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();
    bool first_hand = 0;
    for (int i = 0; i < 6; ++ i) {
        const std::string msg = std::to_string(i + 1) + " " + std::to_string((i + 1) % 6 + 1);
        if (const auto ret = PrivateRequest(0, msg.c_str()); ret == StageErrCode::FAILED) {
            first_hand = 1;
            ASSERT_PRI_MSG(CHECKOUT, 1, msg.c_str());
        } else {
            first_hand = 0;
            ASSERT_ERRCODE(StageErrCode::CHECKOUT, ret);
        }
        ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "质疑");
    }
    if (first_hand == 0) {
        ASSERT_SCORE(0, 1);
    } else {
        ASSERT_SCORE(1, 0);
    }
}

GAME_TEST(2, all_believe_success_collect)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();
    bool first_hand = 0;
    for (int i = 0; i < 6; ++ i) {
        const std::string msg = std::to_string(i + 1) + " " + std::to_string(i + 1);
        if (const auto ret = PrivateRequest(0, msg.c_str()); ret == StageErrCode::FAILED) {
            first_hand = 1;
            ASSERT_PRI_MSG(CHECKOUT, 1, msg.c_str());
        } else {
            first_hand = 0;
            ASSERT_ERRCODE(StageErrCode::CHECKOUT, ret);
        }
        ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "相信");
    }
    if (first_hand == 0) {
        ASSERT_SCORE(0, 1);
    } else {
        ASSERT_SCORE(1, 0);
    }
}
GAME_TEST(2, all_doubt_success)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();
    bool first_hand = 0;
    for (int i = 0; i < 3; ++ i) {
        if (const auto ret = PrivateRequest(0, "1 2"); ret == StageErrCode::FAILED) {
            first_hand = 1;
            ASSERT_PRI_MSG(CHECKOUT, 1, "1 2");
        } else {
            first_hand = 0;
            ASSERT_ERRCODE(StageErrCode::CHECKOUT, ret);
        }
        ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "质疑");
    }
    if (first_hand == 0) {
        ASSERT_SCORE(0, 1);
    } else {
        ASSERT_SCORE(1, 0);
    }
}

GAME_TEST(2, all_believe_success)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();
    bool first_hand = 0;
    for (int i = 0; i < 3; ++ i) {
        if (const auto ret = PrivateRequest(0, "1 1"); ret == StageErrCode::FAILED) {
            first_hand = 1;
            ASSERT_PRI_MSG(CHECKOUT, 1, "1 1");
        } else {
            first_hand = 0;
            ASSERT_ERRCODE(StageErrCode::CHECKOUT, ret);
        }
        ASSERT_PUB_MSG(CHECKOUT, 1 - first_hand, "相信");
    }
    if (first_hand == 0) {
        ASSERT_SCORE(0, 1);
    } else {
        ASSERT_SCORE(1, 0);
    }
}

GAME_TEST(2, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "数字种类 6");
    ASSERT_PUB_MSG(OK, 0, "失败数量 3");
    START_GAME();

    for (uint32_t i = 0; i < 3; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
        ASSERT_TIMEOUT(CHECKOUT);
    }

    ASSERT_FINISHED(true);
}

GAME_TEST(2, leave_1)
{
    START_GAME();
    ASSERT_LEAVE(CHECKOUT, 1);
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, leave_2)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_LEAVE(CHECKOUT, 1);
    ASSERT_SCORE(1, 0);
}

GAME_TEST(2, leave)
{
    START_GAME();
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_LEAVE(CHECKOUT, 1);
    ASSERT_SCORE(1, 0);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
