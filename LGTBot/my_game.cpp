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
class MyPlayer;

/* Stage identifiers */

#define STAGE_CLASS(stage)  stage##_Stage
#define STAGE_NAME(stage)   #stage
#define STAGE_KV(stage)     {STAGE_NAME(stage), stage_container_.get_creator<STAGE_CLASS(stage)>()}

#define DEFINE_END };
/* Player Definition */

#define DEFINE_PLAYER \
class MyPlayer : public GamePlayer \
{ \
public: \
  MyPlayer() : GamePlayer() {} \
private:

/* Stage Definitions */
#define REWRITE_OPERATE_PLAYER \
void OperatePlayer(std::function<void(MyPlayer&)> f)\
{\
  GameStage::OperatePlayer([&f](GamePlayer& p)\
  {\
    f(dynamic_cast<MyPlayer&>(p));\
  });\
}

#define DEFINE_GET_MAIN \
STAGE_CLASS(MAIN)& get_main()\
{\
  return dynamic_cast<STAGE_CLASS(MAIN)&>(main_stage_);\
}

#define DEFINE_SUBSTAGE(stage, type, super) \
class STAGE_CLASS(stage) : public type, public SuperstageRef<STAGE_CLASS(super)> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage, GameStage& superstage) : \
    type(game, main_stage), SuperstageRef<STAGE_CLASS(super)>(superstage) {} \
protected: \
  REWRITE_OPERATE_PLAYER \
  DEFINE_GET_MAIN \
private:

#define DEFINE_STAGE(stage, type) \
class STAGE_CLASS(stage) : public type, public SuperstageRef<void> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage, GameStage& superstage) : \
    type(game, main_stage) {} \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage) : \
    type(game, main_stage) {} \
protected: \
  REWRITE_OPERATE_PLAYER \
  DEFINE_GET_MAIN \
private:

/* Game Head */

#define BIND_STAGE(main, ...) \
class MyGame : public Game\
{\
public:\
  MyGame(Match& match) :\
    Game(match, GAME_NAME, MIN_PLAYER, MAX_PLAYER, std::unique_ptr<STAGE_CLASS(main)>(), {__VA_ARGS__}) {}\
};\
void Bind(GameContainer& game_container)\
{\
  game_container.Bind<MyGame, MyPlayer>(GAME_NAME);\
}

#define SWITCH_SUBSTAGE(stage) SwitchSubstage(STAGE_NAME(stage));
/*================================================================================*/
/*================================================================================*/

typedef enum { ROCK, PAPER, SCISSORS, NONE } SELECTION;

/*======== Define your player here ===========*/
DEFINE_PLAYER
public:
  const int kWinRound = 3;
  int cur_win_ = 0;
  SELECTION sel_;

  int get_score()
  {
    return 0;
  }

  void round_init()
  {
    sel_ = NONE;
  }

  bool has_sel()
  {
    return sel_ != NONE;
  }

  bool sel(SELECTION sel)
  {
    if (has_sel()) return false;
    sel_ = sel;
  }

  bool is_win()
  {
    return cur_win_ >= kWinRound;
  }
DEFINE_END

/* Define main stage here. */
DEFINE_STAGE(MAIN, CompStage)
public:
  const int winRound = 3;
  int cur_round = 0;

  void Start()
  {
    assert(!start_next_round());
  }

  void Over() {}

  bool Request(uint32_t pid, std::string msg, int32_t sub_type)
  {
    if (PassRequest(pid, msg, sub_type)) return start_next_round();
  }

  bool TimerCallback()
  {
    return start_next_round();
  }

  bool start_next_round()
  {
    bool has_winner = false;
    OperatePlayer([&has_winner](MyPlayer& p)
    {
      if (p.is_win())
      {
        assert(!has_winner);
        has_winner = true;
      }
    });
    
    if (!has_winner)
    {
      ++cur_round;
      SWITCH_SUBSTAGE(ROUND);
      return false; // continue
    }
    return true;  // finish
  }
DEFINE_END


DEFINE_SUBSTAGE(ROUND, TimerStage<5>, MAIN)
public:
  void Start()
  {
    OperatePlayer([](MyPlayer& p)
    {
      p.round_init();
    });
    Broadcast("第" + std::to_string(get_main().cur_round) + "回合开始！");
  }

  void Over() {}

  bool Request(uint32_t pid, std::string msg, int32_t sub_type)
  {
    return true;
  }

  bool TimerCallback()
  {
    return true;
  }

DEFINE_END


DEFINE_STAGE(DEPEND, TimerStage<2>)
public:
  void Start()
  {

  }

  void Over()
  {

  }

  bool Request(uint32_t pid, std::string msg, int32_t sub_type)
  {
    return true;
  }

  bool TimerCallback()
  {
    return true;
  }
DEFINE_END

BIND_STAGE(MAIN, STAGE_KV(ROUND), STAGE_KV(DEPEND))
