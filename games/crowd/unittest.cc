// Copyright (c) 2018-present,  ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

GAME_TEST(6, Q1_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q1_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q1_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q1_Basic_ABBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(300,200,200,200,100,100);}

GAME_TEST(6, Q1_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 1"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,200,200,-100,-100);}

GAME_TEST(6, Q2_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q2_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(4, Q2_Basic_AABB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(200,200,600,600);}

GAME_TEST(4, Q2_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(200,0,0,0);}

GAME_TEST(4, Q2_Basic_AAAB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 2"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(200,200,200,0);}

GAME_TEST(12, Q3_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 11; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 11, "A");
        ASSERT_SCORE(300,300,300,300,300,300,300,300,300,300,300,300);}

GAME_TEST(12, Q3_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        for(int i = 0; i < 11; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 11, "B");
        ASSERT_SCORE(0,0,0,0,0,0,0,0,0,0,0,0);}

GAME_TEST(4, Q3_Basic_AAAB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(100,100,100,150);}

GAME_TEST(4, Q3_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 3"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(100,50,50,50);}

GAME_TEST(6, Q4_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q4_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q4_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q4_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q4_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(50,50,50,50,50,50);}

GAME_TEST(6, Q4_Basic_ABBCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,200,200,400,500,300);}

GAME_TEST(6, Q5_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q5_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q5_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q5_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q5_Basic_ABBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(400,300,300,0,0,0);}

GAME_TEST(6, Q5_Basic_AACCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 5"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,200,200,200,200);}

GAME_TEST(6, Q6_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q6_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q6_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-50,-50,-50,-50,-50,-50);}

GAME_TEST(6, Q6_Basic_AAAABC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-300,0);}

GAME_TEST(6, Q6_Basic_ABBBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,200,200,200,200,100);}

GAME_TEST(6, Q6_Basic_ABCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 6"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,-50,-50,-50,-50);}

GAME_TEST(6, Q7_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-300,-300,-300,-300,-300,-300);}

GAME_TEST(6, Q7_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 7"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(7, Q8_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 6; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 6, "A");
        ASSERT_SCORE(100,100,100,100,100,100,100);}

GAME_TEST(7, Q8_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 6; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 6, "B");
        ASSERT_SCORE(125,125,125,125,125,125,125);}

GAME_TEST(9, Q8_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 8; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 8, "A");
        ASSERT_SCORE(100,100,100,100,100,100,100,100,100);}

GAME_TEST(9, Q8_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 8"); StartGame();
        for(int i = 0; i < 8; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 8, "B");
        ASSERT_SCORE(118,118,118,118,118,118,118,118,118);}

GAME_TEST(6, Q9_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q9_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q9_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q9_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q9_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,200,200,100,100,0);}

GAME_TEST(6, Q9_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-200,-200,-200,0,0,0);}

GAME_TEST(6, Q9_Basic_ABCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 9"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,0,0,0,0,50);}

GAME_TEST(6, Q10_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q10_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q10_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-50,-50,-50,-50,-50,-50);}

GAME_TEST(6, Q10_Basic_AAAACC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,200,200,200,-50,-50);}

GAME_TEST(6, Q10_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 10"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-200,-200,-200,-100,-50,-50);}

GAME_TEST(6, Q11_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q11_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q11_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q11_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q11_Basic_BBBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,250,250,250);}

GAME_TEST(6, Q11_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 11"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,200,200,250,250,300);}

GAME_TEST(6, Q12_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q12_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q12_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q12_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q12_Basic_ABBBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,200,200,200,100,0);}

GAME_TEST(6, Q12_Basic_CCDDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 12"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,100,400,400,400,400);}

GAME_TEST(6, Q13_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(300,300,300,300,300,300);}

GAME_TEST(6, Q13_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q13_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q13_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-600,-600,-600,-600,-600,-600);}

GAME_TEST(6, Q13_Basic_ABCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-400,-200,-100,-300,-300,-300);}

GAME_TEST(6, Q13_Basic_ABCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 13"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,200,-100,-100,-100,-100);}

GAME_TEST(10, Q14_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 9; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 9, "A");
        ASSERT_SCORE(0,0,0,0,0,0,0,0,0,0);}

GAME_TEST(10, Q14_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 9; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 9, "B");
        ASSERT_SCORE(0,0,0,0,0,0,0,0,0,0);}

GAME_TEST(10, Q14_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 9; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 9, "C");
        ASSERT_SCORE(0,0,0,0,0,0,0,0,0,0);}

GAME_TEST(10, Q14_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        for(int i = 0; i < 9; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 9, "D");
        ASSERT_SCORE(0,0,0,0,0,0,0,0,0,0);}

GAME_TEST(10, Q14_Basic_ABBBCCCCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(OK, 5, "C");
        ASSERT_PRI_MSG(OK, 6, "C");
        ASSERT_PRI_MSG(OK, 7, "C");
        ASSERT_PRI_MSG(OK, 8, "D");
        ASSERT_PRI_MSG(CHECKOUT, 9, "D");
        ASSERT_SCORE(300,200,200,200,100,100,100,100,200,200);}

GAME_TEST(10, Q14_Basic_AABBBBDDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 14"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(OK, 5, "B");
        ASSERT_PRI_MSG(OK, 6, "D");
        ASSERT_PRI_MSG(OK, 7, "D");
        ASSERT_PRI_MSG(OK, 8, "D");
        ASSERT_PRI_MSG(CHECKOUT, 9, "D");
        ASSERT_SCORE(0,0,0,0,0,0,200,200,200,200);}

GAME_TEST(6, Q15_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q15_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(300,300,300,300,300,300);}

GAME_TEST(6, Q15_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q15_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 15"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,200,200,100,100);}

GAME_TEST(6, Q16_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q16_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q16_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-400,-400,-400,-400,-400,-400);}

GAME_TEST(6, Q16_Basic_AABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,-100,-100,-100,0);}

GAME_TEST(6, Q16_Basic_AACCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 16"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-400,-400,-400,-400);}

GAME_TEST(6, Q17_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-300,-300,-300,-300,-300,-300);}

GAME_TEST(6, Q17_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q17_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q17_Basic_AAACCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(300,300,300,-100,-100,-100);}

GAME_TEST(6, Q17_Basic_AAAACC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 17"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-300,-300,-300,-300,200,200);}

GAME_TEST(6, Q18_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(83,83,83,83,83,83);}

GAME_TEST(6, Q18_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(83,83,83,83,83,83);}

GAME_TEST(6, Q18_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(50,50,50,50,50,50);}

GAME_TEST(6, Q18_Basic_BCCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(500,60,60,60,60,60);}

GAME_TEST(6, Q18_Basic_ABBBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,100,100,100,100,100);}

GAME_TEST(6, Q18_Basic_BBBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(167,167,167,100,100,100);}

GAME_TEST(6, Q18_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 18"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(250,250,250,250,150,150);}

GAME_TEST(6, Q19_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q19_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q19_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q19_Basic_ACCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,100,100,100,100,100);}

GAME_TEST(6, Q19_Basic_AAAABC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 19"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,0,-200);}

GAME_TEST(6, Q20_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q20_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q20_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(300,300,300,300,300,300);}

GAME_TEST(6, Q20_Basic_BCCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(500,300,300,300,300,300);}

GAME_TEST(6, Q20_Basic_ABBBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(600,0,0,0,0,0);}

GAME_TEST(6, Q20_Basic_BBBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(50,50,50,50,300,300);}

GAME_TEST(6, Q20_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 20"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,200,100,100,300,300);}

GAME_TEST(6, Q21_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q21_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q21_Basic_ABBBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 21"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-200,0,0,0,0,0);}

GAME_TEST(6, Q22_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q22_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q22_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q22_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q22_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(-450,-450,-450,-450,-450,-450);}

GAME_TEST(6, Q22_Basic_ABCDEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 22"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,-100,-200,-300,-250,-250);}

GAME_TEST(6, Q23_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q23_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q23_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 23"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,0,0,0);}

GAME_TEST(6, Q24_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q24_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q24_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q24_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-50,-50,-50,-50,-50,-50);}

GAME_TEST(6, Q24_Basic_ABBBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(200,0,0,0,200,0);}

GAME_TEST(6, Q24_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q24_Basic_AAACCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,-200,-200,0,0,0);}

GAME_TEST(6, Q24_Basic_AACCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 24"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,-200,-200,-200,-50,-50);}

GAME_TEST(6, Q25_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q25_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q25_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q25_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,300,300,300,300,300);}

GAME_TEST(6, Q25_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(400,400,400,400,400,400);}

GAME_TEST(6, Q25_All_F){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "F");
        ASSERT_PRI_MSG(CHECKOUT, 5, "F");
        ASSERT_SCORE(500,500,500,500,500,500);}

GAME_TEST(6, Q25_All_G){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "G");
        ASSERT_PRI_MSG(CHECKOUT, 5, "G");
        ASSERT_SCORE(600,600,600,600,600,600);}

GAME_TEST(6, Q25_All_H){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "H");
        ASSERT_PRI_MSG(CHECKOUT, 5, "H");
        ASSERT_SCORE(700,700,700,700,700,700);}

GAME_TEST(6, Q25_Basic_BCCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,0,0,0,0,0);}

GAME_TEST(6, Q25_Basic_GHHHHH){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "G");
        ASSERT_PRI_MSG(OK, 1, "H");
        ASSERT_PRI_MSG(OK, 2, "H");
        ASSERT_PRI_MSG(OK, 3, "H");
        ASSERT_PRI_MSG(OK, 4, "H");
        ASSERT_PRI_MSG(CHECKOUT, 5, "H");
        ASSERT_SCORE(600,0,0,0,0,0);}

GAME_TEST(6, Q25_Basic_BBBHHH){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 25"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "H");
        ASSERT_PRI_MSG(OK, 4, "H");
        ASSERT_PRI_MSG(CHECKOUT, 5, "H");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q26_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(150,150,150,150,150,150);}

GAME_TEST(6, Q26_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q26_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q26_Basic_AABBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,0,0,0,0);}

GAME_TEST(6, Q26_Basic_BBCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,-200,-200,-200,-200);}

GAME_TEST(6, Q26_Basic_CCAAAA){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(OK, 4, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(200,200,150,150,150,150);}

GAME_TEST(6, Q26_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 26"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,-100,100,100,100);}

GAME_TEST(6, Q27_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q27_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q27_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q27_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q27_Basic_ABCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(150,50,100,0,0,0);}

GAME_TEST(6, Q27_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,0,0,100);}

GAME_TEST(6, Q27_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 27"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-150,-150,-150,-150);}

GAME_TEST(6, Q28_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 28"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q28_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 28"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q28_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 28"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q28_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 28"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(150,150,150,150,150,150);}

GAME_TEST(6, Q28_Basic_AAABDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 28"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,-200,-200,100,-150,-150);}

GAME_TEST(6, Q28_Basic_AACDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 28"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(200,200,0,150,150,150);}

GAME_TEST(6, Q29_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 29"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(50,50,50,50,50,50);}

GAME_TEST(6, Q29_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 29"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-50,-50,-50,-50,-50,-50);}

GAME_TEST(6, Q29_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 29"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q29_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 29"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-250,-250,-250,-250,-250,-250);}

GAME_TEST(6, Q29_Basic_AABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 29"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,-50,-50,-50,200);}

GAME_TEST(6, Q29_Basic_ABCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 29"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(50,150,-100,-100,-100,400);}

GAME_TEST(6, Q30_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q30_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q30_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-50,-50,-50,-50,-50,-50);}

GAME_TEST(6, Q30_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-3000,-3000,-3000,-3000,-3000,-3000);}

GAME_TEST(6, Q30_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q30_Basic_ABCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,-50,-50,-50,-50);}

GAME_TEST(6, Q30_Basic_ABCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,-3000,-3000,-3000);}

GAME_TEST(6, Q30_Basic_ABBBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 30"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,0,0,0,100,100);}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
