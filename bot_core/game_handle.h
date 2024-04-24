// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cassert>

#include <functional>
#include <optional>
#include <string>
#include <memory>
#include <filesystem>

#include "image.h"
#include "game_framework/game_main.h"
#include "utility/lock_wrapper.h"

namespace lgtbot {

namespace game {

class MainStageBase;

} // namespace lgtbot

} // namespace game

class MatchBase;

class GameHandle
{
  public:
    // The type of these function pointers should be corresponding to what defined in game_framework/game_main.cc.
    // TODO: Check the type correctness in compiling time.
    using max_player_num_handler = uint64_t(*)(const lgtbot::game::GameOptionsBase*);
    using multiple_handler = uint32_t(*)(const lgtbot::game::GameOptionsBase*);
    using rule_command_handler = const char*(*)(const char* const s);
    using init_options_command_handler = lgtbot::game::InitOptionsResult(*)(const char*, lgtbot::game::GameOptionsBase*, lgtbot::game::MutableGenericOptions*);
    using game_options_allocator = lgtbot::game::GameOptionsBase*(*)();
    using game_options_deleter = void(*)(const lgtbot::game::GameOptionsBase*);
    using main_stage_allocator = lgtbot::game::MainStageBase*(*)(MsgSenderBase*, lgtbot::game::GameOptionsBase*, lgtbot::game::GenericOptions*, MatchBase* match);
    using main_stage_deleter = void(*)(const lgtbot::game::MainStageBase*);

    struct Achievement
    {
        std::string name_;
        std::string description_;
    };

    // This information is defined by game.
    struct BasicInfo
    {
        std::string name_;
        std::string module_name_;
        std::string developer_;
        std::string description_;
        std::string rule_;
        std::vector<Achievement> achievements_;

        max_player_num_handler max_player_num_fn_{nullptr};
        multiple_handler multiple_fn_{nullptr};

        rule_command_handler handle_rule_command_fn_{nullptr};
        init_options_command_handler handle_init_options_command_fn_{nullptr};
    };

    struct InternalHandler
    {
        game_options_allocator game_options_allocator_{nullptr};
        game_options_deleter game_options_deleter_{nullptr};
        main_stage_allocator main_stage_allocator_{nullptr};
        main_stage_deleter main_stage_deleter_{nullptr};
        std::function<void()> mod_guard_;
    };

    GameHandle(const BasicInfo& info, const InternalHandler& internal_handler)
        : info_(info), internal_handler_(internal_handler)
    {
    }

    GameHandle(const GameHandle&) = delete;
    GameHandle(GameHandle&&) = delete;

    ~GameHandle() { internal_handler_.mod_guard_(); }

    using game_options_ptr = std::unique_ptr<lgtbot::game::GameOptionsBase, game_options_deleter>;

    struct Options
    {
        game_options_ptr game_options_;
        lgtbot::game::MutableGenericOptions generic_options_;
    };

    Options CopyDefaultGameOptions() const
    {
        const auto locked_options = default_options_.Lock();
        return Options{
            .game_options_ = game_options_ptr(locked_options->game_options_->Copy(), internal_handler_.game_options_deleter_),
            .generic_options_ = locked_options->generic_options_,
        };
    }

    LockWrapper<Options>& DefaultGameOptions() { return default_options_; }
    const LockWrapper<Options>& DefaultGameOptions() const { return default_options_; }

    using main_stage_ptr = std::unique_ptr<lgtbot::game::MainStageBase, main_stage_deleter>;

    main_stage_ptr MakeMainStage(MsgSenderBase& reply, lgtbot::game::GameOptionsBase& game_options, lgtbot::game::GenericOptions& generic_options, MatchBase& match) const
    {
        return main_stage_ptr(internal_handler_.main_stage_allocator_(&reply, &game_options, &generic_options, &match), internal_handler_.main_stage_deleter_);
    }

    void IncreaseActivity(const uint64_t count) { activity_ += count; }
    uint64_t Activity() const { return activity_; }

    const BasicInfo& Info() const { return info_; }

  private:
    BasicInfo info_;
    InternalHandler internal_handler_;
    LockWrapper<Options> default_options_ = Options{
        game_options_ptr{internal_handler_.game_options_allocator_(), internal_handler_.game_options_deleter_},
        lgtbot::game::GenericOptions{}
    };
    std::atomic<uint64_t> activity_{0}; // the sum of the number of times all users participated in this game
};

