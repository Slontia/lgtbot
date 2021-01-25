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

    EC_DB_CONNECT_FAILED = 101,
    EC_DB_CONNECT_DENIED,
    EC_DB_INIT_FAILED,
    EC_DB_NOT_EXIST,
    EC_DB_ALREADY_CONNECTED,
    EC_DB_NOT_CONNECTED,

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
    static DLLEXPORT(void) HandlePrivateRequest(const UserID uid, const char* const msg);
    static DLLEXPORT(void) HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg);
    static DLLEXPORT(ErrCode) ConnectDatabase(const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const char** errmsg);
  };

  extern std::map<std::string, std::unique_ptr<GameHandle>> g_game_handles;

#undef DLLEXPORT
}

