// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

#include "bot_core/bot_core.h"
#include "bot_core/bot_ctx.h"
#include "bot_core/db_manager.h"
#include "bot_core/match/match.h"
#include "bot_core/match/match_manager.h"
#include "bot_core/msg_sender.h"
#include "bot_core/score_calculation.h"
#include "bot_core/timer.h"
#include "utility/process_signals.h"

using namespace lgtbot;
using namespace lgtbot::core;
using namespace lgtbot::core::match;

namespace lgtbot::core::test {

static_assert(TEST_BOT);

DEFINE_string(image_path, "/tmp/lgtbot_test_chaos", "Path for storing game images");

DEFINE_uint64(chaos_users, 16, "number of concurrent simulated users");
DEFINE_uint64(chaos_rounds, 1000, "number of actions per user (0 = unlimited)");
DEFINE_uint64(chaos_duration, 0, "run duration in seconds (0 = unlimited)");
DEFINE_uint64(chaos_events, 0, "target number of created games (0 = unlimited)");
DEFINE_uint64(chaos_seed, 0, "random seed for reproducibility");
DEFINE_bool(chaos_use_real_timer, false, "use real wall-clock timers instead of skipping waits");
DEFINE_uint64(chaos_report_interval, 1, "report interval in seconds");
DEFINE_uint64(chaos_sample_interval_ms, 100, "throughput sampling interval in milliseconds");

// Game options injected via %配置.
DEFINE_int32(chaos_timer_sec, 2, "chaos_game 时限");
DEFINE_uint64(chaos_max_players, 8, "chaos_game 最大玩家数");
DEFINE_bool(chaos_reject_start, false, "chaos_game 拒绝开始");
DEFINE_uint64(chaos_p_eliminate, 10, "chaos_game 淘汰概率");
DEFINE_uint64(chaos_p_hook, 10, "chaos_game 挂机概率");
DEFINE_uint64(chaos_p_retime, 10, "chaos_game 重计时概率");
DEFINE_uint64(chaos_p_achieve, 10, "chaos_game 成就概率");
DEFINE_uint64(chaos_p_score, 10, "chaos_game 加分概率");
DEFINE_uint64(chaos_p_over, 10, "chaos_game 直接结束概率");
DEFINE_uint64(chaos_p_fail, 10, "chaos_game 失败概率");
DEFINE_uint64(chaos_p_crash, 0, "chaos_game 崩溃概率");

namespace {

constexpr const char* k_admin = "admin";

class MockDBManager : public DBManagerBase
{
  public:
    virtual std::vector<ScoreInfo> RecordMatch(const std::string& game_name, const std::optional<GroupID> gid,
            const UserID& host_uid, const uint64_t multiple,
            const std::vector<std::pair<UserID, int64_t>>& game_score_infos,
            const std::vector<std::pair<UserID, std::string>>& achievements) override
    {
        std::vector<UserInfoForCalScore> user_infos;
        for (const auto& [uid, game_score] : game_score_infos) {
            user_infos.emplace_back(uid, game_score, 0, 1500);
        }
        return CalScores(user_infos, multiple);
    }

    virtual UserProfile GetUserProfile(const UserID& uid, const std::string_view& time_range_begin,
            const std::string_view& time_range_end) override { return {}; }
    virtual bool Suicide(const UserID& uid, const uint32_t required_match_num) override { return true; }
    virtual RankInfo GetRank(const std::string_view& time_range_begin, const std::string_view& time_range_end,
            const std::optional<GroupID>& gid = std::nullopt) override { return {}; }
    virtual GameRankInfo GetLevelScoreRank(const std::string& game_name, const std::string_view& time_range_begin,
            const std::string_view& time_range_end, const std::optional<GroupID>& gid = std::nullopt) override { return {}; }
    virtual AchievementStatisticInfo GetAchievementStatistic(const UserID& uid, const std::string& game_name,
            const std::string& achievement_name) override { return {}; }
    virtual std::vector<HonorInfo> GetHonors(const std::string& honor, const uint32_t limit) override { return {}; }
    virtual bool AddHonor(const UserID& uid, const std::string_view& description) override { return true; }
    virtual bool DeleteHonor(const int32_t id) override { return true; }
};

// Discard outgoing messages; the chaos test only checks that nothing crashes or deadlocks.
void HandleMessages(void*, const char*, const int, const LGTBot_Message*, const size_t) {}

void GetUserName(void* handler, char* buffer, size_t size, const char* const user_id)
{
    strncpy(buffer, user_id, size);
}

void GetUserNameInGroup(void* handler, char* buffer, size_t size, const char* group_id, const char* const user_id)
{
    snprintf(buffer, size, "%s(gid=%s)", user_id, group_id);
}

int DownloadUserAvatar(void*, const char*, const char*) { return false; }

struct SimCommand
{
    std::string uid;
    std::string gid;   // empty means private request
    std::string msg;
};

// Build the next random action for a worker.
SimCommand NextCommand(std::mt19937& rng, const uint32_t worker_id, const std::string& gid)
{
    auto pick = [&](const uint64_t mod) { return std::uniform_int_distribution<uint64_t>(0, mod - 1)(rng); };
    auto pct = [&](const uint64_t p) { return std::uniform_int_distribution<uint64_t>(0, 99)(rng) < p; };

    const std::string uid = "u" + std::to_string(worker_id);

    // Occasional admin interrupt exercises the Terminate path from a different actor.
    if (pct(1)) {
        return SimCommand{k_admin, {}, "%中断"};
    }

    const auto when_public = pct(50);
    const auto channel = when_public ? gid : std::string{};

    switch (pick(10)) {
    case 0: { // new game
        std::string m = "#新游戏 混沌游戏";
        if (pct(10)) m += " 单机";
        else if (pct(10)) m += " 多人";
        return SimCommand{uid, channel, std::move(m)};
    }
    case 1: { // join
        const std::string m = pct(50) ? "#加入" : ("#加入 " + std::to_string(1 + pick(64)));
        return SimCommand{uid, channel, m};
    }
    case 2:   return SimCommand{uid, channel, "#开始"};
    case 3:   return SimCommand{uid, channel, pct(50) ? "#退出" : "#退出 强制"};
    case 4:   return SimCommand{uid, channel, pct(50) ? "#中断" : "#中断 取消"};
    case 5:   return SimCommand{uid, channel, "#替补至 " + std::to_string(pick(16))};
    case 6:   return SimCommand{uid, channel, "#规则 混沌游戏 细节"};
    default:  return SimCommand{uid, channel, "行动"};  // the only game command
    }
}

bool ExecuteWithWatchdog(BotCtx& bot, const SimCommand& cmd, const std::chrono::seconds timeout)
{
    auto task = [&bot, &cmd]() -> ErrCode {
        if (cmd.gid.empty()) {
            return LGTBot_HandlePrivateRequest(&bot, cmd.uid.c_str(), cmd.msg.c_str());
        }
        return LGTBot_HandlePublicRequest(&bot, cmd.gid.c_str(), cmd.uid.c_str(), cmd.msg.c_str());
    };
    auto fut = std::async(std::launch::async, std::move(task));
    if (fut.wait_for(timeout) == std::future_status::timeout) {
        fprintf(stderr, "DEADLOCK: uid=%s gid=%s msg=[%s]\n", cmd.uid.c_str(),
                cmd.gid.empty() ? "-" : cmd.gid.c_str(), cmd.msg.c_str());
        return false;
    }
    fut.get();
    return true;
}

} // namespace

class ChaosTest : public testing::Test
{
  public:
    void SetUp() override
    {
        const char* tmp_dir = std::getenv("TMPDIR");
        const std::string tmp_base = tmp_dir ? tmp_dir : "/tmp";
        conf_dir_ = tmp_base + "/lgtbot_chaos_" + std::to_string(std::rand());
        std::filesystem::create_directories(conf_dir_);

        auto game_handles = BotCtx::LoadGameModules(TEST_CHAOS_PLUGIN_DIR);
        ASSERT_TRUE(std::holds_alternative<GameHandleMap>(game_handles)) << "failed to load chaos game plugin";
        bot_.reset(new BotCtx(
                    TEST_CHAOS_PLUGIN_DIR,
                    conf_dir_ + "/config.json",
                    FLAGS_image_path,
                    LGTBot_Callback{
                        .get_user_name = GetUserName,
                        .get_user_name_in_group = GetUserNameInGroup,
                        .download_user_avatar = DownloadUserAvatar,
                        .handle_messages = HandleMessages,
                    },
                    std::move(std::get<GameHandleMap>(game_handles)),
                    std::set<UserID>{UserID{k_admin}},
#ifdef WITH_SQLITE
                    std::make_unique<MockDBManager>(),
#endif
                    MutableBotOption{},
                    nlohmann::json::object(),
                    nullptr));

        Timer::skip_timer_ = !FLAGS_chaos_use_real_timer;
        InjectOptions_();
    }

    void TearDown() override
    {
        bot_.reset();
        if (!conf_dir_.empty()) {
            std::filesystem::remove_all(conf_dir_);
        }
    }

  protected:
    void InjectOption_(const std::string& name, const std::string& value)
    {
        const std::string msg = "%配置 混沌游戏 " + name + " " + value;
        const auto rc = LGTBot_HandlePrivateRequest(bot_.get(), k_admin, msg.c_str());
        ASSERT_EQ(EC_OK, rc) << "inject option failed: " << name;
    }

    void InjectOptions_()
    {
        InjectOption_("时限", std::to_string(FLAGS_chaos_timer_sec));
        InjectOption_("最大玩家数", std::to_string(FLAGS_chaos_max_players));
        InjectOption_("拒绝开始", FLAGS_chaos_reject_start ? "开启" : "关闭");
        InjectOption_("随机种子", std::to_string(FLAGS_chaos_seed));
        InjectOption_("淘汰概率", std::to_string(FLAGS_chaos_p_eliminate));
        InjectOption_("挂机概率", std::to_string(FLAGS_chaos_p_hook));
        InjectOption_("重计时概率", std::to_string(FLAGS_chaos_p_retime));
        InjectOption_("成就概率", std::to_string(FLAGS_chaos_p_achieve));
        InjectOption_("加分概率", std::to_string(FLAGS_chaos_p_score));
        InjectOption_("直接结束概率", std::to_string(FLAGS_chaos_p_over));
        InjectOption_("失败概率", std::to_string(FLAGS_chaos_p_fail));
        InjectOption_("崩溃概率", std::to_string(FLAGS_chaos_p_crash));
    }

    std::unique_ptr<BotCtx, void(*)(void*)> bot_{nullptr, &LGTBot_Release};
    std::string conf_dir_;
};

TEST_F(ChaosTest, concurrently_create_join_play_and_leave)
{
    const uint32_t users = static_cast<uint32_t>(FLAGS_chaos_users);
    const uint64_t rounds = FLAGS_chaos_rounds;
    const std::chrono::seconds watchdog(10);
    const std::string gid = "g0";

    std::atomic<bool> stop{false};
    std::atomic<bool> deadlock{false};
    std::atomic<uint64_t> done_actions{0};

    std::vector<std::thread> workers;
    workers.reserve(users);
    for (uint32_t i = 0; i < users; ++i) {
        workers.emplace_back([this, i, rounds, &gid, &stop, &deadlock, &done_actions, &watchdog]() {
            std::mt19937 rng(static_cast<uint32_t>(FLAGS_chaos_seed + i));
            for (uint64_t r = 0; (rounds == 0 || r < rounds) && !stop.load(std::memory_order_relaxed); ++r) {
                const SimCommand cmd = NextCommand(rng, i, gid);
                if (!ExecuteWithWatchdog(*bot_, cmd, watchdog)) {
                    deadlock.store(true, std::memory_order_relaxed);
                    return;
                }
                done_actions.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Reporter thread: sysbench-style per-second creation-throughput line.
    std::thread reporter([this, &stop]() {
        const auto interval = std::chrono::seconds(FLAGS_chaos_report_interval);
        const auto t0 = std::chrono::steady_clock::now();
        uint64_t prev_total = 0;
        auto last = t0;
        while (!stop.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_chaos_sample_interval_ms));
            const auto now = std::chrono::steady_clock::now();
            if (now - last < interval) {
                continue;
            }
            const double elapsed_sec = std::chrono::duration<double>(now - last).count();
            const uint64_t total = bot_->match_manager().CreatedCount();
            const auto matches = bot_->match_manager().Matches();
            uint64_t active = 0;
            for (const auto& m : matches) {
                if (m->state() == Match::IS_STARTED) {
                    ++active;
                }
            }
            fprintf(stdout, "[%4.0fs] created/sec=%8.1f total=%5llu active=%3llu live=%3llu (users=%u)\n",
                    std::chrono::duration<double>(now - t0).count(),
                    (total - prev_total) / elapsed_sec,
                    static_cast<unsigned long long>(total),
                    static_cast<unsigned long long>(active),
                    static_cast<unsigned long long>(matches.size()),
                    static_cast<unsigned>(FLAGS_chaos_users));
            prev_total = total;
            last = now;
        }
    });

    // Wait until one of the stop conditions is satisfied.
    const auto start = std::chrono::steady_clock::now();
    while (!stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (FLAGS_chaos_duration > 0 &&
                std::chrono::steady_clock::now() - start >= std::chrono::seconds(FLAGS_chaos_duration)) {
            stop.store(true, std::memory_order_relaxed);
            break;
        }
        if (FLAGS_chaos_events > 0 && bot_->match_manager().CreatedCount() >= FLAGS_chaos_events) {
            stop.store(true, std::memory_order_relaxed);
            break;
        }
        if (FLAGS_chaos_rounds > 0 &&
                done_actions.load(std::memory_order_relaxed) >= users * rounds) {
            stop.store(true, std::memory_order_relaxed);
            break;
        }
    }
    stop.store(true, std::memory_order_relaxed);

    reporter.join();
    for (auto& w : workers) {
        w.join();
    }

    const uint64_t total_created = bot_->match_manager().CreatedCount();
    fprintf(stdout, "SUMMARY created=%llu actions=%llu live=%zu\n",
            static_cast<unsigned long long>(total_created),
            static_cast<unsigned long long>(done_actions.load()),
            bot_->match_manager().Matches().size());

    // Terminate any still-running match so the bot can tear down cleanly.
    for (auto& m : bot_->match_manager().Matches()) {
        if (m->state() == Match::IS_STARTED) {
            m->Terminate(true);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_FALSE(deadlock.load(std::memory_order_relaxed))
        << "deadlock detected; one or more requests did not return within " << watchdog.count() << "s";
    EXPECT_GT(total_created, 0u) << "no game was created during the chaos run";
}

} // namespace lgtbot::core::test

int main(int argc, char** argv)
{
    lgtbot::InstallDefaultSignalHandlersOnce();
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}