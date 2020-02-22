#pragma once
#include <string>

struct GameEnv;
class Stage;
class Game;

extern std::function<void(void*, const std::string&)> boardcast_f;
extern std::function<void(void*, const uint64_t, const std::string&)> tell_f;
extern std::function<std::string(void*, const uint64_t)> at_f;
extern std::function<void(void*, const std::vector<int64_t>&)> game_over_f;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;
extern const char* Rule();

std::pair<std::unique_ptr<Stage>, std::function<int64_t(uint64_t)>> MakeMainStage(const uint64_t player_num);

