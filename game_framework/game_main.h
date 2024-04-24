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

extern "C" {

enum InitOptionsResult {
    INVALID_INIT_OPTIONS_COMMAND, // fail to start game
    NEW_SINGLE_USER_MODE_GAME,    // start game immediately
    NEW_MULTIPLE_USERS_MODE_GAME, // wait other users to join
};

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
    const char* developer_;
    const char* description_;
    struct {
        const GameAchievement* data_;
        uint32_t size_;
    } achievements_;
};

struct ImmutableGenericOptions
{
    bool public_timer_alert_ = false; // The value of true indicates that users will be alerted in group, but not by
                                      // private messages.
    uint32_t user_num_{0}; // The number of users.
    const char* resource_dir_{nullptr}; // The directory that stores resources such as pictures.
    const char* saved_image_dir_{nullptr}; // The directory to save intermediate images which do not be sent to users.
};

struct MutableGenericOptions
{
    uint32_t player_num_each_user_{1}; // [not used] The number of players controlled by each user
    uint32_t bench_computers_to_player_num_{0}; // The minimal player number. If it is greater than the current player
                                                // number, computers will be added to the game.
    uint8_t is_formal_{1}; // The value of 0 indicates the result of this match will not be recorded. The other values
                           // indicate the result will be recorded.
};

struct GenericOptions : public ImmutableGenericOptions, public MutableGenericOptions
{
    GenericOptions() = default;

    GenericOptions(const ImmutableGenericOptions& immutable_options, const MutableGenericOptions& mutable_options)
        : ImmutableGenericOptions(immutable_options)
        , MutableGenericOptions(mutable_options)
    {
    }

    uint32_t PlayerNum() const { return std::max(user_num_ * player_num_each_user_, bench_computers_to_player_num_); }
};

}

class GameOptionsBase
{
  public:
    virtual ~GameOptionsBase() = default;

    virtual bool SetOption(const char* msg) = 0;

    virtual const char* Info(bool with_example, bool with_html_syntax, const char* prefix = "") const = 0;
    virtual const char* const* ShortInfo() const = 0;

    virtual GameOptionsBase* Copy() const = 0;
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
