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
ERRCODE_DEF(EC_MATCH_INVALID_CONFIG_VALUE)

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
#include <stddef.h>

#ifndef META_COMMAND_SIGN
#define META_COMMAND_SIGN "#"
#endif

#ifndef ADMIN_COMMAND_SIGN
#define ADMIN_COMMAND_SIGN "%"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum ErrCode {
#define ERRCODE_DEF_V(errcode, value) errcode = value,
#define ERRCODE_DEF(errcode) errcode,
#include "bot_core.h"
#undef ERRCODE_DEF_V
#undef ERRCODE_DEF
};

inline const char* errcode2str(const enum ErrCode errcode)
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

typedef enum { LGTBOT_MSG_TEXT, LGTBOT_MSG_USER_NAME, LGTBOT_MSG_USER_MENTION, LGTBOT_MSG_IMAGE } LGTBot_MessageType;

typedef struct
{
    const char* str_;
    LGTBot_MessageType type_;
} LGTBot_Message;

// All these callbacks should not be NULL when passed to initializing the bot.
typedef struct
{
    // Get the name of the user.
    // Inputs:
    //   - `handler`: The user defined handler.
    //   - `buffer`: The buffer to store the user name.
    //   - `size`: The size of the buffer.
    //   - `user_id`: The user ID, never be NULL.
    void (*get_user_name)(void* handler, char* buffer, size_t size, const char* user_id);

    // Get the name of the user.
    // Inputs:
    //   - `handler`: The user defined handler.
    //   - `buffer`: The buffer to store the user name.
    //   - `size`: The size of the buffer.
    //   - `group_id`: The group ID, never not NULL.
    //   - `user_id`: The user ID, never be NULL.
    void (*get_user_name_in_group)(void* handler, char* bufer, size_t size, const char* group_id, const char* user_id);

    // Download the avatar of the user.
    // Inputs:
    //   - `handler`: The user defined handler.
    //   - `user_id`: The user ID, never be NULL.
    //   - `dest_filename`: The filename which the downloaded avatar image saved as, never be NULL.
    // Outputs:
    //   If download successfully, return 1. Otherwise, return 0.
    int (*download_user_avatar)(void* handler, const char* user_id, const char* dest_filename);

    // Handle the messages sent by user.
    // Inputs:
    //   - `handler`: The user defined handler.
    //   - `id`: The user ID or group ID, never be NULL.
    //   - `is_to_user`: If is true, the `id` is user ID. Otherwise, the `id` is group ID.
    //   - `messages`: The messages sent by the user.
    //   - `size`: The size of the buffer.
    void (*handle_messages)(void* handler, const char* id, const int is_to_user, const LGTBot_Message* messages, const size_t size);
} LGTBot_Callback;

typedef struct
{
    // The path to the game modules, be NULL if we do not want to load any games.
    const char* game_path_;

    // The path to the sqlite database file, be NULL if we do not want to record match results.
    const char* db_path_;

    // The path to the configuration file, be NULL if we use default configurations.
    const char* conf_path_;

    // The path to store the generated images, be NULL if we save image in a default path.
    const char* image_path_;

    // The list for administor user ID, split by ',', be NULL if there are no administors.
    const char* admins_;

    // The user defined handler which will be passed to callbacks.
    void* handler_;

    // The callbacks to help sending messages.
    LGTBot_Callback callbacks_;
} LGTBot_Option;

// Get the initialized options for the bot.
// Outputs:
//   The initialized options for the bot.
LGTBot_Option LGTBot_InitOptions();

// Create a new bot with the options.
// Inputs:
//   - `options`: The pointer to options for bot, should not be NULL.
//   - `p`: The address of the pointer which will point to the error message if the bot is created failed. Callers can pass a
//          NULL value if they do not care the reasons for failure. Callers need not to free the error message.
// Outputs:
//   If the bot is created successfully, return the created bot. Otherwise, return NULL, and `p` will point to the error message
//   if `p` is not NULL.
DLLEXPORT(void*) LGTBot_Create(const LGTBot_Option* options, const char** p);

// Release the bot.
// Inputs:
//   - `bot`: The pointer to the bot, should not be NULL.
DLLEXPORT(void) LGTBot_Release(void* bot);

// Release the bot if there are no processing games.
// Inputs:
//   - `bot`: The pointer to the bot, should not be NULL.
// Outputs:
//   If the bot is released successfully, return 1. Otherwise, return 0;
DLLEXPORT(int) LGTBot_ReleaseIfNoProcessingGames(void* bot);

// To make the bot handle a message sent from a user privately.
// Inputs:
//   - `bot`: The pointer to the bot, should not be NULL.
//   - `user_id`: The user ID, should not be NULL.
//   - `msg`: The message, should not be NULL.
// Outputs:
//   The errcode. If the message is handled well, the returned errcode should be EC_OK.
DLLEXPORT(enum ErrCode) LGTBot_HandlePrivateRequest(void* bot, const char* user_id, const char* msg);

// To make the bot handle a message sent from a user in a group publicly.
// Inputs:
//   - `bot`: The pointer to the bot, should not be NULL.
//   - `group_id`: The group ID, should not be NULL.
//   - `user_id`: The user ID, should not be NULL.
//   - `msg`: The message, should not be NULL.
// Outputs:
//   The errcode. If the message is handled well, the returned errcode should be EC_OK.
DLLEXPORT(enum ErrCode) LGTBot_HandlePublicRequest(void* bot, const char* group_id, const char* user_id, const char* msg);

// To check whether a user is in a match.
// Inputs:
//   - `bot`: The pointer to the bot, should not be NULL.
//   - `user_id`: The user ID, should not be NULL.
// Outputs:
//  If the user is in a match, return 1. Otherwise, return 0;
DLLEXPORT(int) LGTBot_IsUserInMatch(void* bot, const char* user_id);

// To check whether a group is in a match.
// Inputs:
//   - `bot`: The pointer to the bot, should not be NULL.
//   - `group_id`: The group ID, should not be NULL.
// Outputs:
//  If the group is in a match, return 1. Otherwise, return 0;
DLLEXPORT(int) LGTBot_IsGroupInMatch(void* bot, const char* group_id);

// Get version.
// Output:
//   The version of the bot.
DLLEXPORT(const char*) LGTBot_Version();

#undef DLLEXPORT

#ifdef __cplusplus
}
#endif

#endif
