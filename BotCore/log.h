#pragma once

static const UserID my_uid = 654867229;

inline void LOG_INFO(std::string msg)
{
  SendPrivateMsg(my_uid, "[INFO] " + msg);
}

inline void LOG_ERROR(std::string msg)
{
  SendPrivateMsg(my_uid, "[ERROR] " + msg);
}