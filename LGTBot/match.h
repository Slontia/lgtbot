#pragma once

#include <iostream>
#include <map>
#include "game_container.h"
#include "game.h"

typedef enum
{
  SUCC,
  TOO_MANY_PLAYERS, 
  TOO_FEW_PLAYERS,
  GAME_STARTED,
  HAS_JOINED,
  NOT_JOINED
} ErrNo;

class GamePlayer;     // player info
class Game;



class Match
{
public:
  Match(std::string game_id, int64_t host_qq);
  int         MsgHandle(int64_t from_qq, const char* msg, int32_t subType);
  /* switch status, create game */
  int                GameStart();
  /* send msg to a specific player */
  void                                Reply(uint32_t pid, std::string) const;
  /* send msg to all player */
  void                                Broadcast(std::string msg) const;
  /* bind to map, create GamePlayer and send to game */
  int                              Join(int64_t qq);
  int                              Leave(int64_t qq);

  bool has_qq(int64_t qq);
  
private:
  
  enum { PREPARE, GAMING, OVER }    status_;
  std::string                       game_id_;
  int64_t                           host_qq_;
  std::unique_ptr<Game>             game_;
  std::map<int64_t, uint32_t>       qq2pid_;
  std::vector<int64_t>              qq_list_;
  uint32_t                          player_count_;
};  