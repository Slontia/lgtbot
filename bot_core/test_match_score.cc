// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <string_view>
#include <map>
#include <filesystem>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "bot_core/score_calculation.h"

class TestMatchScore : public testing::Test
{
};

#define ASSERT_SCORE_INFO(ret, uid, game_score, zero_sum, top) \
do { \
    ASSERT_EQ((uid).GetStr(), ret.uid_.GetStr()); \
    ASSERT_EQ((game_score), ret.game_score_); \
    ASSERT_EQ((zero_sum), ret.zero_sum_score_); \
    ASSERT_EQ((top), ret.top_score_); \
} while (0)

TEST_F(TestMatchScore, two_different_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 20, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(2, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, -1000, -20);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 20, 1000, 20);
}

TEST_F(TestMatchScore, three_different_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 20, 0, 1500},
        UserInfoForCalScore{"3", 30, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(3, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, -1500, -30);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 20, 0, 0);
    ASSERT_SCORE_INFO(ret[2], UserID("3"), 30, 1500, 30);
}

TEST_F(TestMatchScore, four_different_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 20, 0, 1500},
        UserInfoForCalScore{"3", 30, 0, 1500},
        UserInfoForCalScore{"4", 40, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(4, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, -1500, -40);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 20, -500, 0);
    ASSERT_SCORE_INFO(ret[2], UserID("3"), 30, 500, 0);
    ASSERT_SCORE_INFO(ret[3], UserID("4"), 40, 1500, 40);
}

TEST_F(TestMatchScore, two_same_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 10, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(2, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, 0, 0);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 10, 0, 0);
}

TEST_F(TestMatchScore, three_both_top_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 20, 0, 1500},
        UserInfoForCalScore{"3", 20, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(3, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, -1500, -30);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 20, 750, 15);
    ASSERT_SCORE_INFO(ret[2], UserID("3"), 20, 750, 15);
}

TEST_F(TestMatchScore, three_both_last_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 10, 0, 1500},
        UserInfoForCalScore{"3", 20, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(3, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, -750, -15);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 10, -750, -15);
    ASSERT_SCORE_INFO(ret[2], UserID("3"), 20, 1500, 30);
}

TEST_F(TestMatchScore, four_both_top_last_user)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", 10, 0, 1500},
        UserInfoForCalScore{"2", 10, 0, 1500},
        UserInfoForCalScore{"3", 20, 0, 1500},
        UserInfoForCalScore{"4", 20, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(4, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), 10, -1000, -20);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), 10, -1000, -20);
    ASSERT_SCORE_INFO(ret[2], UserID("3"), 20, 1000, 20);
    ASSERT_SCORE_INFO(ret[3], UserID("4"), 20, 1000, 20);
}

TEST_F(TestMatchScore, four_user_complex)
{
    std::vector<UserInfoForCalScore> user_infos{
        UserInfoForCalScore{"1", -9, 0, 1500},
        UserInfoForCalScore{"2", -1, 0, 1500},
        UserInfoForCalScore{"3", 4, 0, 1500},
        UserInfoForCalScore{"4", 6, 0, 1500}
    };
    const auto ret = CalScores(user_infos);
    ASSERT_EQ(4, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID("1"), -9, -1800, -40);
    ASSERT_SCORE_INFO(ret[1], UserID("2"), -1, -200, 0);
    ASSERT_SCORE_INFO(ret[2], UserID("3"), 4, 800, 0);
    ASSERT_SCORE_INFO(ret[3], UserID("4"), 6, 1200, 40);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}

