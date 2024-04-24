// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <bitset>

#include "game_framework/game_options.h" // for GameOption
#include "game_framework/game_achievements.h" // for k_achievements
#include "game_framework/util.h"
#include "game_framework/stage.h"
#include "utility/msg_checker.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

class MsgSenderBase;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

extern const char* Rule();

internal::MainStage* MakeMainStage(MainStageFactory factory);

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

namespace this_module = lgtbot::game::GAME_MODULE_NAME;

extern "C" {

// The following functions will be loaded to bot_core by its C-format symbol name.
// It is the most suitable way to expose the handler function to bot_core because the library will be linked in runtime.

lgtbot::game::GameInfo GetGameInfo()
{
    lgtbot::game::GameInfo game_info;

#define STRING_LITERAL2(name) #name
#define STRING_LITERAL(name) STRING_LITERAL2(name)
    game_info.module_name_ = STRING_LITERAL(GAME_MODULE_NAME);
#undef STRING_LITERAL
#undef STRING_LITERAL2
    game_info.game_name_ = this_module::k_game_name.c_str();
    game_info.developer_ = this_module::k_developer.c_str();
    game_info.description_ = this_module::k_description.c_str();
    game_info.achievements_.data_ = this_module::k_achievements.data();
    game_info.achievements_.size_ = this_module::k_achievements.size();

    static const auto commands_str = [](const auto& commands, const std::string& command_prefix, std::string hint)
        {
            if (commands.empty()) {
                return std::string{};
            }
            std::string s = std::move(hint);
            int i = 0;
            for (const auto& command : commands) {
                s += "\n" + std::to_string(++i) + ". ";
                s += command.Info(true, false, command_prefix + this_module::k_game_name + " ");
            }
            return s;
        };
    static const std::string rule_str = std::string(this_module::Rule()) +
        commands_str(this_module::k_rule_commands, "#规则 ", "\n\n可以通过以下指令查看规则细节：") +
        commands_str(this_module::k_init_options_commands, std::string("#新游戏 ") + this_module::k_game_name,
                "\n\n可以通过以下预设指令开启不同模式的游戏：");
    game_info.rule_ = rule_str.c_str();

    return game_info;
}

uint64_t MaxPlayerNum(const lgtbot::game::GameOptionsBase* const game_options)
{
    return MaxPlayerNum(static_cast<const this_module::GameOptions&>(*game_options));
}

uint32_t Multiple(const lgtbot::game::GameOptionsBase* const game_options)
{
    return Multiple(static_cast<const this_module::GameOptions&>(*game_options));
}

lgtbot::game::GameOptionsBase* NewGameOptions() { return new this_module::GameOptions(); }

void DeleteGameOptions(lgtbot::game::GameOptionsBase* const game_options) { delete game_options; }

lgtbot::game::MainStageBase* NewMainStage(MsgSenderBase* const reply, lgtbot::game::GameOptionsBase* const game_options,
        lgtbot::game::GenericOptions* const generic_options, MatchBase* const match)
{
    assert(game_options);
    assert(generic_options);
    assert(match);
    auto* const my_game_options = static_cast<this_module::GameOptions*>(game_options);
    if (!this_module::AdaptOptions(*reply, *my_game_options, *generic_options, *generic_options)) {
        return nullptr;
    }
    return lgtbot::game::GAME_MODULE_NAME::MakeMainStage(
            this_module::MainStageFactory{*my_game_options, *generic_options, *match});
}

void DeleteMainStage(lgtbot::game::MainStageBase* main_stage)
{
    delete main_stage;
}

const char* HandleRuleCommand(const char* const s)
{
    MsgReader reader(s);
    for (const auto& cmd : this_module::k_rule_commands) {
        if (const auto& value = cmd.CallIfValid(reader); value.has_value()) {
            return *value;
        }
    }
    return nullptr;
}

lgtbot::game::InitOptionsResult HandleInitOptionsCommand(const char* const s,
        lgtbot::game::GameOptionsBase* const game_options, lgtbot::game::MutableGenericOptions* const generic_options)
{
    assert(game_options && generic_options);
    MsgReader reader(s);
    for (const auto& cmd : this_module::k_init_options_commands) {
        if (const auto& value = cmd.CallIfValid(reader, static_cast<this_module::GameOptions&>(*game_options), *generic_options);
                value.has_value()) {
            return *value == this_module::NewGameMode::SINGLE_USER ? lgtbot::game::NEW_SINGLE_USER_MODE_GAME
                                                                   : lgtbot::game::NEW_MULTIPLE_USERS_MODE_GAME;
        }
    }
    return lgtbot::game::INVALID_INIT_OPTIONS_COMMAND;
}

} // extern "c"
