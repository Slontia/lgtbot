#include "stdafx.h"
#include "my_game.h"
#include "game_stage.h"
#include "game.h"
#include "player.h"
#include "game_container.h"
#include "match.h"

// ============= Define Parameters Here ==============
#define GAME_NAME     "演示"
#define GAME_ID       Demo
#define MIN_PLAYER    4
#define MAX_PLAYER    0
// ============= Defination Over =====================

class MyGame;

/* Stage identifiers */

#define STAGE_CLASS(stage)  stage##_Stage
#define STAGE_KV(stage)     {stage, stage_container_.get_creator<STAGE_CLASS(stage)>()}

/* Player Definition */

#define DEFINE_PLAYER }; \
class MyPlayer : public GamePlayer \
{ \
public: \
  MyPlayer() : GamePlayer() {} \
private:

/* Stage Definitions */

#define COMP_STAGE CompStage<MyGame>
#define TIMER_STAGE(sec) TimerStage<MyGame, sec>

#define DEFINE_SUBSTAGE(stage, type, super) }; \
class STAGE_CLASS(stage) : public type, public SuperstageRef<STAGE_CLASS(super)> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage, GameStage& superstage) : \
    type(game, main_stage), SuperstageRef<STAGE_CLASS(super)>(superstage) {} \
private:

#define DEFINE_STAGE(stage, type) }; \
class STAGE_CLASS(stage) : public type, public SuperstageRef<void> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage, GameStage& superstage) : \
    type(game, main_stage) {} \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage) : \
    type(game, main_stage) {} \
private:

/* Game Head */

#define OUTSIDE(inside) \
enum { MAIN = 0, inside }) {} };\
void Bind(GameContainer& game_container)\
{\
  game_container.Bind<MyGame, MyPlayer>(GAME_NAME);\
}

#define DECLARE_STAGE(stage, inside) stage, inside STAGE_KV(stage),

#define INSIDE(...) \
__VA_ARGS__ }; \
class MyGame : public Game \
{\
public: \
  MyGame(Match& match) : \
  Game(match, GAME_NAME, MIN_PLAYER, MAX_PLAYER, std::unique_ptr<STAGE_CLASS(MAIN)>(), {

/*================================================================================*/
/*================================================================================*/

OUTSIDE(
  /* Declare stages here. */
  DECLARE_STAGE(ROUND,
  DECLARE_STAGE(DEPEND,
INSIDE(

/* Define main stage here. */
DEFINE_STAGE(MAIN, COMP_STAGE)
public:
  void Start()
  {
    cur_round = 1;
    SwitchSubstage(ROUND);
    Broadcast("第1回合开始！");
  }

  void Over()
  {
    // 写逻辑
  }

  bool Request(int32_t pid, std::string msg, int32_t sub_type)
  {
    PassRequest(pid, msg, sub_type);
    return false;
  }

  bool TimerCallback()
  {
    if (cur_round == kRound)
    {
      return true;
    }
    else
    {
      cur_round++;
      SwitchSubstage(ROUND);
      Broadcast((string) "第" + std::to_string('0' + cur_round) + "回合开始！");
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




DEFINE_SUBSTAGE(ROUND, TIMER_STAGE(5), MAIN)
public:
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




DEFINE_STAGE(DEPEND, TIMER_STAGE(2))
public:
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

  /*======== Define your player here ===========*/
  DEFINE_PLAYER
public:
  int get_score()
  {
    return 0;
  }

))))
