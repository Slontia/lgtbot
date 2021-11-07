#include <array>
#include <string_view>
#include <map>
#include <filesystem>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "bot_core/db_manager.h"
#include "sqlite_modern_cpp.h"

static const std::filesystem::path k_db_path = "/tmp/lgtbot_test_db.db";

class TestDB : public testing::Test
{
  public:
    virtual void SetUp() override
    {
        std::filesystem::remove(k_db_path);
    }

  protected:
    bool UseDB_()
    {
        return (db_manager_ = SQLiteDBManager::UseDB(k_db_path)) != nullptr;
    }

    std::unique_ptr<DBManagerBase> db_manager_;
};

#define ASSERT_USER_PROFILE(uid, zero_sum, top, match_count, recent_count) \
[&]() -> UserProfile { \
    const auto user_profile = db_manager_->GetUserProfile(uid); \
    EXPECT_EQ((uid).Get(), user_profile.uid_.Get()); \
    EXPECT_EQ((zero_sum), user_profile.total_zero_sum_score_); \
    EXPECT_EQ((top), user_profile.total_top_score_); \
    EXPECT_EQ((match_count), user_profile.match_count_); \
    EXPECT_EQ((recent_count), user_profile.recent_matches_.size()); \
    return user_profile; \
}()

#define ASSERT_MATCH_PROFILE(profile, game_name, user_count, game_score, zero_sum, top) \
[&]() { \
    ASSERT_EQ((game_name), profile.game_name_); \
    ASSERT_EQ((user_count), profile.user_count_); \
    ASSERT_EQ((game_score), profile.game_score_); \
    ASSERT_EQ((zero_sum), profile.zero_sum_score_); \
    ASSERT_EQ((top), profile.top_score_); \
}()

TEST_F(TestDB, get_user_profile_empty)
{
    ASSERT_TRUE(UseDB_());
    ASSERT_USER_PROFILE(UserID(1), 0, 0, 0, 0);
}

TEST_F(TestDB, get_user_profile_one_match)
{
    ASSERT_TRUE(UseDB_());
    db_manager_->RecordMatch("mygame", std::nullopt, 1, 1,
            std::vector<ScoreInfo>{ScoreInfo(UserID(2), 30, 40, 50), ScoreInfo(UserID(3), 10, 10, 10)});
    ASSERT_USER_PROFILE(UserID(1), 0, 0, 0, 0);
    const auto profile_2 = ASSERT_USER_PROFILE(UserID(2), 40, 50, 1, 1);
    ASSERT_MATCH_PROFILE(profile_2.recent_matches_[0], "mygame", 2, 30, 40, 50);
    const auto profile_3 = ASSERT_USER_PROFILE(UserID(3), 10, 10, 1, 1);
    ASSERT_MATCH_PROFILE(profile_3.recent_matches_[0], "mygame", 2, 10, 10, 10);
}

TEST_F(TestDB, get_user_profile_two_matches)
{
    ASSERT_TRUE(UseDB_());
    db_manager_->RecordMatch("g1", std::nullopt, 1, 1, std::vector<ScoreInfo>{ScoreInfo(UserID(1), 30, 40, 50)});
    db_manager_->RecordMatch("g2", std::nullopt, 1, 1, std::vector<ScoreInfo>{ScoreInfo(UserID(1), -20, -10, 0)});
    const auto profile = ASSERT_USER_PROFILE(UserID(1), 30, 50, 2, 2);
    ASSERT_MATCH_PROFILE(profile.recent_matches_[0], "g2", 1, -20, -10, 0);
    ASSERT_MATCH_PROFILE(profile.recent_matches_[1], "g1", 1, 30, 40, 50);
}

TEST_F(TestDB, get_user_profile_more_than_ten_matches)
{
    ASSERT_TRUE(UseDB_());
    for (int i = 0; i < 15; ++i) {
        db_manager_->RecordMatch("g1", std::nullopt, 1, 1, std::vector<ScoreInfo>{ScoreInfo(UserID(1), 10, 10, 10)});
    }
    const auto profile = ASSERT_USER_PROFILE(UserID(1), 150, 150, 15, 10);
    for (int i = 0; i < 10; ++i) {
        ASSERT_MATCH_PROFILE(profile.recent_matches_[i], "g1", 1, 10, 10, 10);
    }
}

TEST_F(TestDB, reopen_db)
{
    ASSERT_TRUE(UseDB_());
    db_manager_->RecordMatch("mygame", std::nullopt, 1, 1,
            std::vector<ScoreInfo>{ScoreInfo(UserID(2), 30, 40, 50), ScoreInfo(UserID(3), 10, 10, 10)});

    db_manager_.reset();
    ASSERT_TRUE(UseDB_());

    ASSERT_USER_PROFILE(UserID(1), 0, 0, 0, 0);
    const auto profile_2 = ASSERT_USER_PROFILE(UserID(2), 40, 50, 1, 1);
    ASSERT_MATCH_PROFILE(profile_2.recent_matches_[0], "mygame", 2, 30, 40, 50);
    const auto profile_3 = ASSERT_USER_PROFILE(UserID(3), 10, 10, 1, 1);
    ASSERT_MATCH_PROFILE(profile_3.recent_matches_[0], "mygame", 2, 10, 10, 10);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}

