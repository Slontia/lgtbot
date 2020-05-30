#pragma once
#include <sstream>

template <typename MsgHandle>
class MsgGuard
{
public:
  MsgGuard(MsgHandle&& handle) : handle_(std::forward<MsgHandle>(handle)) {}
  ~MsgGuard() { if (ss_.tellp() > 0) { handle_(ss_.str()); } }
  MsgGuard(const MsgGuard&) = delete;
  MsgGuard(MsgGuard&&) = default;

  template <typename Str>
  MsgGuard& operator<<(Str&& str)
  {
    ss_ << str;
    return *this;
  }

  MsgGuard& operator<<(std::ostream& (*os)(std::ostream&))
  {
    ss_ << os;
    return *this;
  }

  std::stringstream& ss() { return ss_; }

private:
  MsgHandle handle_;
  std::stringstream ss_;
};
