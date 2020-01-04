#pragma once
#include <stdint.h>

extern "C"
{
  typedef uint64_t UserID;
  typedef uint64_t GroupID;
  typedef uint64_t MatchId;

  typedef const char* (*AT_CALLBACK)(const UserID&);
  typedef void(*PRIVATE_MSG_CALLBACK)(const UserID&, const char*);
  typedef void(*PUBLIC_MSG_CALLBACK)(const GroupID&, const char*);
  
  class BOT_API
  {
    static __declspec(dllexport) bool __cdecl Init(const UserID this_uid, const PRIVATE_MSG_CALLBACK pri_msg_cb, const PUBLIC_MSG_CALLBACK pub_msg_cb, const AT_CALLBACK at_cb);
    static __declspec(dllexport) void __cdecl HandlePrivateRequest(const UserID uid, const char* const msg);
    static __declspec(dllexport) void __cdecl HandlePublicRequest(const UserID uid, const GroupID gid, const char* const msg);
  };
}


