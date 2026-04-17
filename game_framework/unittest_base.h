// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "game_framework/game_main.h"
#include "game_framework/game_options.h"
#include "game_framework/mock_match.h"
#include "game_framework/stage.h"
#include "game_framework/util.h"

// gflags must stay at global namespace scope (not inside GAME_MODULE_NAME).
DEFINE_string(resource_dir, "./resource_dir/", "The path of game image resources");
DEFINE_string(image_dir, "./.lgtbot_image/", "The path of directory to store generated images");
DEFINE_bool(gen_image, false, "Whether generate image or not");

// Normalize extend_enum constants and `StageErrCode` values for comparisons / gtest (avoids ambiguous `operator<<`).
#define LGTBOT_STAGE_ERR_UINT_(e) (static_cast<uint32_t>(::StageErrCode(e)))

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// `StageErrCode` is defined at global scope by extend_enum (via game_main.h).
inline constexpr ::StageErrCode OK = ::StageErrCode::OK;
inline constexpr ::StageErrCode CHECKOUT = ::StageErrCode::CHECKOUT;
inline constexpr ::StageErrCode CONTINUE = ::StageErrCode::CONTINUE;
inline constexpr ::StageErrCode FAILED = ::StageErrCode::FAILED;
inline constexpr ::StageErrCode READY = ::StageErrCode::READY;
inline constexpr ::StageErrCode NOT_FOUND = ::StageErrCode::NOT_FOUND;

internal::MainStage* MakeMainStage(MainStageFactory factory);

template <uint64_t kPlayerNum>
class TestGame : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        main_stage_.reset();

        resource_dir_str_ = std::filesystem::absolute(FLAGS_resource_dir).lexically_normal().string();
        if (!resource_dir_str_.empty() && resource_dir_str_.back() != '/') {
            resource_dir_str_ += '/';
        }

        const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
        // Truncate to 64 chars to avoid Windows MAX_PATH (260) limit
        auto truncate = [](const std::string& s) { return s.size() > 64 ? s.substr(0, 64) : s; };
        const std::string suite = truncate(info ? info->test_case_name() : "suite");
        const std::string name = truncate(info ? info->name() : "case");
        saved_image_dir_str_ =
                (std::filesystem::absolute(FLAGS_image_dir) / "unittest" / suite / name).lexically_normal().string();
        std::filesystem::create_directories(saved_image_dir_str_);

        // Default: no markdown→PNG in unit tests (matches run_game.cc). Non-empty MockMatch image_dir makes
        // SaveMarkdown run and can trigger huge strings / failures when --gen_image is off.
        const bool want_mock_images = FLAGS_gen_image && !FLAGS_image_dir.empty();
        ::enable_markdown_to_image = want_mock_images;

        imm_.user_num_ = 0;
        imm_.resource_dir_ = resource_dir_str_.c_str();
        imm_.saved_image_dir_ = saved_image_dir_str_.c_str();
        mut_ = MutableGenericOptions{};
        mut_.bench_computers_to_player_num_ = static_cast<uint32_t>(kPlayerNum);
        mut_.is_formal_ = 1;

        std::filesystem::path mock_image_dir;
        if (want_mock_images) {
            mock_image_dir = saved_image_dir_str_;
        }
        match_ = std::make_unique<MockMatch>(mock_image_dir, kPlayerNum);
        SyncOptions_();
    }

    void TearDown() override { main_stage_.reset(); }

    GenericOptions MakeGenericOptions_() const { return GenericOptions{imm_, mut_}; }

    void SyncOptions_() { options_.generic_options_ = MakeGenericOptions_(); }

    // Before StartGame(), both public and private messages only configure options / init commands.
    ::StageErrCode HandlePreStartMessage_(const char* const msg)
    {
        MsgReader reader(msg);
        for (const auto& cmd : k_init_options_commands) {
            reader.Reset();
            if (cmd.CallIfValid(reader, game_options_, mut_).has_value()) {
                SyncOptions_();
                return OK;
            }
        }
        const ::StageErrCode rc = game_options_.SetOption(msg) ? OK : FAILED;
        SyncOptions_();
        return rc;
    }

    bool StartGame()
    {
        if (main_stage_) {
            return false;
        }
        if (!AdaptOptions(match_->BoardcastMsgSender(), game_options_, MakeGenericOptions_(), mut_)) {
            return false;
        }
        SyncOptions_();
        const GenericOptions gen = MakeGenericOptions_();
        main_stage_.reset(MakeMainStage(MainStageFactory{game_options_, gen, *match_}));
        if (!main_stage_) {
            return false;
        }
        main_stage_->HandleStageBegin();
        return true;
    }

    ::StageErrCode PublicRequest(const uint64_t pid, const char* const msg)
    {
        if (!main_stage_) {
            return HandlePreStartMessage_(msg);
        }
        return main_stage_->HandleRequest(msg, pid, true, match_->BoardcastMsgSender());
    }

    ::StageErrCode PublicRequest(const uint64_t pid, const std::string& msg) { return PublicRequest(pid, msg.c_str()); }

    ::StageErrCode PrivateRequest(const uint64_t pid, const char* const msg)
    {
        if (!main_stage_) {
            return HandlePreStartMessage_(msg);
        }
        return main_stage_->HandleRequest(msg, pid, false,
                match_->TellMsgSender(PlayerID{static_cast<uint32_t>(pid)}));
    }

    ::StageErrCode PrivateRequest(const uint64_t pid, const std::string& msg) { return PrivateRequest(pid, msg.c_str()); }

    ::StageErrCode TimeoutRequest_()
    {
        EXPECT_NE(main_stage_, nullptr);
        return main_stage_->HandleTimeout();
    }

    ::StageErrCode LeaveRequest_(const PlayerID pid)
    {
        EXPECT_NE(main_stage_, nullptr);
        return main_stage_->HandleLeave(pid);
    }

    ::StageErrCode ComputerActRequest_(const uint64_t pid)
    {
        EXPECT_NE(main_stage_, nullptr);
        return main_stage_->HandleComputerAct(pid, true);
    }

    void AssertAchievementsImpl_(const PlayerID pid, std::initializer_list<const char*> names)
    {
        ASSERT_NE(main_stage_, nullptr);
        std::vector<std::string> expected;
        expected.reserve(names.size());
        for (const char* n : names) {
            expected.emplace_back(n);
        }
        std::sort(expected.begin(), expected.end());
        const char* const* p = main_stage_->VerdictateAchievements(pid);
        std::vector<std::string> actual;
        for (; *p; ++p) {
            actual.emplace_back(*p);
        }
        std::sort(actual.begin(), actual.end());
        EXPECT_EQ(actual, expected);
    }

    void AssertAchievementsImpl_(const PlayerID pid) { AssertAchievementsImpl_(pid, {}); }

    void AssertScoresImpl_(const std::vector<int64_t>& scores)
    {
        ASSERT_NE(main_stage_, nullptr);
        ASSERT_EQ(scores.size(), kPlayerNum);
        for (uint64_t i = 0; i < kPlayerNum; ++i) {
            EXPECT_EQ(main_stage_->PlayerScore(PlayerID{static_cast<uint32_t>(i)}), scores[i]);
        }
    }

    std::string resource_dir_str_;
    std::string saved_image_dir_str_;
    ImmutableGenericOptions imm_{};
    MutableGenericOptions mut_{};
    GameOptions game_options_{};
    // Some tests (e.g. holdem_poker) read `options_.generic_options_` for `PlayerNum()` etc.
    struct {
        GenericOptions generic_options_{};
    } options_;
    std::unique_ptr<MockMatch> match_;
    std::unique_ptr<internal::MainStage> main_stage_;
};

// gtest token-pastes fixture + "_" + test name; `TestGame<n>` ends with `>` and breaks ##. Use a derived class.
// `__LINE__` disambiguates repeated test names (e.g. several `start_game` cases with different player counts).
#define GAME_TEST_FIXTURE_NAME_INNER_(n, l) GAME_TEST_FIX_##n##_##l
#define GAME_TEST_FIXTURE_NAME_(name, line) GAME_TEST_FIXTURE_NAME_INNER_(name, line)
#define GAME_TEST(n, name)                                                     \
    class GAME_TEST_FIXTURE_NAME_(name, __LINE__) : public TestGame<n>           \
    {                                                                          \
    };                                                                         \
    TEST_F(GAME_TEST_FIXTURE_NAME_(name, __LINE__), name)

#define START_GAME() ASSERT_TRUE(this->StartGame())

#define ASSERT_PUB_MSG(expect, pid, msg) \
    ASSERT_EQ(LGTBOT_STAGE_ERR_UINT_(expect), LGTBOT_STAGE_ERR_UINT_(this->PublicRequest((pid), (msg))))
#define ASSERT_PRI_MSG(expect, pid, msg) \
    ASSERT_EQ(LGTBOT_STAGE_ERR_UINT_(expect), LGTBOT_STAGE_ERR_UINT_(this->PrivateRequest((pid), (msg))))
#define CHECK_PRI_MSG(expect, pid, msg) \
    (LGTBOT_STAGE_ERR_UINT_(this->PrivateRequest((pid), (msg))) == LGTBOT_STAGE_ERR_UINT_(expect))

#define ASSERT_TIMEOUT(expect) \
    ASSERT_EQ(LGTBOT_STAGE_ERR_UINT_(expect), LGTBOT_STAGE_ERR_UINT_(this->TimeoutRequest_()))
#define CHECK_TIMEOUT(expect) \
    (LGTBOT_STAGE_ERR_UINT_(this->TimeoutRequest_()) == LGTBOT_STAGE_ERR_UINT_(expect))

#define ASSERT_LEAVE(expect, pid) \
    ASSERT_EQ(LGTBOT_STAGE_ERR_UINT_(expect), \
            LGTBOT_STAGE_ERR_UINT_(this->LeaveRequest_(PlayerID{static_cast<uint32_t>(pid)})))

#define ASSERT_COMPUTER_ACT(expect, pid) \
    ASSERT_EQ(LGTBOT_STAGE_ERR_UINT_(expect), LGTBOT_STAGE_ERR_UINT_(this->ComputerActRequest_((pid))))

#define ASSERT_SCORE(...) \
    do { \
        this->AssertScoresImpl_({__VA_ARGS__}); \
    } while (0)

#define ASSERT_ACHIEVEMENTS(pid, ...) \
    this->AssertAchievementsImpl_(PlayerID{static_cast<uint32_t>(pid)} __VA_OPT__(, {__VA_ARGS__}))

#define ASSERT_ELIMINATED(pid) \
    ASSERT_TRUE((this->match_->IsEliminated(PlayerID{static_cast<uint32_t>(pid)})))

#define ASSERT_FINISHED(expect_finished)            \
    do {                                            \
        ASSERT_NE(this->main_stage_, nullptr);      \
        ASSERT_EQ((expect_finished), this->main_stage_->IsOver()); \
    } while (0)

#define ASSERT_ERRCODE(expect, value) ASSERT_EQ(LGTBOT_STAGE_ERR_UINT_(expect), LGTBOT_STAGE_ERR_UINT_(value))

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
