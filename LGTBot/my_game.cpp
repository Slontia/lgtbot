#include "stdafx.h"
#include "my_game.h"
#include "game_stage.h"
#include "game.h"
#include "player.h"
#include "game_container.h"

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

/* Declarations & Definitions */

#define DECLARE_STAGE(...) enum {##__VA_ARGS__

#define DEFINE_GAME(main, ...) }; \
class MyGame : public Game \
{\
public: \
  MyGame(Match& match) : \
    Game(match, GAME_NAME, MIN_PLAYER, MAX_PLAYER, std::unique_ptr<STAGE_CLASS(main)>(), {##__VA_ARGS__}) {} \
private:

#define DEFINE_PLAYER }; \
class MyPlayer : public GamePlayer \
{ \
public: \
  MyPlayer() : GamePlayer() {} \
private:

#define DEFINE_SUBSTAGE(stage, type, super) }; \
class STAGE_CLASS(stage) : public type<MyGame>, public SuperstageRef<STAGE_CLASS(super)> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& superstage) : \
    type<MyGame>(game), SuperstageRef<STAGE_CLASS(super)>(superstage) {} \
private:

#define DEFINE_STAGE(stage, type) }; \
class STAGE_CLASS(stage) : public type<MyGame>, public SuperstageRef<void> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& superstage) : type<MyGame>(game) {} \
  STAGE_CLASS(stage)(Game& game) : type<MyGame>(game) {} \
private:

#define DEFINE_END };

/*================================================================================*/
/*================================================================================*/

/* Declare stages here. */
DECLARE_STAGE
(
  MAIN,
  ROUND,
  DEPEND
)

/* Define main stage here. */
DEFINE_STAGE(MAIN, CompStage)
public:
  void Start()
  {
    cur_round = 1;
    SwitchSubstage(ROUND);
    Broadcast("第1回合开始！");
  }

  void Over()
  {

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

DEFINE_SUBSTAGE(ROUND, TimerStage, MAIN)
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

DEFINE_STAGE(DEPEND, TimerStage)
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

/*======== Define your game here. ===========*/
DEFINE_GAME
(
   MAIN,
   STAGE_KV(ROUND),
   STAGE_KV(DEPEND)
)
public:

/*======== Define your player here ===========*/
DEFINE_PLAYER
public:



DEFINE_END

void Bind(GameContainer& game_container)
{
  game_container.Bind<MyGame, MyPlayer>(GAME_NAME);
}