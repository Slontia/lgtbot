#pragma once
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
    EC_OK = 0,
    EC_UNEXPECTED_ERROR = 1,

    // system error
    EC_DB_CONNECT_FAILED = 101,
    EC_DB_CONNECT_DENIED,
    EC_DB_INIT_FAILED,
    EC_DB_NOT_EXIST,
    EC_DB_ALREADY_CONNECTED,
    EC_DB_NOT_CONNECTED,
    EC_DB_RELEASE_GAME_FAILED,

    // user error
    EC_MATCH_NOT_EXIST = 201,
    EC_MATCH_USER_ALREADY_IN_MATCH,
    EC_MATCH_USER_NOT_IN_MATCH,
    EC_MATCH_USER_ALREADY_IN_OTHER_MATCH,
    EC_MATCH_NOT_THIS_GROUP,
    EC_MATCH_GROUP_NOT_IN_MATCH,
    EC_MATCH_NOT_HOST,
    EC_MATCH_NEED_REQUEST_PUBLIC,
    EC_MATCH_NEED_REQUEST_PRIVATRE,
    EC_MATCH_NEED_ID,
    EC_MATCH_ALREADY_BEGIN,
    EC_MATCH_NOT_BEGIN,
    EC_MATCH_IN_CONFIG,
    EC_MATCH_NOT_IN_CONFIG,
    EC_MATCH_TOO_FEW_PLAYER,
    EC_MATCH_ACHIEVE_MAX_PLAYER,
    EC_MATCH_UNEXPECTED_CONFIG,
    EC_MATCH_NO_PRIVATE_MATCH,

    EC_REQUEST_EMPTY = 301,
    EC_REQUEST_NOT_ADMIN,
    EC_REQUEST_NOT_FOUND,
    EC_REQUEST_UNKNOWN_GAME,

    EC_GAME_ALREADY_RELEASE = 401,
  };

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

