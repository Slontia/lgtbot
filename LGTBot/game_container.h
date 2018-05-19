#pragma once

#include <string>
#include <map>
#include <functional>
#include "game.h"

class GamePlayer;

class GameFactory
{
public:
  virtual void ProduceGame() = 0;
  virtual void ProducePlayer() = 0;
};

class GameContainer
{
private:
  std::map<std::string, GameFactory> factory_map_;
public:
  template <class G>
  void Bind(std::string game_id, GameFactory factory);
  Game GetGame(std::string game_id, uint32_t player_num);  // call bool function
  GamePlayer GetPlayer(std::string game_id);
};

