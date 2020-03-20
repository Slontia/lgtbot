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
  
  class BOT_API
  {
   public:
    static __declspec(dllexport) bool __cdecl Init(const UserID this_uid, const PRIVATE_MSG_CALLBACK pri_msg_cb, const PUBLIC_MSG_CALLBACK pub_msg_cb, const AT_CALLBACK at_cb);
    static __declspec(dllexport) void __cdecl HandlePrivateRequest(const UserID uid, const char* const msg);
    static __declspec(dllexport) void __cdecl HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg);
  };
}


