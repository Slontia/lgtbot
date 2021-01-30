#ifdef ERRCODE_DEF

ERRCODE_DEF_V(EC_OK, 0)
ERRCODE_DEF(EC_UNEXPECTED_ERROR)

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
ERRCODE_DEF(EC_MATCH_NEED_REQUEST_PRIVATRE)
ERRCODE_DEF(EC_MATCH_NEED_ID)
ERRCODE_DEF(EC_MATCH_ALREADY_BEGIN)
ERRCODE_DEF(EC_MATCH_NOT_BEGIN)
ERRCODE_DEF(EC_MATCH_IN_CONFIG)
ERRCODE_DEF(EC_MATCH_NOT_IN_CONFIG)
ERRCODE_DEF(EC_MATCH_TOO_FEW_PLAYER)
ERRCODE_DEF(EC_MATCH_ACHIEVE_MAX_PLAYER)
ERRCODE_DEF(EC_MATCH_UNEXPECTED_CONFIG)
ERRCODE_DEF(EC_MATCH_NO_PRIVATE_MATCH)

ERRCODE_DEF_V(EC_REQUEST_EMPTY, 301)
ERRCODE_DEF(EC_REQUEST_NOT_ADMIN)
ERRCODE_DEF(EC_REQUEST_NOT_FOUND)
ERRCODE_DEF(EC_REQUEST_UNKNOWN_GAME)

ERRCODE_DEF_V(EC_GAME_ALREADY_RELEASE, 401)

#endif

#ifndef BOT_CORE_H
#define BOT_CORE_H
#include <stdint.h>
#include <string>
#include <memory>
#include <map>

class GameHandle;
class MsgSender;

extern "C"
{
  typedef uint64_t UserID;
  typedef uint64_t GroupID;
  typedef uint64_t MatchId;

  enum Target { TO_USER, TO_GROUP };
  typedef MsgSender*(*NEW_MSG_SENDER_CALLBACK)(const Target, const UserID);
  typedef void(*DELETE_MSG_SENDER_CALLBACK)(MsgSender* const msg_sender);

  enum ErrCode
  {
#define ERRCODE_DEF_V(errcode, value) errcode = value,
#define ERRCODE_DEF(errcode) errcode,
#include "bot_core.h"
#undef ERRCODE_DEF_V
#undef ERRCODE_DEF
  };

  inline const char* errcode2str(const ErrCode errcode)
  {
    switch (errcode)
    {
#define ERRCODE_DEF(errcode) case errcode: return #errcode;
#define ERRCODE_DEF_V(errcode, value) ERRCODE_DEF(errcode)
#include "bot_core.h"
#undef ERRCODE_DEF_V
#undef ERRCODE_DEF
    default: return "Unknown Error Code";
    }
  }

#ifdef _WIN32
#define DLLEXPORT(type) __declspec(dllexport) type __cdecl
#else
#define DLLEXPORT(type) type
#endif

  class BOT_API
  {
   public:
    static DLLEXPORT(bool) Init( const UserID this_uid, const NEW_MSG_SENDER_CALLBACK new_msg_sender_cb, const DELETE_MSG_SENDER_CALLBACK delete_msg_sender_cb, int argc, char** argv);
    static DLLEXPORT(ErrCode) HandlePrivateRequest(const UserID uid, const char* const msg);
    static DLLEXPORT(ErrCode) HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg);
    static DLLEXPORT(ErrCode) ConnectDatabase(const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const char** errmsg);
  };

  extern std::map<std::string, std::unique_ptr<GameHandle>> g_game_handles;

#undef DLLEXPORT
}

#endif
