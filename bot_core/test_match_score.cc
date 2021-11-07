#include <array>
#include <string_view>
#include <map>
#include <filesystem>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#define private public
#include "bot_core/match.h"
#undef private

void* OpenMessager(const uint64_t id, const bool is_uid) { return nullptr; }
void MessagerPostText(void* p, const char* data, uint64_t len) {}
void MessagerPostUser(void* p, uint64_t uid, bool is_at) {}
void MessagerPostImage(void* p, const char* path) {}
void MessagerFlush(void* p) {}
void CloseMessager(void* p) {}
const char* GetUserName(const uint64_t uid, const uint64_t* const group_id) { return nullptr; }

class TestMatchScore : public testing::Test
{
};

#define ASSERT_SCORE_INFO(ret, uid, game_score, zero_sum, top) \
do { \
    ASSERT_EQ((uid).Get(), ret.uid_.Get()); \
    ASSERT_EQ((game_score), ret.game_score_); \
    ASSERT_EQ((zero_sum), ret.zero_sum_score_); \
    ASSERT_EQ((top), ret.top_score_); \
} while (0)

TEST_F(TestMatchScore, two_different_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 20} });
    ASSERT_EQ(2, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, -1000, -20);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 20, 1000, 20);
}

TEST_F(TestMatchScore, three_different_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 20}, {3, 30}});
    ASSERT_EQ(3, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, -1500, -30);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 20, 0, 0);
    ASSERT_SCORE_INFO(ret[2], UserID(3), 30, 1500, 30);
}

TEST_F(TestMatchScore, four_different_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 20}, {3, 30}, {4, 40}});
    ASSERT_EQ(4, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, -1500, -40);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 20, -500, 0);
    ASSERT_SCORE_INFO(ret[2], UserID(3), 30, 500, 0);
    ASSERT_SCORE_INFO(ret[3], UserID(4), 40, 1500, 40);
}

TEST_F(TestMatchScore, two_same_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 10} });
    ASSERT_EQ(2, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, 0, 0);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 10, 0, 0);
}

TEST_F(TestMatchScore, three_both_top_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 20}, {3, 20} });
    ASSERT_EQ(3, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, -1500, -30);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 20, 750, 15);
    ASSERT_SCORE_INFO(ret[2], UserID(3), 20, 750, 15);
}

TEST_F(TestMatchScore, three_both_last_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 10}, {3, 20} });
    ASSERT_EQ(3, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, -750, -15);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 10, -750, -15);
    ASSERT_SCORE_INFO(ret[2], UserID(3), 20, 1500, 30);
}

TEST_F(TestMatchScore, four_both_top_last_user)
{
    const auto ret = Match::CalScores_({ {1, 10}, {2, 10}, {3, 20}, {4, 20}});
    ASSERT_EQ(4, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), 10, -1000, -20);
    ASSERT_SCORE_INFO(ret[1], UserID(2), 10, -1000, -20);
    ASSERT_SCORE_INFO(ret[2], UserID(3), 20, 1000, 20);
    ASSERT_SCORE_INFO(ret[3], UserID(4), 20, 1000, 20);
}

TEST_F(TestMatchScore, four_user_complex)
{
    const auto ret = Match::CalScores_({ {1, -9}, {2, -1}, {3, 4}, {4, 6}});
    ASSERT_EQ(4, ret.size());
    ASSERT_SCORE_INFO(ret[0], UserID(1), -9, -1800, -40);
    ASSERT_SCORE_INFO(ret[1], UserID(2), -1, -200, 0);
    ASSERT_SCORE_INFO(ret[2], UserID(3), 4, 800, 0);
    ASSERT_SCORE_INFO(ret[3], UserID(4), 6, 1200, 40);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}

