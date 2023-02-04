// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file should be included at the bottom of each game's mygame.cc file.

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

extern const std::string k_game_name;
extern const uint64_t k_max_player;
extern const uint64_t k_multiple;
extern const std::string k_developer;
extern const std::string k_description;

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match)
{
    if (!options.ToValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

