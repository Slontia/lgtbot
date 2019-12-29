#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <iostream>
#include <functional>
#include <optional>
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
  virtual std::optional<T> Check(MsgReader& reader) const = 0;
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

template <typename Func> class FuncTypeHelper;
template <typename R, typename ...Args>
class FuncTypeHelper<R(Args...)>
{
public:
  typedef R ResultType;
  typedef std::tuple<Args...> ArgsTuple;
};

template <typename UserFuncType>
class MsgCommand
{
protected:
  typedef typename FuncTypeHelper<UserFuncType>::ResultType ResultType;
  typedef typename FuncTypeHelper<UserFuncType>::ArgsTuple UserArgsTuple;
  typedef typename std::conditional<std::is_void<ResultType>::value, bool, std::optional<ResultType>>::type CommandResultType;
public:
  MsgCommand() {}
  virtual ~MsgCommand() {}
  virtual CommandResultType CallIfValid(MsgReader& msg_reader, UserArgsTuple& user_args_tuple) const = 0;
  virtual std::string Info() const = 0;
};

template <typename Callback, typename UserFuncType, typename ...CheckTypes> //TODO: infer result_type from callback
class MsgCommandImpl final : public MsgCommand<UserFuncType>
{
private:
  using CheckerTuple = std::tuple<std::unique_ptr<MsgArgChecker<CheckTypes>>...>;

  static typename MsgCommand<UserFuncType>::CommandResultType NotMatch()
  {
    if constexpr (std::is_void_v<MsgCommand<UserFuncType>::ResultType>) { return false; }
    else { return {}; }
  }

  template <unsigned N, typename ...Args>
  typename MsgCommand<UserFuncType>::CommandResultType CallIfValidParseArgs(MsgReader& msg_reader, typename MsgCommand::UserArgsTuple& user_args_tuple, const Args&... args) const
  {
    if constexpr (N == std::tuple_size<CheckerTuple>::value)
    {
      if (msg_reader.HasNext()) { return NotMatch(); }
      const auto unpack_callback = [&callback = callback_, &args...](auto&... user_args) { return callback(user_args..., args...); };
      if constexpr (std::is_void_v<MsgCommand<UserFuncType>::ResultType>)
      {
        std::apply(unpack_callback, user_args_tuple);
        return true;
      }
      else
      {
        return std::apply(unpack_callback, user_args_tuple);
      }
    }
    else
    {
      auto& checker = *std::get<N>(checkers_);
      typedef typename std::decay<decltype(checker)>::type::arg_type checker_type;
      if constexpr (std::is_void<checker_type>::value)
      {
        return checker.Check(msg_reader) ? CallIfValidParseArgs<N + 1, Args...>(msg_reader, user_args_tuple, args...)
          : NotMatch();
      }
      else
      {
        std::optional value = checker.Check(msg_reader);
        return value ? CallIfValidParseArgs<N + 1, Args..., checker_type>(msg_reader, user_args_tuple, args..., *value)
          : NotMatch();
      }
    }
  }

  template <typename Checker>
  std::string GetFormatInfo(const Checker& checker) const { return checker.FormatInfo(); }

public:
  MsgCommandImpl(Callback&& callback, std::unique_ptr<MsgArgChecker<CheckTypes>>&&... checkers) : callback_(callback), checkers_ (std::forward<std::unique_ptr<MsgArgChecker<CheckTypes>>&&>(checkers)...) {}

  virtual typename MsgCommand::CommandResultType CallIfValid(MsgReader& msg_reader, typename MsgCommand::UserArgsTuple& user_args_tuple) const override
  {
    msg_reader.Reset();
    return CallIfValidParseArgs<0>(msg_reader, user_args_tuple);
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

template <typename UserFuncType, typename Callback, typename ...Checkers>
static auto MakeCommand(const Callback& f, std::unique_ptr<Checkers>&&... checkers)
{
  
  static_assert(std::is_same_v<typename FuncTypeHelper<UserFuncType>::ResultType, typename decltype(std::function(f))::result_type>);
  return std::make_shared<MsgCommandImpl<decltype(std::function(f)), UserFuncType, typename Checkers::arg_type...>>(std::function(f), std::move(checkers)...);
};