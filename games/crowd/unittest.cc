// Copyright (c) 2018-present,  ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(6, Q0_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 0"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q0_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 0"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q0_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 0"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q0_Basic_ABBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 0"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,2,2,2,1,1);}

GAME_TEST(6, Q0_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 0"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,3,2,2,1,1);}

GAME_TEST(6, Q1_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q1_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(4, Q1_Basic_AABB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(2,2,6,6);}

GAME_TEST(4, Q1_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(2,0,0,0);}

GAME_TEST(4, Q1_Basic_AAAB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(2,2,2,0);}

GAME_TEST(12, Q2_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        for(int i = 0; i < 11; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 11, "A");
        ASSERT_SCORE(3,3,3,3,3,3,3,3,3,3,3,3);}

GAME_TEST(12, Q2_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        for(int i = 0; i < 11; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 11, "B");
        ASSERT_SCORE(0,0,0,0,0,0,0,0,0,0,0,0);}

GAME_TEST(4, Q2_Basic_AAAB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(1,1,1,1);}

GAME_TEST(4, Q2_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(1,0,0,0);}

GAME_TEST(6, Q3_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q3_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q3_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q3_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q3_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q3_Basic_ABBCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(1,2,2,4,5,6);}

GAME_TEST(6, Q4_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q4_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q4_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q4_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q4_Basic_ABBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(4,3,3,0,0,0);}

GAME_TEST(6, Q4_Basic_AACCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,2,2,2,2);}

GAME_TEST(6, Q5_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q5_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q5_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q5_Basic_AAAABC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,-1,-3,0);}

GAME_TEST(6, Q5_Basic_ABBBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,2,2,2,2,1);}

GAME_TEST(6, Q5_Basic_ABCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q6_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-3,-3,-3,-3,-3,-3);}

GAME_TEST(6, Q6_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(4, Q7_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "A");
        ASSERT_SCORE(2,2,2,2);}

GAME_TEST(4, Q7_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(2,2,2,2);}

GAME_TEST(5, Q7_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 4, "A");
        ASSERT_SCORE(2,2,2,2,2);}

GAME_TEST(5, Q7_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "B");
        ASSERT_SCORE(2,2,2,2,2);}

GAME_TEST(6, Q7_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q7_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(9, Q7_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 8; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 8, "A");
        ASSERT_SCORE(1,1,1,1,1,1,1,1,1);}

GAME_TEST(9, Q7_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 8; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 8, "B");
        ASSERT_SCORE(1,1,1,1,1,1,1,1,1);}

GAME_TEST(6, Q8_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q8_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q8_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q8_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,3,3,1,1,0);}

GAME_TEST(6, Q8_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-3,-3,-3,0,0,0);}

GAME_TEST(6, Q8_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q9_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q9_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-2,-2,-2,-2,-2,-2);}

GAME_TEST(6, Q9_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q9_Basic_AAAACC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(2,2,2,2,-1,-1);}

GAME_TEST(6, Q9_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-4,-4,-4,-2,-1,-1);}

GAME_TEST(6, Q10_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q10_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q10_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q10_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q10_Basic_BBBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,2,2,2);}

GAME_TEST(6, Q10_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(1,2,2,2,2,3);}

GAME_TEST(6, Q11_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q11_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q11_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q11_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q11_Basic_AABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,3,2,2,2,1);}

GAME_TEST(6, Q11_Basic_CDDDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "D");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(1,5,5,5,5,5);}

GAME_TEST(6, Q12_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(3,3,3,3,3,3);}

GAME_TEST(6, Q12_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q12_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q12_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-6,-6,-6,-6,-6,-6);}

GAME_TEST(6, Q12_Basic_ABCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-4,-3,-1,-3,-3,-3);}

GAME_TEST(6, Q12_Basic_ABCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(3,2,-1,-1,-1,-1);}

GAME_TEST(6, Q13_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q13_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q13_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q13_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q13_Basic_ABBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,2,2,1,1,1);}

GAME_TEST(6, Q13_Basic_AABBBD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,2);}

GAME_TEST(6, Q14_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q14_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(3,3,3,3,3,3);}

GAME_TEST(6, Q14_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q14_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,2,2,1,1);}

GAME_TEST(6, Q15_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q15_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q15_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-4,-4,-4,-4,-4,-4);}

GAME_TEST(6, Q15_Basic_AABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,-1,-1,-1,0);}

GAME_TEST(6, Q15_Basic_AACCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-4,-4,-4,-4);}

GAME_TEST(6, Q16_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-3,-3,-3,-3,-3,-3);}

GAME_TEST(6, Q16_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q16_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q16_Basic_AAACCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,3,3,-1,-1,-1);}

GAME_TEST(6, Q16_Basic_AAAACC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-3,-3,-3,-3,2,2);}

GAME_TEST(6, Q17_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q17_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q17_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q17_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(5,5,5,5,3,3);}

GAME_TEST(6, Q17_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,3,3,2,2,4);}

GAME_TEST(6, Q18_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q18_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q18_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q18_Basic_ACCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(2,1,1,1,1,1);}

GAME_TEST(6, Q18_Basic_AAAABC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,-1,0,-2);}

GAME_TEST(6, Q19_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q19_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q19_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(3,3,3,3,3,3);}

GAME_TEST(6, Q19_Basic_CCCCCB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(3,3,3,3,3,5);}

GAME_TEST(6, Q19_Basic_BBBBBA){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,6);}

GAME_TEST(6, Q19_Basic_BBBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,1,3,3,3);}

GAME_TEST(6, Q19_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(2,2,1,1,3,3);}

GAME_TEST(6, Q20_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-2,-2,-2,-2,-2,-2);}

GAME_TEST(6, Q20_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q20_Basic_ABBBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-2,0,0,0,0,0);}

GAME_TEST(6, Q21_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q21_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q21_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q21_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-2,-2,-2,-2,-2,-2);}

GAME_TEST(6, Q21_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(-4,-4,-4,-4,-4,-4);}

GAME_TEST(6, Q21_Basic_ABCDEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,-1,-2,-3,-2,-2);}

GAME_TEST(6, Q22_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q22_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q22_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(2,2,2,0,0,0);}

GAME_TEST(6, Q23_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-2,-2,-2,-2,-2,-2);}

GAME_TEST(6, Q23_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q23_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-2,-2,-2,-2,-2,-2);}

GAME_TEST(6, Q23_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-1,-1,-1,-1,-1,-1);}

GAME_TEST(6, Q23_Basic_ABBBAD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(2,0,0,0,2,0);}

GAME_TEST(6, Q23_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q23_Basic_AAACCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-2,-2,-2,0,0,0);}

GAME_TEST(6, Q23_Basic_AACCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-2,-2,-2,-2,-1,-1);}

GAME_TEST(6, Q24_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q24_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q24_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(2,2,2,2,2,2);}

GAME_TEST(6, Q24_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(3,3,3,3,3,3);}

GAME_TEST(6, Q24_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(4,4,4,4,4,4);}

GAME_TEST(6, Q24_All_F){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "F");
        ASSERT_PRI_MSG(CHECKOUT, 5, "F");
        ASSERT_SCORE(5,5,5,5,5,5);}

GAME_TEST(6, Q24_All_G){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "G");
        ASSERT_PRI_MSG(CHECKOUT, 5, "G");
        ASSERT_SCORE(6,6,6,6,6,6);}

GAME_TEST(6, Q24_All_H){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "H");
        ASSERT_PRI_MSG(CHECKOUT, 5, "H");
        ASSERT_SCORE(7,7,7,7,7,7);}

GAME_TEST(6, Q24_Basic_BCCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,0,0,0,0,0);}

GAME_TEST(6, Q24_Basic_GHHHHH){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "G");
        ASSERT_PRI_MSG(OK, 1, "H");
        ASSERT_PRI_MSG(OK, 2, "H");
        ASSERT_PRI_MSG(OK, 3, "H");
        ASSERT_PRI_MSG(OK, 4, "H");
        ASSERT_PRI_MSG(CHECKOUT, 5, "H");
        ASSERT_SCORE(6,0,0,0,0,0);}

GAME_TEST(6, Q24_Basic_BBBHHH){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "H");
        ASSERT_PRI_MSG(OK, 4, "H");
        ASSERT_PRI_MSG(CHECKOUT, 5, "H");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q25_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(1,1,1,1,1,1);}

GAME_TEST(6, Q25_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q25_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-2,-2,-2,-2,-2,-2);}

GAME_TEST(6, Q25_Basic_AABBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-1,-1,0,0,0,0);}

GAME_TEST(6, Q25_Basic_BBCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(1,1,-2,-2,-2,-2);}

GAME_TEST(6, Q25_Basic_CCAAAA){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(2,2,1,1,1,1);}

GAME_TEST(6, Q25_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-1,-1,-1,1,1,1);}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
