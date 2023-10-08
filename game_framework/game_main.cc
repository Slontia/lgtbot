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
#include "utility/msg_checker.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

class MsgSenderBase;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

extern const char* Rule();
extern MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match);

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

extern "C" {

// The following functions will be loaded to bot_core by its C-format symbol name.
// It is the most suitable way to expose the handler function to bot_core because the library will be linked in runtime.

const bool GetGameInfo(lgtbot::game::GameInfo* game_info)
{
    if (!game_info) {
        return false;
    }
#define STRING_LITERAL2(name) #name
#define STRING_LITERAL(name) STRING_LITERAL2(name)
    game_info->module_name_ = STRING_LITERAL(GAME_MODULE_NAME);
#undef STRING_LITERAL
#undef STRING_LITERAL2
    game_info->game_name_ = lgtbot::game::GAME_MODULE_NAME::k_game_name.c_str();
    game_info->max_player_ = lgtbot::game::GAME_MODULE_NAME::k_max_player;
    game_info->multiple_ = lgtbot::game::GAME_MODULE_NAME::k_multiple;
    game_info->developer_ = lgtbot::game::GAME_MODULE_NAME::k_developer.c_str();
    game_info->description_ = lgtbot::game::GAME_MODULE_NAME::k_description.c_str();
    game_info->achievements_ = lgtbot::game::GAME_MODULE_NAME::k_achievements.data();

    if (lgtbot::game::GAME_MODULE_NAME::k_rule_commands.empty()) {
        game_info->rule_ = lgtbot::game::GAME_MODULE_NAME::Rule();
    } else {
        static const std::string rule_str = lgtbot::game::GAME_MODULE_NAME::Rule() + []() -> std::string
            {
                std::string s = "\n\n可以通过以下指令查看规则细节：";
                int i = 0;
                for (const auto& command : lgtbot::game::GAME_MODULE_NAME::k_rule_commands) {
                    s += "\n" + std::to_string(++i) + ". ";
                    s += command.Info(true, false, std::string("#规则 ") + lgtbot::game::GAME_MODULE_NAME::k_game_name + " ");
                }
                return s;
            }();
        game_info->rule_ = rule_str.c_str();
    }
    return true;
}

lgtbot::game::GameOptionBase* NewGameOptions()
{
    return new lgtbot::game::GAME_MODULE_NAME::GameOption();
}

void DeleteGameOptions(lgtbot::game::GameOptionBase* const game_options)
{
    delete game_options;
}

lgtbot::game::MainStageBase* NewMainStage(MsgSenderBase& reply, lgtbot::game::GameOptionBase& options, MatchBase& match)
{
    return MakeMainStage(reply, static_cast<lgtbot::game::GAME_MODULE_NAME::GameOption&>(options), match);
}

void DeleteMainStage(lgtbot::game::MainStageBase* main_stage)
{
    delete main_stage;
}

const char* HandleRuleCommand(const char* const s)
{
    MsgReader reader(s);
    for (const auto& cmd : lgtbot::game::GAME_MODULE_NAME::k_rule_commands) {
        if (const auto& value = cmd.CallIfValid(reader); value.has_value()) {
            return *value;
        }
    }
    return nullptr;
}

} // extern "c"
