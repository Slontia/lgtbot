#pragma once
#include <string>

struct GameEnv;
template <typename GameEnv> class Stage;
template <typename GameEnv> class Game;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;
extern const char* Rule();

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num);
std::unique_ptr<Stage<GameEnv>> MakeMainStage(Game<GameEnv>& game);

