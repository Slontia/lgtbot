// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

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
    virtual const char* PlayerName(const PlayerID& pid) = 0;
    virtual void StartTimer(const uint64_t sec, void* p, void(*cb)(void*, uint64_t)) = 0;
    virtual void StopTimer() = 0;
    virtual void Eliminate(const PlayerID pid) = 0;
};
