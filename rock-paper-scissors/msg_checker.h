#pragma once
#include <string>
#include <vector>

class MsgReader
{
public:
  MsgReader(const std::string& msg);
  ~MsgReader();
  bool HasNext() const;
  const std::string& NextArg() const;
  void Reset();

private:
  std::vector<std::string> args_;
  std::vector<std::string>::iterator iter_;
};

template <typename T>
class MsgArgChecker
{
public:
  typedef T Type;
  virtual bool DoCheck(const MsgReader&, T& value) const = 0;
  static std::string Hint();
  static std::string Example();
  T Value() const { return value_; }
  bool IsValid() const { return is_valid_; }

private:
  T value_;
  bool is_valid_;
};

template <>
class MsgArgChecker<void>
{
public:
  typedef void Type;
  MsgArgChecker() {}
  virtual ~MsgArgChecker() {}
  virtual bool DoCheck(const MsgReader&) const = 0;
  static std::string Hint();
  static std::string Example();
  bool IsValid() const { return is_valid_; }

private:
  bool is_valid_;
};

class MsgCheckerInterface
{
 public:
  MsgCheckerInterface(std::string&& info) : info_(info) {}

  virtual bool CheckAndCall(MsgReader& msg_reader) = 0;
  const std::string& Info() { return info_; }

 private:
  const std::string info_;
};

template <typename Callback, typename Checker, typename ...Checkers>
class MsgChecker : MsgCheckerInterface
{
public:
  MsgChecker(const Callback& callback) : callback_(callback), 
    MsgCheckerInterface("Hint: " + Hint<Checker, Checkers...>() + "\n" + "Example: " + Example<Checker, Checkers...>()) {}

  bool CheckAndCall(MsgReader& msg_reader)
  {
    return CheckAndCall<Callback, std::tuple<>, Checker, Checkers...>(callback, msg_reader, std::tuple<>());
  }

private:
  Callback callback_;

  template <typename Callback, typename ArguTuple>
  static bool CheckAndCall(const Callback& callback, MsgReader& msg_reader, const ArguTuple& tuple)
  {
    Checker checker(msg_reader);
    if (!msg_reader.HasNext())
    {
      std::apply(callback, std::tuple_cat(tuple, std::make_tuple(checker.Value())));
      return true;
    }
    return false;
  }

  template <typename Callback, typename ArguTuple, typename Checker, typename ...Checkers>
  static bool CheckAndCall(const Callback& callback, MsgReader& msg_reader, const ArguTuple& tuple)
  {
    Checker checker(msg_reader);
    if (!checker.IsValid())
    {
      return false;
    }
    else if constexpr (std::is_void_v<Checker::type>)
    {
      return CheckAndCall(callback, msg_reader, tuple);
    }
    else
    {
      return CheckAndCall(callback, msg_reader, std::tuple_cat(tuple, std::make_tuple(checker.Value())));
    }
  }

  template <typename Checker, typename ...Checkers>
  static std::string Hint()
  {
    return "[" + Checker::Hint() + "] " + Hint<Checkers...>();
  }

  template <>
  static std::string Hint<>()
  {
    return "";
  }

  template <typename Checker, typename ...Checkers>
  static std::string Example()
  {
    return Checker::Example() + Example<Checkers...>();
  }

  template <>
  static std::string Example<>()
  {
    return ""
  }
};




