#include "game_framework/unittest_base.h"

GAME_TEST(1, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(5, bid_1)
{
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    START_GAME();

    // item 1
    ASSERT_PUB_MSG(FAILED, 0, "10");
    ASSERT_PRI_MSG(OK, 0, "10");
    ASSERT_PRI_MSG(OK, 1, "12");
    ASSERT_PRI_MSG(OK, 2, "11");
    ASSERT_PRI_MSG(OK, 3, "10");
    ASSERT_PRI_MSG(OK, 4, "11");
    ASSERT_PRI_MSG(FAILED, 3, "101");
    ASSERT_PRI_MSG(FAILED, 4, "0");
    ASSERT_TIMEOUT(CHECKOUT); // player 1 wins

    // item 2
    ASSERT_PRI_MSG(OK, 2, "100"); // coins not decrease in item 1
    ASSERT_TIMEOUT(CHECKOUT); // player 2 wins

    // item 3
    ASSERT_PRI_MSG(FAILED, 1, "89");
    ASSERT_PRI_MSG(FAILED, 2, "1");
    ASSERT_PRI_MSG(OK, 1, "88");
    ASSERT_TIMEOUT(CHECKOUT); // player 1 wins
}

GAME_TEST(5, bid_2)
{
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    ASSERT_PUB_MSG(OK, 0, "投标轮数 3");
    START_GAME();

    // item 5
    ASSERT_PRI_MSG(OK, 0, "9");
    ASSERT_PRI_MSG(OK, 1, "10");
    ASSERT_PRI_MSG(OK, 2, "11");
    ASSERT_PRI_MSG(OK, 3, "11");
    ASSERT_PUB_MSG(FAILED, 2, "撤标"); // must be private
    ASSERT_PRI_MSG(OK, 2, "撤标"); // cancel
    ASSERT_PRI_MSG(OK, 3, "10"); // modify
    ASSERT_PRI_MSG(OK, 4, "10");
    ASSERT_TIMEOUT(OK); // player 1 3 4 win
    ASSERT_PRI_MSG(FAILED, 0, "30"); // lose in term 1
    ASSERT_PRI_MSG(FAILED, 2, "30"); // lose in term 1
    ASSERT_PRI_MSG(FAILED, 1, "9"); // should greater than 10
    ASSERT_PRI_MSG(OK, 1, "100");
    ASSERT_PRI_MSG(FAILED, 1, "撤标"); // cannot cancel in term 2
    ASSERT_PRI_MSG(FAILED, 2, "11"); // lose in term 1
    ASSERT_PRI_MSG(OK, 3, "10");
    ASSERT_PRI_MSG(OK, 4, "11");
    ASSERT_TIMEOUT(CHECKOUT); // player 1 wins
}

GAME_TEST(5, do_nothing)
{
    ASSERT_PUB_MSG(OK, 0, "投标轮数 2");
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    ASSERT_PUB_MSG(OK, 0, "回合数 5");
    START_GAME();

    for (int j = 0; j < 10; ++j) {
        ASSERT_TIMEOUT(OK);
        ASSERT_TIMEOUT(CHECKOUT);
    }
    for (int i = 0; i < 5; ++i) {
        ASSERT_TIMEOUT(CHECKOUT); // discard stage
        for (int j = 0; j < 10; ++j) {
            ASSERT_TIMEOUT(OK);
            ASSERT_TIMEOUT(CHECKOUT);
        }
    }

    ASSERT_SCORE(100, 100, 100, 100, 100);
}

GAME_TEST(5, do_nothing_2)
{
    ASSERT_PUB_MSG(OK, 0, "投标轮数 2");
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    ASSERT_PUB_MSG(OK, 0, "回合数 5");
    START_GAME();

    for (int j = 0; j < 10; ++j) {
        ASSERT_TIMEOUT(OK);
        ASSERT_TIMEOUT(CHECKOUT);
    }
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 4; ++j) {
            ASSERT_PRI_MSG(OK, j, "不弃牌");
        }
        ASSERT_PRI_MSG(CHECKOUT, 4, "不弃牌");
        for (int j = 0; j < 10; ++j) {
            ASSERT_TIMEOUT(OK);
            ASSERT_TIMEOUT(CHECKOUT);
        }
    }

    ASSERT_SCORE(100, 100, 100, 100, 100);
}

GAME_TEST(5, do_nothing_no_items)
{
    ASSERT_PUB_MSG(OK, 0, "投标轮数 2");
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    ASSERT_PUB_MSG(OK, 0, "回合数 5");
    START_GAME();

    for (int j = 0; j < 10; ++j) {
        ASSERT_PRI_MSG(OK, 0, "1");
        ASSERT_TIMEOUT(CHECKOUT);
    }
    for (int i = 0; i < 5; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    ASSERT_SCORE(300, 50, 50, 50, 50);
}

// 种子 ABC
// 1号商品： ♣2 ♡2 ♢3 ♣4 ♡6
// 2号商品： ♣3 ♣5 ♠6 ♢9 ♠K
// 3号商品： ♡3 ♠3 ♣8 ♠8 ♠A
// 4号商品： ♢4 ♣10 ♣J ♢J ♡J
// 5号商品： ♡4 ♢6 ♢8 ♡9 ♢A
// 6号商品： ♠4 ♣9 ♢10 ♢K ♡A
// 7号商品： ♡5 ♢7 ♠10 ♠J ♢Q
// 8号商品： ♠5 ♣6 ♣7 ♠7 ♡10
// 9号商品： ♢2 ♢5 ♡8 ♠9 ♣Q ♡Q
// 10号商品： ♠2 ♡7 ♠Q ♣K ♡K ♣A

GAME_TEST(5, discard_1)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "投标轮数 1");
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    ASSERT_PUB_MSG(OK, 0, "回合数 1");
    START_GAME();

    for (int i = 0; i < 5; ++i) {
        ASSERT_PRI_MSG(OK, i, "50");
        ASSERT_TIMEOUT(CHECKOUT);
    }
    for (int i = 0; i < 5; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    ASSERT_TIMEOUT(CHECKOUT);
    for (int i = 0; i < 5; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    // pool coins 50 * 5 + 25 = 275
    ASSERT_SCORE(50 + 20, 25 + 3, 50 + 61, 50 + 184, 50 + 7);
}

GAME_TEST(5, discard_2)
{
    ASSERT_PUB_MSG(OK, 0, "种子 ABC");
    ASSERT_PUB_MSG(OK, 0, "投标轮数 1");
    ASSERT_PUB_MSG(OK, 0, "初始金币数 100");
    ASSERT_PUB_MSG(OK, 0, "回合数 2");
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1"); // pool coins 1
    ASSERT_TIMEOUT(CHECKOUT);
    for (int i = 0; i < 9; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    // round 1
    ASSERT_PRI_MSG(OK, 0, "弃牌 梅花2");
    ASSERT_PRI_MSG(OK, 0, "弃牌 红桃2 方板3 红桃6");
    ASSERT_PRI_MSG(FAILED, 0, "弃牌 黑桃A");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_PRI_MSG(FAILED, 0, "1"); // cannot bid own item
    ASSERT_PRI_MSG(FAILED, 0, "撤标"); // cannot cancel own item
    ASSERT_PRI_MSG(OK, 1, "50");
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PRI_MSG(OK, 0, "101"); // pool coins 1 + 101 = 102
    ASSERT_TIMEOUT(CHECKOUT);
    ASSERT_PRI_MSG(OK, 2, "1"); // pool coins 102 + 1 = 103
    ASSERT_TIMEOUT(CHECKOUT);
    for (int i = 0; i < 7; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    // round 2
    ASSERT_PRI_MSG(FAILED, 1, "弃牌"); // empty discard
    ASSERT_PRI_MSG(FAILED, 1, "弃牌 今天天气真好啊");
    ASSERT_PRI_MSG(FAILED, 1, "弃牌 红桃0");
    ASSERT_PRI_MSG(FAILED, 1, "弃牌 红桃14");
    ASSERT_PRI_MSG(FAILED, 1, "弃牌 条子3");
    ASSERT_PRI_MSG(OK, 1, "弃牌 红桃2 方块3 红心6");
    ASSERT_PRI_MSG(FAILED, 1, "弃牌 梅花2");
    ASSERT_PRI_MSG(FAILED, 1, "弃牌 梅花4");
    ASSERT_PUB_MSG(FAILED, 1, "不弃牌");
    ASSERT_PRI_MSG(OK, 0, "不弃牌");
    ASSERT_PRI_MSG(OK, 1, "不弃牌");
    ASSERT_PRI_MSG(OK, 3, "不弃牌");
    ASSERT_PRI_MSG(OK, 4, "不弃牌");
    ASSERT_PRI_MSG(CHECKOUT, 2, "弃牌 黑桃A");

    for (int i = 0; i < 8; ++i) {
        ASSERT_TIMEOUT(CHECKOUT);
    }

    // pool coins 103 + 25 + 50 + 50 = 228
    ASSERT_SCORE(48 + 152, 25, 99 + 76, 50, 50);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
