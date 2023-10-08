// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "utility/msg_checker.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// Const variable at namespace scope has internal linkage. we need use 'extern' keyword to expose them.
extern const std::string k_game_name;
extern const uint64_t k_max_player;
extern const uint64_t k_multiple;
extern const std::string k_developer;
extern const std::string k_description;

using RuleCommand = Command<const char* const()>;
extern const std::vector<RuleCommand> k_rule_commands;

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
