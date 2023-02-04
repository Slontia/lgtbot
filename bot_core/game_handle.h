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

namespace lgtbot {

namespace game {

class MainStageBase;
class GameOptionBase;

} // namespace lgtbot

} // namespace game

class MatchBase;

struct GameHandle
{
    using ModGuard = std::function<void()>;
    using game_options_allocator = lgtbot::game::GameOptionBase*(*)();
    using game_options_deleter = void(*)(const lgtbot::game::GameOptionBase*);
    using game_options_ptr = std::unique_ptr<lgtbot::game::GameOptionBase, game_options_deleter>;
    using main_stage_allocator = lgtbot::game::MainStageBase*(*)(MsgSenderBase&, const lgtbot::game::GameOptionBase&, MatchBase& match);
    using main_stage_deleter = void(*)(const lgtbot::game::MainStageBase*);
    using main_stage_ptr = std::unique_ptr<lgtbot::game::MainStageBase, main_stage_deleter>;

    struct Achievement
    {
        std::string name_;
        std::string description_;
    };

    GameHandle(std::string name, std::string module_name,
               const uint64_t max_player, std::string rule,
               std::vector<Achievement> achievements, const uint32_t multiple,
               std::string developer, std::string description,
               game_options_allocator game_options_allocator_fn,
               game_options_deleter game_options_deleter_fn,
               main_stage_allocator main_stage_allocator_fn,
               main_stage_deleter main_stage_deleter_fn,
               ModGuard mod_guard)
        : name_(std::move(name))
        , module_name_(std::move(module_name))
        , max_player_(max_player)
        , rule_(std::move(rule))
        , achievements_(std::move(achievements))
        , multiple_(multiple)
        , developer_(std::move(developer))
        , description_(std::move(description))
        , game_options_allocator_(std::move(game_options_allocator_fn))
        , game_options_deleter_(std::move(game_options_deleter_fn))
        , main_stage_allocator_(std::move(main_stage_allocator_fn))
        , main_stage_deleter_(std::move(main_stage_deleter_fn))
        , mod_guard_(std::move(mod_guard))
        , activity_(0)
    {}

    GameHandle(GameHandle&&) = delete;

    game_options_ptr make_game_options() const
    {
        return game_options_ptr(game_options_allocator_(), game_options_deleter_);
    }

    main_stage_ptr make_main_stage(MsgSenderBase& reply, const lgtbot::game::GameOptionBase& game_options, MatchBase& match) const
    {
        return main_stage_ptr(main_stage_allocator_(reply, game_options, match), main_stage_deleter_);
    }

    const std::string name_;
    const std::string module_name_;
    const uint64_t max_player_;
    const std::string rule_;
    const std::vector<Achievement> achievements_;
    uint32_t multiple_;
    const std::string developer_;
    const std::string description_;
    const game_options_allocator game_options_allocator_;
    const game_options_deleter game_options_deleter_;
    const main_stage_allocator main_stage_allocator_;
    const main_stage_deleter main_stage_deleter_;
    const ModGuard mod_guard_;
    std::atomic<uint64_t> activity_;
};

