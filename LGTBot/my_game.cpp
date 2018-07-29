#include "my_game.h"

#define GAME_ID "MyGame"
#define MIN_PLAYER 6
#define MAX_PLAYER 0

enum MyStates
{
  MAIN_STATE,
  ROUND_STATE
};

// MAIN_STATE
class MyMainState : public CompState
{
public:
  MyMainState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    CompState(game, container, superstate_ptr), cur_round(0) {}

  void Start()
  {
    SwitchSubstate(ROUND_STATE);
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
};

// ROUND_STATE
class RoundState : public AtomState
{
public:
  RoundState(Game& game, StateContainer& container, GameStatePtr superstate_ptr) :
    AtomState(game, container, superstate_ptr) {}

  void Start()
  {
    game_.Broadcast("µÚ" + ("0" + ((MyMainState*) superstate_)->get_round()));
  }

  void Over()
  {

  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    
  }
};


class MyPlayer : public GamePlayer
{

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


class MyGame : public Game
{
public:
  MyGame(Match& match) : Game(match, new MyStateContainer(*this), GAME_ID, MIN_PLAYER, MAX_PLAYER) {};
};

void Bind(GameContainer& game_container)
{
  game_container.Bind<MyGame, MyPlayer>(GAME_ID);
}
