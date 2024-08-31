// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file defines the base classes which visited by bot_core.
// This is the only file which can be included by bot_core and GAME_MODULE_NAME should not appear in this file.

#pragma once

namespace lgtbot {

namespace game {

extern "C" {

struct GameProperties
{
    const char* name_;               // The game name which should be unique among all the games.
    const char* developer_;          // The game developer which can be shown in the game list image.
    const char* description_;        // The game description which can be shown in the game list image.
    bool shuffled_player_id_{false}; // The true value indicates each user may be assigned with different player IDs in different matches
};

} // namespace game

} // namespace lgtbot

}

