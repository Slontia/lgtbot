#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "game_framework/game_main.h"
#include "game_framework/util.h"
#include "game_framework/game_options.h"

extern MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options);

extern "C" {

const char* GameInfo(uint64_t* max_player, const char** rule)
{
    if (!max_player || !rule) {
        return nullptr;
    }
    *max_player = k_max_player;
    *rule = Rule();
    return k_game_name.c_str();
}

GameOptionBase* NewGameOptions() { return new GameOption(); }

void DeleteGameOptions(GameOptionBase* const game_options) { delete game_options; }

MainStageBase* NewMainStage(MsgSenderBase& reply, const GameOptionBase& options)
{
    return MakeMainStage(reply, static_cast<const GameOption&>(options));
}

void DeleteMainStage(MainStageBase* main_stage) { delete main_stage; }

}
