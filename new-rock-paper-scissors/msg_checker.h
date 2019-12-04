#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <iostream>
//#include <type_traits>

static const std::string k_empty_str_ = "";

class MsgReader final
{
public:
  MsgReader(const std::string& msg)
  {
    uint32_t pos = 0;
    uint32_t last_pos = 0;
    while (true)
    {
      /* Read next word. */
      pos = msg.find(" ", last_pos);
      if (last_pos != pos) { args_.push_back(msg.substr(last_pos, pos - last_pos)); }

      /* If finish reading, break. */
      if (pos == std::string::npos) { break; }
      last_pos = pos + 1;
    }
    iter_ = args_.begin();
  }

  ~MsgReader() {}

  bool HasNext() const { return iter_ != args_.end(); }

  const std::string& NextArg()
  {
    return iter_ == args_.end() ? k_empty_str_ : *(iter_++);
  }

  void Reset() { iter_ = args_.begin(); }

private:
  std::vector<std::string> args_;
  std::vector<std::string>::iterator iter_;
};

template <typename T>
class MsgArgChecker
{
public:
  typedef T arg_type;
  MsgArgChecker() {}
  virtual ~MsgArgChecker() {}
  virtual std::string FormatInfo() const = 0;
  virtual std::string ExampleInfo() const = 0;
  virtual std::pair<bool, T> Check(MsgReader& reader) const = 0;
};

template <>
class MsgArgChecker<void> final
{
public:
  typedef void arg_type;
  /* const_arg cannot contain spaces or be empty */
  MsgArgChecker(std::string&& const_arg) : const_arg_(const_arg) {}
  ~MsgArgChecker() {}
  bool Check(MsgReader& reader) const { return reader.HasNext() && reader.NextArg() == const_arg_; }
  std::string FormatInfo() const { return const_arg_; };
  std::string ExampleInfo() const { return const_arg_; };
private:
  const std::string const_arg_;
};

class MsgCommand
{
public:
  MsgCommand() {}
  virtual ~MsgCommand() {}
  virtual bool CallIfValid(MsgReader& msg_reader) const = 0;
  virtual std::string Info() const = 0;
};

template <typename Callback, typename ...CheckTypes>
class MsgCommandImpl final : public MsgCommand
{
private:
  using CheckerTuple = std::tuple<std::unique_ptr<MsgArgChecker<CheckTypes>>...>;

  template <unsigned N, typename ...Args>
  bool CallIfValid(MsgReader& msg_reader, const Args& ...args) const
  {
    if constexpr (N == std::tuple_size<CheckerTuple>::value)
    {
      if (msg_reader.HasNext()) { return false; }
      callback_(args...);
      return true;
    }
    else
    {
      auto& checker = *std::get<N>(checkers_);
      typedef typename std::decay<decltype(checker)>::type::arg_type checker_type;
      if constexpr (std::is_void<checker_type>::value)
      {
        return checker.Check(msg_reader) ? CallIfValid<N + 1, Args...>(msg_reader, args...)
          : false;
      }
      else
      {
        auto[is_valid, value] = checker.Check(msg_reader);
        return is_valid ? CallIfValid<N + 1, Args..., checker_type>(msg_reader, args..., value)
          : false;
      }
    }
  }

  template <typename Checker>
  std::string GetFormatInfo(const Checker& checker) const { return checker.FormatInfo(); }

public:
  MsgCommandImpl(Callback&& callback, std::unique_ptr<MsgArgChecker<CheckTypes>>&&... checkers) : callback_(callback), checkers_ (std::forward<std::unique_ptr<MsgArgChecker<CheckTypes>>&&>(checkers)...) {}

  virtual bool CallIfValid(MsgReader& msg_reader) const override
  {
    msg_reader.Reset();
    return CallIfValid<0>(msg_reader);
  }

  virtual std::string Info() const override
  {
    //const auto cat_str = [](std::string&& strs, ...) -> std::string { return strs + ..; };
    //const auto f = [this](const auto&... checkers) -> std::string { (GetFormatInfo(*checkers), ...); return ""; };
    //return "格式提示：" + std::apply(f, checkers_) + "\n举例：";
    return "";
  }

private:
  const Callback callback_;
  const CheckerTuple checkers_;
};
