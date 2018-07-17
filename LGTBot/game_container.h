#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include "game.h"
#include "game_state.h"

class GamePlayer;
class Match;

class GameContainer
{
private:
  typedef std::shared_ptr<Game> GamePtr;
  typedef std::shared_ptr<GamePlayer> PlayerPtr;
  std::map<std::string, std::function<GamePtr(Match&)>> game_creator_map_;
  std::map<std::string, std::function<PlayerPtr()>> player_creator_map_;
public:
  GameContainer() {}
  template <class G, class P, class SC> void Bind(std::string game_id);
  /* return game binded with state container */
  GamePtr MakeGame(std::string game_id, Match& match);
  PlayerPtr MakePlayer(std::string game_id);
};



 