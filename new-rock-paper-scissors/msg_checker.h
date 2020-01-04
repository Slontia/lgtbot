#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <iostream>
#include <functional>
#include <optional>
#include <sstream>
static const std::string k_empty_str_ = "";

class MsgReader final
{
public:
  MsgReader(const std::string& msg)
  {
    std::istringstream ss(msg);
    for (std::string arg; ss >> arg;) { args_.push_back(arg); }
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

class AnyArg : public MsgArgChecker<std::string>
{
public:
  AnyArg(const std::string& meaning = "字符串", const std::string& example = "字符串") : meaning_(meaning), example_(example) {}
  virtual ~AnyArg() {}
  virtual std::string FormatInfo() const override { return "<" + meaning_ + ">"; }
  virtual std::string ExampleInfo() const override { return example_; }
  virtual std::optional<std::string> Check(MsgReader& reader) const override
  {
    if (!reader.HasNext()) { return {}; }
    return reader.NextArg();
  }

private:
  const std::string meaning_;
  const std::string example_;
};

class BoolChecker : public MsgArgChecker<bool>
{
public:
  BoolChecker(const std::string& true_str, const std::string& false_str) : true_str_(true_str), false_str_(false_str) {}
  virtual ~BoolChecker() {}
  virtual std::string FormatInfo() const override { return "<" + true_str_ + ":" + false_str_ + ">"; }
  virtual std::string ExampleInfo() const override { return true_str_; }
  virtual std::optional<bool> Check(MsgReader& reader) const override
  {
    if (!reader.HasNext()) { return {}; }
    const std::string str = reader.NextArg();
    if (str == true_str_) { return true; }
    else if (str == false_str_) { return false; }
    else { return {}; }
  }

private:
  const std::string true_str_;
  const std::string false_str_;
};

template <typename T, T Min, T Max, bool Optional = false, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
class ArithChecker : public MsgArgChecker<std::conditional_t<Optional, std::optional<T>, T>>
{
public:
  ArithChecker(const std::string meaning = "数字", const bool show_range = true) : meaning_(meaning) { static_assert(Max >= Min, "Invalid Range"); }
  virtual ~ArithChecker() {}
  virtual std::string FormatInfo() const override { return "<" + meaning_ + "：" + std::to_string(Min) + "~" + std::to_string(Max) + ">"; }
  virtual std::string ExampleInfo() const override { return std::to_string((Min + Max) / 2); }
  virtual std::optional<std::conditional_t<Optional, std::optional<T>, T>> Check(MsgReader& reader) const
  {
    if (!reader.HasNext()) { return std::optional<T>(); }
    std::stringstream ss;
    ss << reader.NextArg();
    if (T value; ss >> value && Min <= value && value <= Max) { return value; }
    else { return {}; };
  }

private:
  const std::string meaning_;
};

template <typename T, bool Optional = false, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
class BasicChecker : public MsgArgChecker<std::conditional_t<Optional, std::optional<T>, T>>
{
public:
  BasicChecker(const std::string meaning = "对象", const bool show_range = true) : meaning_(meaning) {}
  virtual ~BasicChecker() {}
  virtual std::string FormatInfo() const override
  {
    return "<" + meaning_ + ">";
  }
  virtual std::string ExampleInfo() const override { return std::to_string(T()); }
  virtual std::optional<std::conditional_t<Optional, std::optional<T>, T>> Check(MsgReader& reader) const
  {
    if (!reader.HasNext()) { return std::optional<T>(); }
    std::stringstream ss;
    ss << reader.NextArg();
    if (T value; ss >> value) { return value; }
    else { return {}; };
  }

private:
  const std::string meaning_;
};

using VoidChecker = MsgArgChecker<void>;

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
  virtual CommandResultType CallIfValid(MsgReader& msg_reader, UserArgsTuple&& user_args_tuple = {}) const = 0;
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

  virtual typename MsgCommand::CommandResultType CallIfValid(MsgReader& msg_reader, typename MsgCommand::UserArgsTuple&& user_args_tuple) const override
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