// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "game_framework/game_main.h"
#include "bot_core/subprocess.h"

// Parent-side client for the per-game config_runner subprocess.
//
// The subprocess is lazily started on first use and automatically shut down
// after kIdleTimeout of inactivity. If the subprocess dies, it is transparently
// restarted on the next request (the subprocess reads the persisted config file
// on startup, so no replay is needed).
//
// All public methods are thread-safe.
class GameConfigClient
{
  public:
    static constexpr auto kIdleTimeout = std::chrono::minutes(5);

    GameConfigClient(std::filesystem::path runner_exe,
                     std::filesystem::path game_library,
                     std::filesystem::path conf_path);
    ~GameConfigClient();

    GameConfigClient(const GameConfigClient&) = delete;
    GameConfigClient& operator=(const GameConfigClient&) = delete;

    // Returns the option info text for the game (displayed by %配置 <game>).
    // Returns empty string on failure.
    std::string QueryOptionInfo(bool text_mode);

    // Sets a default option. Updates max_player/multiple on success.
    // Returns false on failure.
    bool SetDefaultOption(const std::string& text, uint64_t& max_player, uint32_t& multiple);

    // Sets the default formal/informal flag for new matches.
    bool SetDefaultFormal(bool is_formal);

    // Queries the current default is_formal flag.
    bool QueryDefaultFormal();

    // Returns the list of applied default options (for replaying in match_game_runner at game start).
    std::vector<std::string> GetAppliedLog();

    // Parses init-options args (from #新游戏 <game> <args>).
    // Returns INVALID_INIT_OPTIONS_COMMAND on failure.
    lgtbot::game::InitOptionsResult InitOptions(const std::string& args,
                                                uint64_t& max_player,
                                                uint32_t& multiple,
                                                uint32_t& bench,
                                                bool& is_formal);

    // Shut down the subprocess if it is running (called on bot shutdown).
    void Shutdown();

  private:
    // Ensure the subprocess is running. Must be called with mutex_ held.
    [[nodiscard]] bool EnsureRunning_();

    // Serialize req to wire, send, receive response, deserialize to resp. Returns false on I/O error.
    // Must be called with mutex_ held.
    [[nodiscard]] bool SendRecv_(const std::string& req_bytes, std::string& resp_bytes);

    // Stop the subprocess unconditionally. Must be called with mutex_ held.
    void Stop_();

    // Idle-timeout watchdog thread body.
    void WatchdogRun_();

    const std::filesystem::path runner_exe_;
    const std::filesystem::path game_library_;
    const std::filesystem::path conf_path_;

    std::mutex mutex_;
    std::unique_ptr<Subprocess> proc_;
    FILE* child_in_{nullptr};
    FILE* child_out_{nullptr};

    // Idle-timeout tracking
    std::chrono::steady_clock::time_point last_use_;
    bool watchdog_stop_{false};
    std::condition_variable watchdog_cv_;
    std::mutex watchdog_cv_mutex_;
    std::thread watchdog_thread_;
};
