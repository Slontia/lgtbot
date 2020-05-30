#pragma once
#include "game_options.h"
#include "msg_guard.h"
#include <string>

class MainStageBase;
class Game;

extern std::function<void(void*, const std::string&)> boardcast_f;
extern std::function<void(void*, const uint64_t, const std::string&)> tell_f;
extern std::function<std::string(void*, const uint64_t)> at_f;
extern std::function<void(void*, const std::vector<int64_t>&)> game_over_f;
extern std::function<void(void*, const uint64_t)> start_timer_f;
extern std::function<void(void*)> stop_timer_f;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;
extern const char* Rule();

using reply_type = std::function<MsgGuard<std::function<void(const std::string&)>>()>;
std::unique_ptr<MainStageBase> MakeMainStage(const uint64_t player_num, const GameOption& options);

