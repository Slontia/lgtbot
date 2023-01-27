// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(Achievement)
#define EXTEND_ACHIEVEMENT(name, description) ENUM_MEMBER(Achievement, name)
#include "achievements.h"
#undef EXTEND_ACHIEVEMENT
ENUM_END(Achievement)

#endif
#endif
#endif

#ifndef GAME_ACHIEVEMENTS_H
#define GAME_ACHIEVEMENTS_H

#include "game_framework/util.h"
#include "game_framework/game_main.h"

#define ENUM_FILE "../game_framework/game_achievements.h"
#include "../utility/extend_enum.h"

static std::array<const GameAchievement, Achievement::Count() + 1> k_achievements = {
#define EXTEND_ACHIEVEMENT(name, description) GameAchievement{.name_ = #name, .description_ = description},
#include "achievements.h"
#undef EXTEND_ACHIEVEMENT
    GameAchievement{.name_ = nullptr, .description_ = nullptr}
};

#endif
