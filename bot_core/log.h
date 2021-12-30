// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#if WITH_GLOG && !TEST_BOT

#ifndef GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_NO_ABBREVIATED_SEVERITIES
#endif
#include <glog/logging.h>

#define DebugLog() LOG(INFO)
#define InfoLog() LOG(INFO)
#define WarnLog() LOG(WARNING)
#define ErrorLog() LOG(ERROR)
#define FatalLog() LOG(FATAL)

#else

class EmptyLogger
{
  public:
    template <typename Arg>
    EmptyLogger& operator <<(Arg&& arg) { return *this; } 
};

#define DebugLog EmptyLogger
#define InfoLog EmptyLogger
#define WarnLog EmptyLogger
#define ErrorLog EmptyLogger
#define FatalLog EmptyLogger

#endif
