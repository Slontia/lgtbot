#pragma once
#include <string>

struct GameEnv;
enum StageEnum;
template <typename GameEnv, typename StateEnum> class Stage;
template <typename GameEnv, typename StateEnum> class Game;

extern const std::string k_game_name;
extern const uint64_t k_min_player;
extern const uint64_t k_max_player;

std::unique_ptr<GameEnv> MakeGameEnv(const uint64_t player_num);
std::unique_ptr<Stage<StageEnum, GameEnv>> MakeMainStage(Game<StageEnum, GameEnv>& game);

