// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cassert>

#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "game_framework/game_main.h"
#include "bot_core/game_config_client.h"
#include "utility/lock_wrapper.h"

class GameHandle
{
  public:
    // Static information loaded once at startup (via short-lived dlopen).
    struct BasicInfo : public lgtbot::game::GameProperties
    {
        BasicInfo() = default;
        BasicInfo(const lgtbot::game::GameProperties& properties)
            : GameProperties(properties)
            , name_owned_(properties.name_ ? properties.name_ : "")
            , developer_owned_(properties.developer_ ? properties.developer_ : "")
            , description_owned_(properties.description_ ? properties.description_ : "")
        {
            name_ = name_owned_.c_str();
            developer_ = developer_owned_.c_str();
            description_ = description_owned_.c_str();
        }
        BasicInfo(const BasicInfo& other)
            : GameProperties(other)
            , name_owned_(other.name_owned_)
            , developer_owned_(other.developer_owned_)
            , description_owned_(other.description_owned_)
            , module_name_(other.module_name_)
            , rule_(other.rule_)
            , achievements_(other.achievements_)
        {
            name_ = name_owned_.c_str();
            developer_ = developer_owned_.c_str();
            description_ = description_owned_.c_str();
        }
        BasicInfo(BasicInfo&& other)
            : GameProperties(other)
            , name_owned_(std::move(other.name_owned_))
            , developer_owned_(std::move(other.developer_owned_))
            , description_owned_(std::move(other.description_owned_))
            , module_name_(std::move(other.module_name_))
            , rule_(std::move(other.rule_))
            , achievements_(std::move(other.achievements_))
        {
            name_ = name_owned_.c_str();
            developer_ = developer_owned_.c_str();
            description_ = description_owned_.c_str();
        }
        BasicInfo& operator=(const BasicInfo&) = delete;
        BasicInfo& operator=(BasicInfo&&) = delete;

        std::string name_owned_;
        std::string developer_owned_;
        std::string description_owned_;
        std::string module_name_;
        std::string rule_;

        struct Achievement
        {
            std::string name_;
            std::string description_;
        };
        std::vector<Achievement> achievements_;
    };

    // These type aliases are still needed by match_game_runner (child process),
    // but NOT by bot_core (parent process). The function pointers themselves are
    // never stored in GameHandle.
    using main_stage_allocator = lgtbot::game::MainStageBase*(*)(MsgSenderBase*, lgtbot::game::GameOptionsBase*, lgtbot::game::GenericOptions*, MatchBase*);
    using main_stage_deleter   = void(*)(const lgtbot::game::MainStageBase*);
    using game_options_allocator = lgtbot::game::GameOptionsBase*(*)();
    using game_options_deleter   = void(*)(const lgtbot::game::GameOptionsBase*);

    using game_options_ptr = std::unique_ptr<lgtbot::game::GameOptionsBase, game_options_deleter>;
    using main_stage_ptr   = std::unique_ptr<lgtbot::game::MainStageBase, main_stage_deleter>;

    GameHandle(BasicInfo info,
               uint64_t default_max_player,
               uint32_t default_multiple,
               std::unique_ptr<GameConfigClient> config_client)
        : info_(std::move(info))
        , cached_max_player_(default_max_player)
        , cached_multiple_(default_multiple)
        , config_client_(std::move(config_client))
    {}

    GameHandle(const GameHandle&) = delete;
    GameHandle(GameHandle&&) = delete;

    const BasicInfo& Info() const { return info_; }

    uint64_t CachedMaxPlayer() const { return cached_max_player_.load(); }
    uint32_t CachedMultiple()  const { return cached_multiple_.load(); }

    void UpdateCachedLimits(const uint64_t max_player, const uint32_t multiple)
    {
        cached_max_player_.store(max_player);
        cached_multiple_.store(multiple);
    }

    GameConfigClient& ConfigClient() { return *config_client_; }
    GameConfigClient& ConfigClient() const { return *config_client_; }

    void IncreaseActivity(const uint64_t count) { activity_ += count; }
    uint64_t Activity() const { return activity_; }

  private:
    BasicInfo info_;
    std::atomic<uint64_t> cached_max_player_;
    std::atomic<uint32_t> cached_multiple_;
    std::unique_ptr<GameConfigClient> config_client_;
    std::atomic<uint64_t> activity_{0};
};

using GameHandleMap = std::map<std::string, GameHandle>;
