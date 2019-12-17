#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <iostream>
#include <functional>

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

template <typename ResultType>
class MsgCommand
{
public:
  MsgCommand() {}
  virtual ~MsgCommand() {}
  virtual typename std::conditional<std::is_void<ResultType>::value, bool, std::pair<bool, ResultType>>::type CallIfValid(MsgReader& msg_reader) const = 0;
  virtual std::string Info() const = 0;
};

template <typename Callback, typename ...CheckTypes> //TODO: infer result_type from callback
class MsgCommandImpl final : public MsgCommand<typename Callback::result_type>
{
private:
  using CheckerTuple = std::tuple<std::unique_ptr<MsgArgChecker<CheckTypes>>...>;
  using ResultType = typename Callback::result_type;
  using CommandResultType = typename std::conditional<std::is_void<ResultType>::value, bool, std::pair<bool, ResultType>>::type;
  
  static CommandResultType NotMatch()
  {
    if constexpr (std::is_void_v<ResultType>) { return false; }
    else { return {false, {}}; }
  }

  template <unsigned N, typename ...Args>
  CommandResultType CallIfValid(MsgReader& msg_reader, const Args& ...args) const
  {
    if constexpr (N == std::tuple_size<CheckerTuple>::value)
    {
      if (msg_reader.HasNext()) { return NotMatch(); }
      if constexpr (std::is_void_v<ResultType>)
      {
        callback_(args...);
        return true;
      }
      else
      {
        return {true, callback_(args...)};
      }
    }
    else
    {
      auto& checker = *std::get<N>(checkers_);
      typedef typename std::decay<decltype(checker)>::type::arg_type checker_type;
      if constexpr (std::is_void<checker_type>::value)
      {
        return checker.Check(msg_reader) ? CallIfValid<N + 1, Args...>(msg_reader, args...)
          : NotMatch();
      }
      else
      {
        auto[is_valid, value] = checker.Check(msg_reader);
        return is_valid ? CallIfValid<N + 1, Args..., checker_type>(msg_reader, args..., value)
          : NotMatch();
      }
    }
  }

  template <typename Checker>
  std::string GetFormatInfo(const Checker& checker) const { return checker.FormatInfo(); }

public:
  MsgCommandImpl(Callback&& callback, std::unique_ptr<MsgArgChecker<CheckTypes>>&&... checkers) : callback_(callback), checkers_ (std::forward<std::unique_ptr<MsgArgChecker<CheckTypes>>&&>(checkers)...) {}

  virtual CommandResultType CallIfValid(MsgReader& msg_reader) const override
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

template <typename Callback>
using to_func = decltype(std::function{ Callback() });

template <typename Callback, typename ...Checkers>
static auto Make(const Callback& f, std::unique_ptr<Checkers>&&... checkers)
{
  return std::make_shared < MsgCommandImpl<decltype(std::function{ f }), typename Checkers::arg_type... >> (std::function{ f }, std::move(checkers)...);
};