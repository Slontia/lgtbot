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

class MainStageBase;
class GameOptionBase;
class MatchBase;

struct GameHandle {
    using ModGuard = std::function<void()>;
    using game_options_allocator = GameOptionBase*(*)();
    using game_options_deleter = void(*)(const GameOptionBase*);
    using game_options_ptr = std::unique_ptr<GameOptionBase, game_options_deleter>;
    using main_stage_allocator = MainStageBase*(*)(MsgSenderBase&, const GameOptionBase&, MatchBase& match);
    using main_stage_deleter = void(*)(const MainStageBase*);
    using main_stage_ptr = std::unique_ptr<MainStageBase, main_stage_deleter>;

    GameHandle(const std::string& name, const std::string& module_name,
               const uint64_t max_player, const std::string& rule, const uint32_t multiple,
               const game_options_allocator& game_options_allocator_fn,
               const game_options_deleter& game_options_deleter_fn,
               const main_stage_allocator& main_stage_allocator_fn,
               const main_stage_deleter& main_stage_deleter_fn,
               ModGuard&& mod_guard)
        : name_(name)
        , module_name_(module_name)
        , max_player_(max_player)
        , rule_(rule)
        , multiple_(multiple)
        , game_options_allocator_(game_options_allocator_fn)
        , game_options_deleter_(game_options_deleter_fn)
        , main_stage_allocator_(main_stage_allocator_fn)
        , main_stage_deleter_(main_stage_deleter_fn)
        , mod_guard_(std::forward<ModGuard>(mod_guard))
    {}

    GameHandle(GameHandle&&) = delete;

    game_options_ptr make_game_options() const
    {
        return game_options_ptr(game_options_allocator_(), game_options_deleter_);
    }

    main_stage_ptr make_main_stage(MsgSenderBase& reply, const GameOptionBase& game_options, MatchBase& match) const
    {
        return main_stage_ptr(main_stage_allocator_(reply, game_options, match), main_stage_deleter_);
    }

    const std::string name_;
    const std::string module_name_;
    const uint64_t max_player_;
    const std::string rule_;
    uint32_t multiple_;
    const game_options_allocator game_options_allocator_;
    const game_options_deleter game_options_deleter_;
    const main_stage_allocator main_stage_allocator_;
    const main_stage_deleter main_stage_deleter_;
    const ModGuard mod_guard_;
};

