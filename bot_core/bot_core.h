// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ERRCODE_DEF

ERRCODE_DEF_V(EC_OK, 0)
ERRCODE_DEF(EC_UNEXPECTED_ERROR)
ERRCODE_DEF(EC_NOT_INIT)
ERRCODE_DEF(EC_INVALID_ARGUMENT)

// system error
ERRCODE_DEF_V(EC_DB_CONNECT_FAILED, 101)
ERRCODE_DEF(EC_DB_CONNECT_DENIED)
ERRCODE_DEF(EC_DB_INIT_FAILED)
ERRCODE_DEF(EC_DB_NOT_EXIST)
ERRCODE_DEF(EC_DB_ALREADY_CONNECTED)
ERRCODE_DEF(EC_DB_NOT_CONNECTED)
ERRCODE_DEF(EC_DB_RELEASE_GAME_FAILED)

// user error
ERRCODE_DEF_V(EC_MATCH_NOT_EXIST, 201)
ERRCODE_DEF(EC_MATCH_USER_ALREADY_IN_MATCH)
ERRCODE_DEF(EC_MATCH_USER_NOT_IN_MATCH)
ERRCODE_DEF(EC_MATCH_USER_ALREADY_IN_OTHER_MATCH)
ERRCODE_DEF(EC_MATCH_NOT_THIS_GROUP)
ERRCODE_DEF(EC_MATCH_GROUP_NOT_IN_MATCH)
ERRCODE_DEF(EC_MATCH_NOT_HOST)
ERRCODE_DEF(EC_MATCH_NEED_REQUEST_PUBLIC)
ERRCODE_DEF(EC_MATCH_NEED_REQUEST_PRIVATE)
ERRCODE_DEF(EC_MATCH_NEED_ID)
ERRCODE_DEF(EC_MATCH_ALREADY_BEGIN)
ERRCODE_DEF(EC_MATCH_NOT_BEGIN)
ERRCODE_DEF(EC_MATCH_IN_CONFIG)
ERRCODE_DEF(EC_MATCH_NOT_IN_CONFIG)
ERRCODE_DEF(EC_MATCH_ACHIEVE_MAX_PLAYER)
ERRCODE_DEF(EC_MATCH_UNEXPECTED_CONFIG)
ERRCODE_DEF(EC_MATCH_ALREADY_OVER)
ERRCODE_DEF(EC_MATCH_FORBIDDEN_OPERATION)
ERRCODE_DEF(EC_MATCH_ELIMINATED)
ERRCODE_DEF(EC_MATCH_SCORE_NOT_ENOUGH)
ERRCODE_DEF(EC_MATCH_SINGLE_MULTT_MATCH_NOT_ENOUGH)

ERRCODE_DEF_V(EC_REQUEST_EMPTY, 301)
ERRCODE_DEF(EC_REQUEST_NOT_ADMIN)
ERRCODE_DEF(EC_REQUEST_NOT_FOUND)
ERRCODE_DEF(EC_REQUEST_UNKNOWN_GAME)

ERRCODE_DEF_V(EC_GAME_ALREADY_RELEASE, 401)
ERRCODE_DEF(EC_USER_SUICIDE_FAILED)

ERRCODE_DEF_V(EC_HONOR_ADD_FAILED, 501)
ERRCODE_DEF(EC_HONOR_DELETE_FAILED)

ERRCODE_DEF_V(EC_GAME_REQUEST_OK, 10000)
ERRCODE_DEF(EC_GAME_REQUEST_CHECKOUT)
ERRCODE_DEF(EC_GAME_REQUEST_FAILED)
ERRCODE_DEF(EC_GAME_REQUEST_CONTINUE)
ERRCODE_DEF(EC_GAME_REQUEST_NOT_FOUND)
ERRCODE_DEF(EC_GAME_REQUEST_UNKNOWN)

#endif

#ifndef BOT_CORE_H
#define BOT_CORE_H

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <filesystem>

#include "bot_core/id.h"

class GameHandle;

extern "C" {

enum ErrCode {
#define ERRCODE_DEF_V(errcode, value) errcode = value,
#define ERRCODE_DEF(errcode) errcode,
#include "bot_core.h"
#undef ERRCODE_DEF_V
#undef ERRCODE_DEF
};

inline const char* errcode2str(const ErrCode errcode)
{
    switch (errcode) {
#define ERRCODE_DEF(errcode) \
    case errcode:            \
        return #errcode;
#define ERRCODE_DEF_V(errcode, value) ERRCODE_DEF(errcode)
#include "bot_core.h"
#undef ERRCODE_DEF_V
#undef ERRCODE_DEF
    default:
        return "Unknown Error Code";
    }
}

#ifdef _WIN32
#define DLLEXPORT(type) __declspec(dllexport) type __cdecl
#else
#define DLLEXPORT(type) type
#endif

struct BotOption
{
    const char* this_uid_ = nullptr; // the user id of the robot, should not be null
    const char* game_path_ = nullptr; // should not be null
    const char* image_path_ = nullptr; // should not be null
    const char* admins_ = nullptr; // an list for admin user id, split by ',', should not be null
    const char* db_path_ = nullptr;
    const char* conf_path_ = nullptr;
};

class BOT_API
{
   public:
    static DLLEXPORT(void*) Init(const BotOption* option);
    static DLLEXPORT(void) Release(void* bot);
    static DLLEXPORT(bool) ReleaseIfNoProcessingGames(void* bot);
    static DLLEXPORT(ErrCode) HandlePrivateRequest(void* bot, const char* uid, const char* msg);
    static DLLEXPORT(ErrCode) HandlePublicRequest(void* bot, const char* gid, const char* uid, const char* msg);
};

#undef DLLEXPORT

}

#endif
