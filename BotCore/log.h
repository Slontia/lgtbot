#pragma once
//#include "appmain.h"
//#include "cqp.h"
#include "lgtbot.h"

static const unsigned int my_qq = 654867229;

inline void LOG_INFO(std::string msg)
{
  LGTBOT::send_private_msg(my_qq, "[INFO] " + msg);
}

inline void LOG_ERROR(std::string msg)
{
  LGTBOT::send_private_msg(my_qq, "[ERROR] " + msg);
}