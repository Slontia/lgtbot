// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "bot_core/game_config_client.h"

#include "match_process/ipc_frame.h"
#include "match_process/match_ipc.pb.h"
#include "utility/log.h"

GameConfigClient::GameConfigClient(std::filesystem::path runner_exe,
                                   std::filesystem::path game_library,
                                   std::filesystem::path conf_path)
    : runner_exe_(std::move(runner_exe))
    , game_library_(std::move(game_library))
    , conf_path_(std::move(conf_path))
    , last_use_(std::chrono::steady_clock::now())
    , watchdog_thread_([this] { WatchdogRun_(); })
{}

GameConfigClient::~GameConfigClient()
{
    Shutdown();
    {
        std::lock_guard<std::mutex> lk(watchdog_cv_mutex_);
        watchdog_stop_ = true;
    }
    watchdog_cv_.notify_all();
    if (watchdog_thread_.joinable()) {
        watchdog_thread_.join();
    }
}

void GameConfigClient::Shutdown()
{
    std::lock_guard<std::mutex> lk(mutex_);
    Stop_();
}

void GameConfigClient::Stop_()
{
    if (!proc_) {
        return;
    }
    // Send shutdown, best-effort
    if (child_in_) {
        lgtbot::ipc::ConfigRequest req;
        req.set_shutdown(true);
        std::string buf;
        if (req.SerializeToString(&buf)) {
            WriteFrame(child_in_, buf);
        }
    }
    proc_.reset(); // destructor kills and waits
    child_in_ = nullptr;
    child_out_ = nullptr;
}

bool GameConfigClient::EnsureRunning_()
{
    if (proc_ && proc_->ok()) {
        last_use_ = std::chrono::steady_clock::now();
        return true;
    }
    // (Re)start
    proc_.reset();
    child_in_ = nullptr;
    child_out_ = nullptr;

    std::vector<std::string> argv{
        runner_exe_.string(),
        game_library_.string(),
        conf_path_.string(),
    };
    std::string spawn_err;
    proc_ = std::make_unique<Subprocess>(std::move(argv), spawn_err);
    if (!proc_->ok()) {
        ErrorLog() << "GameConfigClient: failed to spawn config_runner: " << spawn_err;
        proc_.reset();
        return false;
    }
    child_in_ = proc_->child_stdin();
    child_out_ = proc_->child_stdout();
    last_use_ = std::chrono::steady_clock::now();
    InfoLog() << "GameConfigClient: started config_runner for " << game_library_;
    return true;
}

bool GameConfigClient::SendRecv_(const std::string& req_bytes, std::string& resp_bytes)
{
    if (!child_in_ || !child_out_) {
        return false;
    }
    if (!WriteFrame(child_in_, req_bytes)) {
        Stop_();
        return false;
    }
    if (!ReadFrame(child_out_, resp_bytes)) {
        Stop_();
        return false;
    }
    return true;
}

// Helper macro to avoid repetition: serialize req, call SendRecv_, parse resp.
// Returns false and continues the retry loop on failure.
#define PROTO_SEND_RECV(req_obj, resp_obj)                          \
    do {                                                             \
        std::string _req_bytes, _resp_bytes;                        \
        if (!(req_obj).SerializeToString(&_req_bytes)) return {};   \
        if (!SendRecv_(_req_bytes, _resp_bytes)) continue;          \
        if (!(resp_obj).ParseFromString(_resp_bytes)) {             \
            Stop_(); continue;                                       \
        }                                                            \
    } while (0)

// Variant that returns false instead of {} for bool-returning methods.
#define PROTO_SEND_RECV_BOOL(req_obj, resp_obj)                     \
    do {                                                             \
        std::string _req_bytes, _resp_bytes;                        \
        if (!(req_obj).SerializeToString(&_req_bytes)) return false;\
        if (!SendRecv_(_req_bytes, _resp_bytes)) continue;          \
        if (!(resp_obj).ParseFromString(_resp_bytes)) {             \
            Stop_(); continue;                                       \
        }                                                            \
    } while (0)

std::string GameConfigClient::QueryOptionInfo(const bool text_mode)
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return {};
        }
        lgtbot::ipc::ConfigRequest req;
        req.mutable_query_option_info()->set_text_mode(text_mode);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return {};
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (resp.has_option_info()) {
            return resp.option_info().text();
        }
    }
    return {};
}

std::string GameConfigClient::QueryMatchOptionInfo(const bool text_mode,
                                                   const std::vector<std::string>& applied_options_log,
                                                   const std::string& init_options_args)
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return {};
        }
        lgtbot::ipc::ConfigRequest req;
        auto* q = req.mutable_query_match_option_info();
        q->set_text_mode(text_mode);
        for (const auto& line : applied_options_log) {
            q->add_applied_options_log(line);
        }
        q->set_init_options_args(init_options_args);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return {};
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (resp.has_option_info()) {
            return resp.option_info().text();
        }
    }
    return {};
}

bool GameConfigClient::SetDefaultOption(const std::string& text, uint64_t& max_player, uint32_t& multiple)
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return false;
        }
        lgtbot::ipc::ConfigRequest req;
        req.mutable_set_default_option()->set_text(text);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return false;
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (!resp.has_option_update() || !resp.option_update().ok()) {
            return false;
        }
        max_player = resp.option_update().max_player();
        multiple = resp.option_update().multiple();
        return true;
    }
    return false;
}

bool GameConfigClient::SetDefaultFormal(const bool is_formal)
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return false;
        }
        lgtbot::ipc::ConfigRequest req;
        req.mutable_set_formal()->set_value(is_formal);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return false;
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        return resp.has_ack() && resp.ack().ok();
    }
    return false;
}

std::vector<std::string> GameConfigClient::GetAppliedLog()
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return {};
        }
        lgtbot::ipc::ConfigRequest req;
        req.set_get_applied_log(true);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return {};
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (!resp.has_applied_log()) {
            return {};
        }
        std::vector<std::string> log;
        for (const auto& entry : resp.applied_log().log()) {
            log.push_back(entry);
        }
        return log;
    }
    return {};
}

bool GameConfigClient::QueryDefaultFormal()
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return true; // default to formal on failure
        }
        lgtbot::ipc::ConfigRequest req;
        req.set_query_formal(true);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return true;
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (resp.has_formal()) {
            return resp.formal().is_formal();
        }
    }
    return true;
}

lgtbot::game::InitOptionsResult GameConfigClient::InitOptions(const std::string& args,
                                                               uint64_t& max_player,
                                                               uint32_t& multiple,
                                                               uint32_t& bench,
                                                               bool& is_formal)
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return lgtbot::game::INVALID_INIT_OPTIONS_COMMAND;
        }
        lgtbot::ipc::ConfigRequest req;
        req.mutable_init_options()->set_args(args);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return lgtbot::game::INVALID_INIT_OPTIONS_COMMAND;
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (!resp.has_init_options() || !resp.init_options().ok()) {
            return lgtbot::game::INVALID_INIT_OPTIONS_COMMAND;
        }
        const auto& r = resp.init_options();
        max_player = r.max_player();
        multiple = r.multiple();
        bench = r.bench();
        is_formal = r.is_formal();
        return (r.start_mode() == lgtbot::ipc::InitOptionsResp::SINGLE)
                    ? lgtbot::game::NEW_SINGLE_USER_MODE_GAME
                    : lgtbot::game::NEW_MULTIPLE_USERS_MODE_GAME;
    }
    return lgtbot::game::INVALID_INIT_OPTIONS_COMMAND;
}

std::optional<std::string> GameConfigClient::HandleRuleCommand(const std::string& args)
{
    std::lock_guard<std::mutex> lk(mutex_);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!EnsureRunning_()) {
            return std::nullopt;
        }
        lgtbot::ipc::ConfigRequest req;
        req.mutable_handle_rule_command()->set_args(args);
        lgtbot::ipc::ConfigResponse resp;
        std::string req_bytes, resp_bytes;
        if (!req.SerializeToString(&req_bytes)) return std::nullopt;
        if (!SendRecv_(req_bytes, resp_bytes)) continue;
        if (!resp.ParseFromString(resp_bytes)) { Stop_(); continue; }
        if (!resp.has_rule_command()) {
            return std::nullopt;
        }
        if (!resp.rule_command().ok()) {
            return std::nullopt;
        }
        return resp.rule_command().text();
    }
    return std::nullopt;
}

void GameConfigClient::WatchdogRun_()
{
    for (;;) {
        {
            std::unique_lock<std::mutex> lk(watchdog_cv_mutex_);
            watchdog_cv_.wait_for(lk, std::chrono::seconds(30), [this] { return watchdog_stop_; });
            if (watchdog_stop_) {
                break;
            }
        }
        std::lock_guard<std::mutex> lk(mutex_);
        if (!proc_) {
            continue;
        }
        const auto idle = std::chrono::steady_clock::now() - last_use_;
        if (idle >= kIdleTimeout) {
            InfoLog() << "GameConfigClient: idle timeout, shutting down config_runner for " << game_library_;
            Stop_();
        }
    }
}
