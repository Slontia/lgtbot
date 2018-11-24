#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>

class GamePlayer;
class Match;
class Game;

class GameContainer
{
private:
  typedef std::unique_ptr<Game> GamePtr;
  typedef std::shared_ptr<GamePlayer> PlayerPtr;
  std::map<std::string, std::function<GamePtr(Match&)>> game_creator_map_;
  std::map<std::string, std::function<PlayerPtr()>> player_creator_map_;
public:
  GameContainer() {}
  template <class G, class P> void Bind(std::string game_id)
  {
    game_creator_map_[game_id] = [](Match& match) -> GamePtr
    {
      return std::make_unique<G>(match);
    };
    player_creator_map_[game_id] = []() -> PlayerPtr
    {
      return std::make_shared<P>();
    };
  }
  /* return game binded with state container */
  GamePtr MakeGame(std::string game_id, Match& match);
  PlayerPtr MakePlayer(std::string game_id);
};



 