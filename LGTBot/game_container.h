#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
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
  typedef std::shared_ptr<Game> GamePtr;
  typedef std::shared_ptr<GamePlayer> PlayerPtr;
  std::map<std::string, GamePtr> game_creator_map_;
  std::map<std::string, PlayerPtr> player_creator_map_;
public:
  template <class G, class P> void Bind(std::string game_id)
  {
    game_creator_map_[game_id] = [](uint32_t p_num) -> GamePtr
    {
      return new G(p_num);
    };
    player_creator_map_[game_id] = [](int32_t p_id) -> PlayerPtr
    {
      return new P(p_id);
    };
  }
  GamePtr GetGame(std::string game_id, uint32_t p_num);  // call bool function
  PlayerPtr GetPlayer(std::string game_id, int32_t p_id);
};

 