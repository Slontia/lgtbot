#include "stdafx.h"
#include "my_game.h"
/*
// ============= Define Parameters Here ==============
#define GAME_NAME     MyGame
#define PLAYER_NAME   MyPlayer
#define MIN_PLAYER    4
#define MAX_PLAYER    0
// ============= Defination Over =====================

#define TO_STR(name) #name          // cast class name to string 

#define CAST_GAME(game) ((GAME_NAME&) game)
#define CAST_PLAYER(player) ((PLAYER_NAME&) player)

// game
#define DECLARE_GAME \
class GAME_NAME : public Game\
{\
public:\
  GAME_NAME(Match& match) : Game(match, new MyStateContainer(*this), TO_STR(GAME_NAME), MIN_PLAYER, MAX_PLAYER) {}\
private:

// player
#define DECLARE_PLAYER \
class PLAYER_NAME : public GamePlayer\
{\
public:\
  PLAYER_NAME(int64_t qqid) : GamePlayer(qqid) {}\
private:

// comp state
#define DECLARE_COMP_STATE(StateName) \
class StateName : public CompState\
{\
public:\
  StateName(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :\
    CompState(game, container, superstate_ptr) {}\
private:\
  GAME_NAME& game() { return CAST_GAME(game_); }

#define DECLARE_END };



enum MyStates
{
  MAIN_STATE,
  ROUND_STATE
};

DECLARE_GAME
  // here add members

DECLARE_END

DECLARE_PLAYER
  // here add members

DECLARE_END

// MAIN_STATE

DECLARE_COMP_STATE(MyMainState)

public:
  void Start()
  {
    cur_round = 1;
    SwitchSubstate(ROUND_STATE);
    game().Broadcast("第1回合开始！");
  }

  void Over()
  {

  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    PassRequest(pid, msg, sub_type);
    return false;
  }

  bool HandleTimer()
  {
    if (cur_round == kRound)
    {
      return true;
    }
    else
    {
      cur_round++;
      SwitchSubstate(ROUND_STATE);
      game().Broadcast((string) "第" + ('0' + cur_round) + "回合开始！");
    }
    return true;
  }

protected:
  int get_round()
  {
    return cur_round;
  }

private:
  const int kRound = 6;
  int cur_round;

DECLARE_END

// ROUND_STATE
class RoundState : public AtomState
{
public:
  RoundState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    AtomState(game, container, superstate_ptr) {}

  void Start()
  {

  }

  void Over()
  {

  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    
  }
};


class MyStateContainer : public StateContainer
{
public:
  MyStateContainer(Game& game) : StateContainer(game) {}
protected:
  void BindCreators()
  {
    Bind<MyMainState>(MAIN_STATE);
    Bind<RoundState>(ROUND_STATE);
  }
};



void Bind(GameContainer& game_container)
{
  game_container.Bind<GAME_NAME, PLAYER_NAME>(TO_STR(GAME_NAME));
}
*/
