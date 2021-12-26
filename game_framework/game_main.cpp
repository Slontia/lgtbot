#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "game_framework/game_main.h"
#include "game_framework/util.h"
#include "game_framework/game_options.h"

extern MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match);

extern "C" {

const bool GetGameInfo(GameInfo* game_info)
{
    if (!game_info) {
        return false;
    }
    game_info->game_name_ = k_game_name.c_str();
#ifndef GAME_MODULE_NAME
    game_info->module_name_ = "[unset_module_name]";
#else
    game_info->module_name_ = GAME_MODULE_NAME;
#endif
    game_info->rule_ = Rule();
    game_info->max_player_ = k_max_player;
    game_info->multiple_ = k_multiple;
    return true;
}

GameOptionBase* NewGameOptions() { return new GameOption(); }

void DeleteGameOptions(GameOptionBase* const game_options) { delete game_options; }

MainStageBase* NewMainStage(MsgSenderBase& reply, GameOptionBase& options, MatchBase& match)
{
    return MakeMainStage(reply, static_cast<GameOption&>(options), match);
}

void DeleteMainStage(MainStageBase* main_stage) { delete main_stage; }

}
