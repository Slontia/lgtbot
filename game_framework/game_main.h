// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file defines the base classes which visited by bot_core.
// This is the only file which can be included by bot_core and GAME_MODULE_NAME should not appear in this file.

#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(StageErrCode)
ENUM_MEMBER(StageErrCode, OK)
ENUM_MEMBER(StageErrCode, CHECKOUT)
ENUM_MEMBER(StageErrCode, READY)
ENUM_MEMBER(StageErrCode, FAILED)
ENUM_MEMBER(StageErrCode, CONTINUE)
ENUM_MEMBER(StageErrCode, NOT_FOUND)
ENUM_END(StageErrCode)

#endif
#endif
#endif

#ifndef GAME_MAIN_H_
#define GAME_MAIN_H_

#include <stdint.h>
#include <array>
#include <optional>
#include <map>
#include <bitset>


#include "bot_core/msg_sender.h"

#ifdef _WIN32
#define DLLEXPORT(type) __declspec(dllexport) type __cdecl
#else
#define DLLEXPORT(type) type
#endif

#define ENUM_FILE "game_framework/game_main.h"
#include "utility/extend_enum.h"

class MsgSenderForGame;
class MatchBase;

namespace lgtbot {

namespace game {

struct GameAchievement
{
    const char* name_;
    const char* description_;
};

struct GameInfo
{
    const char* game_name_;
    const char* module_name_;
    const char* rule_;
    uint64_t max_player_;
    uint32_t multiple_;
    const char* developer_;
    const char* description_;
    const GameAchievement* achievements_;
};

struct GlobalGameOption
{
    bool public_timer_alert_ = false;
};

class GameOptionBase
{
  public:
    GameOptionBase() {}
    virtual ~GameOptionBase() {}

    virtual bool SetOption(const char* msg) = 0;
    virtual void SetPlayerNum(const uint64_t player_num) = 0;
    virtual void SetResourceDir(const char* resource_dir) = 0;
    virtual void SetSavedImageDir(const char* saved_image_dir) = 0;

    virtual uint64_t PlayerNum() const = 0;
    virtual const char* ResourceDir() const = 0;
    virtual const char* SavedImageDir() const = 0;

    virtual bool ToValid(MsgSenderBase& reply) = 0;
    virtual uint64_t BestPlayerNum() const = 0;

    virtual const char* Info(bool with_example, bool with_html_syntax, const char* prefix = "") const = 0;
    virtual const char* Status() const = 0;

    virtual GameOptionBase* Copy() const = 0;
    virtual const char* const* Content() const = 0;

    GlobalGameOption global_options_;
};

class StageBase
{
  public:
    virtual ~StageBase() = default;

    virtual void HandleStageBegin() = 0;

    virtual StageErrCode HandleTimeout() = 0;

    virtual StageErrCode HandleLeave(const PlayerID pid) = 0;

    virtual StageErrCode HandleComputerAct(const uint64_t pid, const bool ready_as_user) = 0;

    virtual bool IsOver() const = 0;
};

class MainStageBase : virtual public StageBase
{
  public:
    virtual StageErrCode HandleRequest(const char* const msg, const uint64_t player_id, const bool is_public,
                                       MsgSenderBase& reply) = 0;

    virtual const char* StageInfoC() const = 0;

    virtual const char* CommandInfoC(const bool text_mode) const = 0;

    virtual int64_t PlayerScore(const PlayerID pid) const = 0;

    virtual const char* const* VerdictateAchievements(const PlayerID pid) const = 0;
};

} // namespace game

} // namespace lgtbot

#endif
