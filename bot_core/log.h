#pragma once

#ifdef WITH_GLOG

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
