// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/bet_pool.h"

#include <vector>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

TEST(TestBetPool, only_one_bet)
{
    const auto ret = CallBetPool<char>(std::map<uint64_t, CallBetPoolInfo<char>>{
                {1, CallBetPoolInfo<char>{.coins_ = 10, .obj_ = 'A'}},
            });
    ASSERT_EQ(1, ret.size());
    ASSERT_EQ(10, ret[0].total_coins_);
    ASSERT_EQ(std::set<uint64_t>{1}, ret[0].winner_ids_);
    ASSERT_EQ(std::set<uint64_t>{1}, ret[0].participant_ids_);
}

TEST(TestBetPool, two_diff_bet_winner_bet_more)
{
    const auto ret = CallBetPool<char>(std::map<uint64_t, CallBetPoolInfo<char>>{
                {1, CallBetPoolInfo<char>{.coins_ = 10, .obj_ = 'A'}},
                {2, CallBetPoolInfo<char>{.coins_ = 12, .obj_ = 'B'}},
            });
    ASSERT_EQ(2, ret.size());
    ASSERT_EQ(20, ret[0].total_coins_);
    ASSERT_EQ(std::set<uint64_t>{2}, ret[0].winner_ids_);
    ASSERT_EQ((std::set<uint64_t>{1, 2}), ret[0].participant_ids_);
    ASSERT_EQ(2, ret[1].total_coins_);
    ASSERT_EQ(std::set<uint64_t>{2}, ret[1].winner_ids_);
    ASSERT_EQ(std::set<uint64_t>{2}, ret[1].participant_ids_);
}

TEST(TestBetPool, two_diff_bet_winner_bet_less)
{
    const auto ret = CallBetPool<char>(std::map<uint64_t, CallBetPoolInfo<char>>{
                {1, CallBetPoolInfo<char>{.coins_ = 12, .obj_ = 'A'}},
                {2, CallBetPoolInfo<char>{.coins_ = 10, .obj_ = 'B'}},
            });
    ASSERT_EQ(2, ret.size());
    ASSERT_EQ(20, ret[0].total_coins_);
    ASSERT_EQ(std::set<uint64_t>{2}, ret[0].winner_ids_);
    ASSERT_EQ((std::set<uint64_t>{1, 2}), ret[0].participant_ids_);
    ASSERT_EQ(2, ret[1].total_coins_);
    ASSERT_EQ(std::set<uint64_t>{1}, ret[1].winner_ids_);
    ASSERT_EQ(std::set<uint64_t>{1}, ret[1].participant_ids_);
}

TEST(TestBetPool, two_diff_bet_winner_bet_same)
{
    const auto ret = CallBetPool<char>(std::map<uint64_t, CallBetPoolInfo<char>>{
                {1, CallBetPoolInfo<char>{.coins_ = 10, .obj_ = 'A'}},
                {2, CallBetPoolInfo<char>{.coins_ = 10, .obj_ = 'B'}},
            });
    ASSERT_EQ(1, ret.size());
    ASSERT_EQ(20, ret[0].total_coins_);
    ASSERT_EQ(std::set<uint64_t>{2}, ret[0].winner_ids_);
    ASSERT_EQ((std::set<uint64_t>{1, 2}), ret[0].participant_ids_);
}

TEST(TestBetPool, three_diff_two_winners)
{
    const auto ret = CallBetPool<char>(std::map<uint64_t, CallBetPoolInfo<char>>{
                {1, CallBetPoolInfo<char>{.coins_ = 30, .obj_ = 'A'}},
                {2, CallBetPoolInfo<char>{.coins_ = 20, .obj_ = 'B'}},
                {3, CallBetPoolInfo<char>{.coins_ = 10, .obj_ = 'B'}},
            });
    ASSERT_EQ(3, ret.size());
    ASSERT_EQ(30, ret[0].total_coins_);
    ASSERT_EQ((std::set<uint64_t>{2, 3}), ret[0].winner_ids_);
    ASSERT_EQ((std::set<uint64_t>{1, 2, 3}), ret[0].participant_ids_);
    ASSERT_EQ(20, ret[1].total_coins_);
    ASSERT_EQ((std::set<uint64_t>{2}), ret[1].winner_ids_);
    ASSERT_EQ((std::set<uint64_t>{1, 2}), ret[1].participant_ids_);
    ASSERT_EQ(10, ret[2].total_coins_);
    ASSERT_EQ((std::set<uint64_t>{1}), ret[2].winner_ids_);
    ASSERT_EQ((std::set<uint64_t>{1}), ret[2].participant_ids_);
}
