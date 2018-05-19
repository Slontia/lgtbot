#pragma once

#include <string>
#include <map>
#include <functional>
#include "game.h"

class GamePlayer;

class GameContainer
{
private:
  std::map<std::string, std::function<void>> boot_map_;
public:
  template <class G>
  void bind(std::string game_id);     // bind boot function to map
  void NewGame(std::string game_id, uint32_t player_num, Game* game, GamePlayer* player);  // call bool function
};