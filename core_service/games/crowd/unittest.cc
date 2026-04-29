// Copyright (c) 2018-present,  ZhengYang Xu <github.com/jeffxzy>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

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
        ASSERT_SCORE(350,350,350,350,350,350);}

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
        ASSERT_SCORE(67,67,67,67,67,67);}

GAME_TEST(6, Q4_Basic_ABBCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 4"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,350,350,300,500,400);}

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
        ASSERT_SCORE(300,300,300,300,300,300);}

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
        ASSERT_SCORE(300,300,300,300,-50,-50);}

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

GAME_TEST(6, Q31_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 31"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(150,150,150,150,150,150);}

GAME_TEST(6, Q31_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 31"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(50,50,50,50,50,50);}

GAME_TEST(6, Q31_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 31"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q31_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 31"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q31_Basic_AADDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 31"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-350,-350,-200,-200,-200,-200);}

GAME_TEST(6, Q31_Basic_ABBCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 31"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(500,50,50,0,-200,-200);}

GAME_TEST(6, Q32_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q32_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-300,-300,-300,-300,-300,-300);}

GAME_TEST(6, Q32_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q32_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(5, Q32_Basic_AABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(100,100,400,-100,100);}

GAME_TEST(5, Q32_Basic_ABBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(200,-300,-300,200,300);}

GAME_TEST(5, Q32_Basic_ABCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(200,300,100,100,-100);}

GAME_TEST(5, Q32_Basic_ABCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(100,300,100,0,0);}

GAME_TEST(4, Q32_Basic_BBDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 32"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(CHECKOUT, 3, "D");
        ASSERT_SCORE(300,300,0,0);}

GAME_TEST(6, Q33_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 33"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q33_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 33"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q33_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 33"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q33_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 33"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,0,0,0);}

GAME_TEST(6, Q33_Basic_ABBCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 33"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,300,300,0,0,0);}

GAME_TEST(5, Q33_Basic_ABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 33"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,300,300,600,600);}

GAME_TEST(6, Q34_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 34"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q34_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 34"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q34_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 34"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q34_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 34"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(50,50,50,50,50,50);}

GAME_TEST(6, Q34_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 34"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,0,0,100,100,50);}

GAME_TEST(6, Q34_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 34"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,200,200,100);}

GAME_TEST(6, Q35_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 35"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(150,150,150,150,150,150);}

GAME_TEST(6, Q35_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 35"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q35_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 35"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q35_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 35"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-250,-250,0,0,-100,-100);}

GAME_TEST(6, Q35_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 35"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,200,200,-200,-200,0);}

GAME_TEST(6, Q36_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 36"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(50,50,50,50,50,50);}

GAME_TEST(6, Q36_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 36"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q36_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 36"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q36_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 36"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(6, Q36_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 36"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(50,100,100,150,150,200);}

GAME_TEST(6, Q36_Basic_BBCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 36"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,200);}

GAME_TEST(5, Q37_Basic_ADDDD_AABDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 2"); ASSERT_PUB_MSG(OK, 0, "测试 37"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "D");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(150,100,50,100,100);}

GAME_TEST(5, Q37_Basic_CDDDD_AAAAB_AAABB){
    ASSERT_PUB_MSG(OK, 0, "回合数 3"); ASSERT_PUB_MSG(OK, 0, "测试 37"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "D");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "A");
        ASSERT_PRI_MSG(CHECKOUT, 4, "B");
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "B");
        ASSERT_SCORE(-200,-150,-150,-150,150);}

GAME_TEST(5, Q38_Basic_ABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 38"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(-100,-150,-150,-150,200);}

GAME_TEST(5, Q38_Basic_AABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 38"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(200,200,-150,-150,200);}

GAME_TEST(5, Q39_Basic_ABBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 39"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(-400,-300,-300,-200,-100);}

GAME_TEST(5, Q39_Basic_BCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 39"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(-300,-600,-600,-600,-600);}

GAME_TEST(6, Q39_Basic_CCCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 39"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "C");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-200,-200,-200,-600,-600,-600);}

GAME_TEST(5, Q40_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 40"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 4, "A");
        ASSERT_SCORE(0,0,0,0,0);}

GAME_TEST(5, Q40_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 40"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "B");
        ASSERT_SCORE(0,0,0,0,0);}

GAME_TEST(5, Q40_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 40"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(100,100,100,100,100);}

GAME_TEST(5, Q40_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 40"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(50,50,50,50,50);}

GAME_TEST(5, Q40_Basic_AACCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 40"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(400,400,0,0,0);}

GAME_TEST(5, Q40_Basic_AABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 40"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,0,200,0,50);}

GAME_TEST(5, Q41_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 4, "A");
        ASSERT_SCORE(0,0,0,0,0);}

GAME_TEST(5, Q41_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "B");
        ASSERT_SCORE(200,200,200,200,200);}

GAME_TEST(5, Q41_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,0,0,0,0);}

GAME_TEST(5, Q41_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,0,0,0,0);}

GAME_TEST(5, Q41_All_E){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        for(int i = 0; i < 4; i++) ASSERT_PRI_MSG(OK, i, "E");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(-200,-200,-200,-200,-200);}

GAME_TEST(5, Q41_Basic_BBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(200,200,200,200,200);}

GAME_TEST(5, Q41_Basic_ABCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(0,100,-200,0,200);}

GAME_TEST(5, Q41_Basic_BDDEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 41"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "D");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "E");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(100,0,0,-200,-200);}

GAME_TEST(6, Q42_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 42"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-200,-200,-200,-200,-200,-200);}

GAME_TEST(6, Q42_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 42"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(150,150,150,150,150,150);}

GAME_TEST(6, Q42_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 42"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q42_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 42"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(200,200,150,150,0,0);}

GAME_TEST(6, Q42_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 42"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-200,-200,-200,-50,-50,200);}

GAME_TEST(5, Q43_Basic_ABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 43"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(300,200,200,200,-100);}

GAME_TEST(5, Q43_Basic_ABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 43"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(300,-100,-100,300,300);}

GAME_TEST(6, Q43_Basic_ABBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 43"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,167,167,167,400,400);}

GAME_TEST(6, Q44_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 44"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(200,200,200,200,200,200);}

GAME_TEST(6, Q44_Basic_ABBBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 44"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(0,50,50,50,50,50);}

GAME_TEST(6, Q45_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 45"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(200,-200,-200,50,50,300);}

GAME_TEST(6, Q45_Basic_AABBDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 45"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(200,200,-200,-200,300,300);}

GAME_TEST(6, Q45_Basic_AAABDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 45"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-100,-100,-100,150,300,300);}

GAME_TEST(4, Q46_Basic_AAAB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 46"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(300,300,300,100);}

GAME_TEST(4, Q46_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 46"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(-200,100,100,100);}

GAME_TEST(4, Q46_Basic_AABB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 46"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(-200,-200,100,100);}

GAME_TEST(5, Q47_Basic_ABCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 47"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(150,200,100,100,100);}

GAME_TEST(5, Q48_Basic_ABBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 48"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(150,200,200,100,-200);}

GAME_TEST(5, Q48_Basic_AABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 48"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(150,150,200,100,-200);}

GAME_TEST(5, Q48_Basic_ABCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 48"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,-100,100,100,200);}

GAME_TEST(6, Q49_Basic_AABCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 49"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,100,300,300,300,300);}

GAME_TEST(6, Q49_Basic_ABBCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 49"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q49_Basic_ABCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 49"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,300,300,300,0);}

GAME_TEST(6, Q49_Basic_ABCCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 49"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,300,-150,-150,200,200);}

GAME_TEST(6, Q49_Basic_AADDEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 49"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(550,550,500,500,-50,-50);}

GAME_TEST(6, Q50_Basic_ABBDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 50"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(500,300,300,300,300,300);}

GAME_TEST(6, Q50_Basic_ABCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 50"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(400,0,-100,100,100,100);}

GAME_TEST(6, Q50_Basic_ABCCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 50"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,-400,-100,-100,0,0);}

GAME_TEST(5, Q51_Basic_AABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 51"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(200,200,150,150,200);}

GAME_TEST(5, Q51_Basic_AABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 51"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(400,400,150,50,50);}

GAME_TEST(6, Q52_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 52"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,0,-40,-40);}

GAME_TEST(6, Q52_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 52"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-200,-200,-240,-240);}

GAME_TEST(6, Q53_Basic_ABBCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 53"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,200,200,-200,-50,-50);}

GAME_TEST(6, Q53_Basic_ABCCCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 53"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-300,100,100,100,100);}

GAME_TEST(5, Q54_Basic_ABCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 54"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(100,200,300,400,400);}

GAME_TEST(5, Q54_Basic_ABBDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 54"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(100,200,200,-400,-400);}

GAME_TEST(5, Q54_Basic_BBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 54"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(-200,-200,-300,-300,-400);}

GAME_TEST(6, Q55_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 55"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,200,-200,-200);}

GAME_TEST(6, Q55_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 55"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-200,-200,-200,200,200,200);}

GAME_TEST(6, Q55_Basic_AABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 55"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,-200,-200,100,100);}

GAME_TEST(4, Q56_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 56"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "A");
        ASSERT_SCORE(-300,-300,-300,-300);}

GAME_TEST(4, Q56_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 56"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(0,0,0,0);}

GAME_TEST(4, Q56_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 56"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(300,0,0,0);}

GAME_TEST(4, Q56_Basic_AABB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 56"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(-300,-300,0,0);}

GAME_TEST(4, Q56_Basic_AAAB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 56"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(-300,-300,-300,0);}

GAME_TEST(5, Q57_Basic_ABCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 57"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,100,200,200,-300);}

GAME_TEST(5, Q57_Basic_ABCEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 57"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "E");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(0,100,200,-400,-400);}

GAME_TEST(5, Q57_Basic_BCCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 57"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(0,0,0,0,400);}

GAME_TEST(6, Q58_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 58"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 5, "A");
        ASSERT_SCORE(-300,-300,-300,-300,-300,-300);}

GAME_TEST(6, Q58_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 58"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(-300,-300,-300,-300,-300,-300);}

GAME_TEST(6, Q58_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 58"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q58_All_D){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 58"); StartGame();
        for(int i = 0; i < 5; i++) ASSERT_PRI_MSG(OK, i, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-100,-100,-100,-100,-100,-100);}

GAME_TEST(6, Q58_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 58"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,-300,-300,-100,-100,100);}

GAME_TEST(6, Q58_Basic_AABCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 58"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-300,-300,300,100,-100,-100);}

GAME_TEST(5, Q59_Basic_AACCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 59"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(320,320,100,100,100);}

GAME_TEST(5, Q59_Basic_ABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 59"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,-100,-100,100,100);}

GAME_TEST(5, Q59_Basic_AABDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 59"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(0,0,0,-400,0);}

GAME_TEST(5, Q59_Basic_ABCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 59"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(0,0,0,200,0);}

GAME_TEST(5, Q60_Basic_ABBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 60"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,200,200,100,100);}

GAME_TEST(5, Q60_Basic_AABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 60"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,0,-300,-300,-100);}

GAME_TEST(5, Q60_Basic_ABCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 60"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(300,200,200,0,0);}

GAME_TEST(6, Q60_Basic_ABCCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 60"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,700,700);}

GAME_TEST(6, Q60_Basic_BBCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 60"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-300,-300,-100,0,0,0);}

GAME_TEST(6, Q60_Basic_BCCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 60"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,0,0,0);}

GAME_TEST(5, Q61_Basic_AABCE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 61"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(0,0,100,150,250);}

GAME_TEST(5, Q61_Basic_ABCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 61"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "E");
        ASSERT_SCORE(50,100,150,200,250);}

GAME_TEST(4, Q62_Basic_ABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 62"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(200,-150,-150,-150);}

GAME_TEST(4, Q62_Basic_AABB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 62"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(0,0,0,0);}

GAME_TEST(7, Q62_Basic_AABBBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 62"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(OK, 5, "C");
        ASSERT_PRI_MSG(CHECKOUT, 6, "D");
        ASSERT_SCORE(0,0,-150,-150,-150,200,0);}

GAME_TEST(6, Q63_Basic_AAABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 63"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(200,200,200,200,-50,-100);}

GAME_TEST(6, Q63_Basic_ABBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 63"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(-100,0,0,0,50,50);}

GAME_TEST(7, Q63_Basic_AABBCCE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 63"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(OK, 5, "C");
        ASSERT_PRI_MSG(CHECKOUT, 6, "E");
        ASSERT_SCORE(-100,-100,100,100,100,100,200);}

GAME_TEST(5, Q64_Basic_AAABC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 64"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(-100,-100,-100,100,0);}

GAME_TEST(5, Q64_Basic_ABBBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 64"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(200,-100,-100,-100,0);}

GAME_TEST(5, Q64_Basic_AABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 64"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(-100,-100,-100,-100,0);}

GAME_TEST(6, Q65_Basic_AABCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 65"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-100,-100,200,300,100,100);}

GAME_TEST(6, Q65_Basic_BBDDEF){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 65"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "F");
        ASSERT_SCORE(-200,-200,-100,-100,200,-300);}

GAME_TEST(6, Q66_Basic_AABCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 66"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,100,200,300,400,500);}

GAME_TEST(6, Q66_Basic_AABBDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 66"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,100,200,200,0,0);}

GAME_TEST(6, Q67_Basic_AAABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 67"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,200,200,400);}

GAME_TEST(6, Q67_Basic_ABBBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 67"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,-100,-100,-100,400,400);}

GAME_TEST(6, Q67_Basic_AAABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 67"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "C");
        ASSERT_SCORE(0,0,0,200,-200,-200);}

GAME_TEST(6, Q68_Basic_AAABBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 68"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(100,100,100,100,100,100);}

GAME_TEST(6, Q68_Basic_ABBBBB){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 68"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "B");
        ASSERT_SCORE(500,100,100,100,100,100);}

GAME_TEST(6, Q69_Basic_AAABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 69"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(100,100,100,180,0,100);}

GAME_TEST(6, Q69_Basic_ABBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 69"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(300,90,90,300,300,100);}

GAME_TEST(6, Q70_Basic_ABBCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 70"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,100,100,50,175,175);}

GAME_TEST(6, Q70_Basic_ABBBDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 70"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,-50,-50,-50,-25,-25);}

GAME_TEST(6, Q70_Basic_AAABBD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 70"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "B");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,0,0,100,100,100);}

GAME_TEST(6, Q71_Basic_AACDDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 71"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,100,300,-100,-100,0);}

GAME_TEST(6, Q71_Basic_AABCEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 71"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(100,100,400,300,0,0);}

GAME_TEST(6, Q71_Basic_AADDEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 71"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,0,-100,-100,100,100);}

GAME_TEST(6, Q71_Basic_AABBDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 71"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,0,-100,-100,200,100);}

GAME_TEST(6, Q71_Basic_ACDDEE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 71"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "E");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,300,-100,-100,100,100);}

GAME_TEST(6, Q72_Basic_AABBDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 72"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,0,200,200,200,-100);}

GAME_TEST(6, Q72_Basic_AABBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 72"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(200,200,0,0,400,100);}

GAME_TEST(6, Q72_Basic_ABBCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 72"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,200,200,-200,100,200);}

GAME_TEST(5, Q73_Basic_AABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 73"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(100,100,200,-300,0);}

GAME_TEST(5, Q73_Basic_ABBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 73"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(100,-200,-200,400,100);}

GAME_TEST(5, Q73_Basic_AABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 73"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(100,100,200,200,-300);}

GAME_TEST(6, Q74_Basic_ABBBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 74"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-100,-200,-200,-200,700,300);}

GAME_TEST(6, Q74_Basic_ABCCDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 74"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(-100,0,50,50,-300,-300);}

GAME_TEST(6, Q74_Basic_BCCCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 74"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "D");
        ASSERT_SCORE(0,-300,-300,-300,-300,300);}

GAME_TEST(4, Q75_All_A){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 75"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "A");
        ASSERT_PRI_MSG(CHECKOUT, 3, "A");
        ASSERT_SCORE(200,200,200,200);}

GAME_TEST(4, Q75_All_B){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 75"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "B");
        ASSERT_PRI_MSG(CHECKOUT, 3, "B");
        ASSERT_SCORE(100,100,100,100);}

GAME_TEST(4, Q75_All_C){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 75"); StartGame();
        for(int i = 0; i < 3; i++) ASSERT_PRI_MSG(OK, i, "C");
        ASSERT_PRI_MSG(CHECKOUT, 3, "C");
        ASSERT_SCORE(300,300,300,300);}

GAME_TEST(4, Q75_Basic_AACC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 75"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(CHECKOUT, 3, "C");
        ASSERT_SCORE(200,200,-100,-100);}

GAME_TEST(4, Q75_Basic_ABCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 75"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(CHECKOUT, 3, "C");
        ASSERT_SCORE(0,100,-100,-100);}

GAME_TEST(4, Q75_Basic_BBCC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 75"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(CHECKOUT, 3, "C");
        ASSERT_SCORE(100,100,300,300);}

GAME_TEST(5, Q76_Basic_AAABC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 76"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,0,0,400,300);}

GAME_TEST(5, Q76_Basic_AABBC){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 76"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "C");
        ASSERT_SCORE(0,0,-100,-100,300);}

GAME_TEST(5, Q76_Basic_AABCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 76"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,0,-100,300,200);}

GAME_TEST(5, Q76_Basic_BCDDD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 76"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "D");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(-100,-200,-300,-300,-300);}

GAME_TEST(5, Q77_Basic_ABBCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 77"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(300,100,100,-100,200);}

GAME_TEST(5, Q77_Basic_BBCCD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 77"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "B");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,0,400,400,0);}

GAME_TEST(5, Q77_Basic_AABBD){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 77"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "B");
        ASSERT_PRI_MSG(CHECKOUT, 4, "D");
        ASSERT_SCORE(0,0,100,100,0);}

GAME_TEST(6, Q78_Basic_ACCCDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 78"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "C");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(300,0,0,0,500,500);}

GAME_TEST(6, Q78_Basic_ABCDDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 78"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "B");
        ASSERT_PRI_MSG(OK, 2, "C");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(300,150,100,0,0,0);}

GAME_TEST(6, Q78_Basic_AABCCE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 78"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "B");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(300,300,150,0,0,0);}

GAME_TEST(6, Q78_Basic_AAADDE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 78"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "D");
        ASSERT_PRI_MSG(OK, 4, "D");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,0,0,250,250,500);}

GAME_TEST(6, Q78_Basic_AAACCE){
    ASSERT_PUB_MSG(OK, 0, "回合数 1"); ASSERT_PUB_MSG(OK, 0, "测试 78"); StartGame();
        ASSERT_PRI_MSG(OK, 0, "A");
        ASSERT_PRI_MSG(OK, 1, "A");
        ASSERT_PRI_MSG(OK, 2, "A");
        ASSERT_PRI_MSG(OK, 3, "C");
        ASSERT_PRI_MSG(OK, 4, "C");
        ASSERT_PRI_MSG(CHECKOUT, 5, "E");
        ASSERT_SCORE(0,0,0,200,200,500);}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
