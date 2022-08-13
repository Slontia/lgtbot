#ifdef INIT_OPTION_DEPEND

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(GameMap)
ENUM_MEMBER(GameMap, ace)
ENUM_MEMBER(GameMap, curiosity)
ENUM_MEMBER(GameMap, grail)
ENUM_MEMBER(GameMap, mercury)
ENUM_MEMBER(GameMap, sophie)
ENUM_MEMBER(GameMap, genius)
ENUM_MEMBER(GameMap, refraction)
ENUM_MEMBER(GameMap, gemini)
ENUM_MEMBER(GameMap, daisuke)
ENUM_MEMBER(GameMap, 随机) // must be the last one, easy for rand
ENUM_END(GameMap)

#endif
#endif
#endif

#ifndef LASER_CHESS_OPTIONS_H_
#define LASER_CHESS_OPTIONS_H_

#define ENUM_FILE "../games/laser_chess/options.h"
#include "../../utility/extend_enum.h"

#endif

#else

GAME_OPTION("每每手棋x秒超时", 局时, (ArithChecker<uint32_t>(10, 3600, "局时（秒）")), 180)
GAME_OPTION("最大回合数（两名玩家各下一次算一回合）", 回合数, (ArithChecker<uint32_t>(10, 100, "回合数")), 30)
GAME_OPTION("游戏地图", 地图, (AlterChecker<GameMap>(GameMap::ParseMap())), GameMap::随机)

#endif
