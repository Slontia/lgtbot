#pragma once
#include <glog/logging.h>

#define DebugLog() LOG(INFO)
#define InfoLog() LOG(INFO)
#define WarnLog() LOG(WARNING)
#define ErrorLog() LOG(ERROR)
#define FatalLog() LOG(FATAL)
