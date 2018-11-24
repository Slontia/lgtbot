#include "stdafx.h"
#include "my_game.h"
#include "game_stage.h"
#include "game.h"
#include "player.h"
#include "game_container.h"

// ============= Define Parameters Here ==============
#define GAME_NAME     "Demo"
#define PLAYER_NAME   MyPlayer
#define MIN_PLAYER    4
#define MAX_PLAYER    0
// ============= Defination Over =====================

class MyGame;

enum
{
  ROUND_STAGE, DEPEND_STAGE
};

class MainStage : public CompStage<MyGame>, SuperstageRef<void>
{
public:
  MainStage(Game& game) : CompStage<MyGame>(game) {}

  template <class MyGame>
  void Start()
  {
    cur_round = 1;
    SwitchSubstage(ROUND_STAGE);
    game_.Broadcast("第1回合开始！");
  }

  void Over()
  {

  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    PassRequest(pid, msg, sub_type);
    return false;
  }

  template <class MyGame>
  bool HandleTimer()
  {
    if (cur_round == kRound)
    {
      return true;
    }
    else
    {
      cur_round++;
      SwitchSubstage(ROUND_STAGE);
      game_.Broadcast((string) "第" + std::to_string('0' + cur_round) + "回合开始！");
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

};

// ROUND_STATE
class RoundStage : public TimerStage<MyGame>, public SuperstageRef<MainStage>
{
public:
  RoundStage(Game& game, GameStage& superstage) :
    TimerStage<MyGame>(game), SuperstageRef<MainStage>(superstage) {}

  void Start()
  {

  }

  void Over()
  {

  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    return true;
  }

  bool TimerCallback()
  {
    return true;
  }
};

class DependStage : public TimerStage<MyGame>, public SuperstageRef<void>
{
public:
  DependStage(Game& game, GameStage& superstage) : TimerStage<MyGame>(game) {}

  void Start()
  {

  }

  void Over()
  {

  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    return true;
  }

  bool TimerCallback()
  {
    return true;
  }
};

class MyGame : public Game
{
public:
  MyGame(Match& match) :
    Game(match, GAME_NAME, MIN_PLAYER, MAX_PLAYER, std::unique_ptr<MainStage>(),
    {
      {DEPEND_STAGE, stage_container_.get_creator<DependStage>()},
      {ROUND_STAGE, stage_container_.get_creator<RoundStage>()}
    }) {}
};

class MyPlayer : public GamePlayer
{
public:
  MyPlayer() : GamePlayer() {}
};

void Bind(GameContainer& game_container)
{
  game_container.Bind<MyGame, MyPlayer>(GAME_NAME);
}

