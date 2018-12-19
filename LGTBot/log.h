#pragma once
#include "appmain.h"
#include "cqp.h"

const unsigned int my_qq = 654867229;

inline void LOG_INFO(std::string msg)
{
  CQ_sendPrivateMsg(-1, my_qq, ("[INFO] " + msg).c_str());
}

inline void LOG_ERROR(std::string msg)
{
  CQ_sendPrivateMsg(-1, my_qq, ("[ERROR] " + msg).c_str());
}