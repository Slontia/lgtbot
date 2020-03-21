#pragma once
#include <stdint.h>

extern "C"
{
  typedef uint64_t UserID;
  typedef uint64_t GroupID;
  typedef uint64_t MatchId;

  typedef void(*PRIVATE_MSG_CALLBACK)(const UserID&, const char*);
  typedef void(*PUBLIC_MSG_CALLBACK)(const GroupID&, const char*);
  typedef void(*AT_CALLBACK)(const UserID&, char*, const uint64_t);
  
  enum ErrCode
  {
    EC_OK = 0,

    EC_DB_GET_DRIVER_FAILED = 101,
    EC_DB_CONNECT_FAILED,
    EC_DB_NOT_EXIST,
    EC_DB_ALREADY_CONNECTED,
    EC_DB_NOT_CONNECTED,

  };

  class BOT_API
  {
   public:
    static __declspec(dllexport) bool __cdecl Init(const UserID this_uid, const PRIVATE_MSG_CALLBACK pri_msg_cb, const PUBLIC_MSG_CALLBACK pub_msg_cb, const AT_CALLBACK at_cb);
    static __declspec(dllexport) void __cdecl HandlePrivateRequest(const UserID uid, const char* const msg);
    static __declspec(dllexport) void __cdecl HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg);
    static __declspec(dllexport) ErrCode __cdecl ConnectDatabase(const char* const addr, const char* const user, const char* const passwd, const char* const db_name, const bool create_if_not_found);
  };
}


