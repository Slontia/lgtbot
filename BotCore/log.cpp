#include "log.h"
#include <glog/logging.h>
#include "util.h"

std::vector<std::unique_ptr<Logger>> loggers_;

PrivateMsgLogger::PrivateMsgLogger(const uint64_t uid) : uid_(uid) {}

void PrivateMsgLogger::Log(const LogLevel level, std::string_view msg) const
{
  switch (level)
  {
  case LogLevel::I: SendPrivateMsg(uid_, std::string("[INFO] ") + std::string(msg)); break;
  case LogLevel::W: SendPrivateMsg(uid_, std::string("[WARN] ") + std::string(msg)); break;
  case LogLevel::E: SendPrivateMsg(uid_, std::string("[ERROR] ") + std::string(msg)); break;
  case LogLevel::F: SendPrivateMsg(uid_, std::string("[FATAL] ") + std::string(msg)); break;
  }
}

std::once_flag GLogger::flag_;

GLogger::GLogger(const char* const arg)
{
  std::call_once(flag_, [arg] { google::InitGoogleLogging(arg); });
};

void GLogger::Log(const LogLevel level, std::string_view msg) const
{
  switch (level)
  {
  case LogLevel::D: LOG(INFO) << msg; break;
  case LogLevel::I: LOG(INFO) << msg; break;
  case LogLevel::W: LOG(WARNING) << msg; break;
  case LogLevel::E: LOG(ERROR) << msg; break;
  case LogLevel::F: LOG(FATAL) << msg; break;
  }
}
