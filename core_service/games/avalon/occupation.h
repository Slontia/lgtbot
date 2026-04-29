#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(Occupation)
    // killer team
    ENUM_MEMBER(Occupation, 莫德雷德)
    ENUM_MEMBER(Occupation, 莫甘娜)
    ENUM_MEMBER(Occupation, 奥伯伦)
    ENUM_MEMBER(Occupation, 刺客)
    ENUM_MEMBER(Occupation, 莫德雷德的爪牙)
    // civilian team
    ENUM_MEMBER(Occupation, 梅林)
    ENUM_MEMBER(Occupation, 派西维尔)
    ENUM_MEMBER(Occupation, 亚瑟的忠臣)
    // special team
    ENUM_MEMBER(Occupation, 兰斯洛特)
ENUM_END(Occupation)

ENUM_BEGIN(Team)
    ENUM_MEMBER(Team, 好)
    ENUM_MEMBER(Team, 坏)
ENUM_END(Team)

#endif
#endif
#endif

#ifndef AVALON_OCCUPATION_H_
#define AVALON_OCCUPATION_H_

#include <array>
#include <optional>
#include <map>
#include <bitset>

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#define ENUM_FILE "../games/avalon/occupation.h"
#include "../../utility/extend_enum.h"

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

#endif

