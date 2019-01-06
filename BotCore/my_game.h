#pragma once
#include "lgtbot.h"
#include "my_game.h"
#include "game_stage.h"
#include "game.h"
#include "player.h"
#include "game_container.h"
#include "match.h"

// ============= Define Parameters Here ==============
#define GAME_NAME     kGameName
#define MIN_PLAYER    kMinPlayer
#define MAX_PLAYER    kMaxPlayer
// ============= Defination Over =====================

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

/* Operate each player. We define this function here to cast player from general to what we defined in different games. */
#define REWRITE_OPERATE_PLAYER \
void OperatePlayer(std::function<void(MyPlayer&)> f)\
{\
  GameStage::OperatePlayer([&f](GamePlayer& p)\
  {\
    f(dynamic_cast<MyPlayer&>(p));\
  });\
}

/* Get main stage. We define this function here to cast main stage from general to what we defined in different games. */
#define DEFINE_GET_MAIN \
STAGE_CLASS(MAIN)& get_main()\
{\
  return dynamic_cast<STAGE_CLASS(MAIN)&>(*game_.main_stage_);\
}

#define GET_PLAYER(pid) dynamic_cast<MyPlayer&>(get_player(pid))

/* Stage Definitions */

#define DEFINE_SUBSTAGE(stage, type, super) \
class STAGE_CLASS(stage) : public type, public SuperstageRef<STAGE_CLASS(super)> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, const std::string& name, GameStage& superstage) : \
    type(game, name), SuperstageRef<STAGE_CLASS(super)>(superstage) {} \
protected: \
  REWRITE_OPERATE_PLAYER \
  DEFINE_GET_MAIN \
private:

#define DEFINE_STAGE(stage, type) \
class STAGE_CLASS(stage) : public type, public SuperstageRef<void> \
{ \
public: \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage, const std::string& name, GameStage& superstage) : \
    type(game, main_stage, name) {} \
  STAGE_CLASS(stage)(Game& game, GameStage& main_stage, const std::string& name) : \
    type(game, main_stage, name) {} \
protected: \
  REWRITE_OPERATE_PLAYER \
  DEFINE_GET_MAIN \
private:

#define DEFINE_MAIN_STAGE(type) \
class STAGE_CLASS(MAIN) : public type, public SuperstageRef<void> \
{ \
public: \
  STAGE_CLASS(MAIN)(Game& game) : \
    type(game, STAGE_NAME(MAIN)) {} \
protected: \
  REWRITE_OPERATE_PLAYER \
private:

/* Game Head */

#define BIND_STAGE(main, ...) \
class MyGame : public Game\
{\
public:\
  MyGame(Match& match) :\
    Game(match, GAME_NAME, MIN_PLAYER, MAX_PLAYER, std::make_unique<STAGE_CLASS(main)>(*this), {__VA_ARGS__}) {}\
};\
void Bind(GameContainer& game_container)\
{\
  game_container.Bind<MyGame, MyPlayer>(GAME_NAME);\
}

#define SWITCH_SUBSTAGE(stage) SwitchSubstage(STAGE_NAME(stage));

void Bind(GameContainer& game_container);