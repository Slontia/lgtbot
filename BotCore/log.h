#pragma once
#include "../Utility/msg_guard.h"
#include <vector>
#include <mutex>
#include <iostream>

enum class LogLevel { D, I, W, E, F };

class Logger
{
 public:
  virtual ~Logger() {}
  virtual void Log(const LogLevel level, std::string_view msg) const = 0;
};

class PrivateMsgLogger : public Logger
{
public:
  PrivateMsgLogger(const uint64_t uid);
  virtual void Log(const LogLevel level, std::string_view msg) const override;

private:
  const uint64_t uid_;
};

class GLogger : public Logger
{
public:
  GLogger(const char* const arg);
  virtual void Log(const LogLevel level, std::string_view msg) const override;

private:
  static std::once_flag flag_;
};

extern std::vector<std::unique_ptr<Logger>> loggers_;

template <typename MyLogger, typename ...Args>
Logger& EmplaceLogger(Args&&... args) { return *loggers_.emplace_back(std::make_unique<MyLogger>(args...)); }

auto Log(const LogLevel level)
{
  return MsgGuard([level, &loggers = loggers_](const std::string_view sv)
    {
      for (auto& logger : loggers) { logger->Log(level, sv); }
    });
}

auto InfoLog() { return Log(LogLevel::I); }
auto WarnLog() { return Log(LogLevel::W); }
auto ErrorLog() { return Log(LogLevel::E); }
auto FatalLog() { return Log(LogLevel::F); }
