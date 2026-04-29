// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "game_framework/game_properties.h"
#include "utility/msg_checker.h"

class MsgSenderBase;

namespace lgtbot {

namespace game {

class MutableGenericOptions;
class ImmutableGenericOptions;
class GenericOptions;

namespace GAME_MODULE_NAME {

class CustomOptions;

// =====================================
//      Developer-defined constance
// =====================================

// Const variable at namespace scope has internal linkage. we need use 'extern' keyword to expose them.
extern const GameProperties k_properties;
extern const MutableGenericOptions k_default_generic_options;

// ====================================
//      Developer-defined commands
// ====================================

using RuleCommand = Command<const char* const()>; // command to show detail rules
extern const std::vector<RuleCommand> k_rule_commands;

enum class NewGameMode {
    SINGLE_USER,    // start game immediately
    MULTIPLE_USERS, // wait other users to join
};
using InitOptionsCommand = Command<NewGameMode(CustomOptions& game_options, MutableGenericOptions& generic_options)>; // command to initialize options
extern const std::vector<InitOptionsCommand> k_init_options_commands;

// =====================================
//      Developer-defined functions
// =====================================

// Validate the options and adapt them if not valid.
// The return value of false indicates the options are not valid and fail to adapt. In this scenario, the game will fail
// to start.
bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options);

// Get the maximum player numbers under the current options.
// The return value of 0 indicates there are no player number limits.
uint64_t MaxPlayerNum(const CustomOptions& options);

// Get the score multiple under the current options.
// The value of 0 indicates it is a match for practice.
uint32_t Multiple(const CustomOptions& options);

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
