#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(Occupation)
    // killer team
    ENUM_MEMBER(Occupation, 杀手)
    ENUM_MEMBER(Occupation, 替身)
    ENUM_MEMBER(Occupation, 恶灵)
    ENUM_MEMBER(Occupation, 刺客)
    // civilian team
    ENUM_MEMBER(Occupation, 平民)
    ENUM_MEMBER(Occupation, 圣女)
    ENUM_MEMBER(Occupation, 侦探)
    ENUM_MEMBER(Occupation, 灵媒)
    ENUM_MEMBER(Occupation, 守卫)
    // special team
    ENUM_MEMBER(Occupation, 内奸)
    // npc
    ENUM_MEMBER(Occupation, 人偶)
ENUM_END(Occupation)


ENUM_BEGIN(Team)
    ENUM_MEMBER(Team, 杀手)
    ENUM_MEMBER(Team, 平民)
    ENUM_MEMBER(Team, 特殊)
ENUM_END(Team)

#endif
#endif
#endif

#ifndef HP_KILLER_OCCUPATION_H_
#define HP_KILLER_OCCUPATION_H_

#define ENUM_FILE "../games/hp_killer/occupation.h"
#include "../../utility/extend_enum.h"

#endif

