#pragma once

#include "bot_core/id.h"

class MsgSenderBase;

// Cross-module class interface for game level.
class MatchBase
{
  public:
    MatchBase() {}
    virtual ~MatchBase() {}
    virtual MsgSenderBase& BoardcastMsgSender() = 0;
    virtual MsgSenderBase& TellMsgSender(const PlayerID pid) = 0;
    virtual void StartTimer(const uint64_t sec) = 0;
    virtual void StopTimer() = 0;
};
