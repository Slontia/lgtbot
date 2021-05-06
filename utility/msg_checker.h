#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
static const std::string k_empty_str_ = "";

// TODO: check callback parameters

class MsgReader final
{
   public:
    MsgReader(const std::string& msg)
    {
        std::istringstream ss(msg);
        for (std::string arg; ss >> arg;) {
            args_.push_back(arg);
        }
        iter_ = args_.begin();
    }

    ~MsgReader() {}

    bool HasNext() const { return iter_ != args_.end(); }

    const std::string& NextArg() { return iter_ == args_.end() ? k_empty_str_ : *(iter_++); }

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

// Require: has next argument
// Return: the next argument
class AnyArg : public MsgArgChecker<std::string>
{
   public:
    AnyArg(const std::string& meaning = "字符串", const std::string& example = "字符串")
            : meaning_(meaning), example_(example)
    {
    }
    virtual ~AnyArg() {}
    virtual std::string FormatInfo() const override { return "<" + meaning_ + ">"; }
    virtual std::string ExampleInfo() const override { return example_; }
    virtual std::optional<std::string> Check(MsgReader& reader) const override
    {
        if (!reader.HasNext()) {
            return {};
        }
        return reader.NextArg();
    }

   private:
    const std::string meaning_;
    const std::string example_;
};

// Request: the argument should be <true_str> or <false_str>
// Return: if argument is <true_str>, return true; if argument is <false_str>, return false
class BoolChecker : public MsgArgChecker<bool>
{
   public:
    BoolChecker(const std::string& true_str, const std::string& false_str) : true_str_(true_str), false_str_(false_str)
    {
    }
    virtual ~BoolChecker() {}
    virtual std::string FormatInfo() const override { return "<" + true_str_ + "|" + false_str_ + ">"; }
    virtual std::string ExampleInfo() const override { return true_str_; }
    virtual std::optional<bool> Check(MsgReader& reader) const override
    {
        if (!reader.HasNext()) {
            return {};
        }
        const std::string str = reader.NextArg();
        if (str == true_str_) {
            return true;
        } else if (str == false_str_) {
            return false;
        } else {
            return {};
        }
    }

   private:
    const std::string true_str_;
    const std::string false_str_;
};

// Requires: the argument is one of the key in <arg_map>
// Return: the value to the key
template <typename T>
class AlterChecker : public MsgArgChecker<T>
{
   public:
    AlterChecker(std::map<std::string, T>&& arg_map, const std::string& meaning = "选择")
            : arg_map_(arg_map), meaning_(meaning)
    {
    }
    virtual ~AlterChecker() {}
    virtual std::string FormatInfo() const override
    {
        std::stringstream ss;
        ss << "<" << meaning_ << "：";
        if (arg_map_.empty()) {
            ss << "(错误，可选项为空)";
        } else {
            bool is_first = true;
            for (const auto& [arg_str, _] : arg_map_) {
                if (is_first) {
                    is_first = !is_first;
                } else {
                    ss << "|";
                }
                ss << arg_str;
            }
        }
        ss << ">";
        return ss.str();
    }
    virtual std::string ExampleInfo() const override
    {
        return arg_map_.empty() ? "（错误，可选项为空）" : arg_map_.begin()->first;
    }
    virtual std::optional<T> Check(MsgReader& reader) const
    {
        if (!reader.HasNext()) {
            return std::optional<T>();
        }
        std::string s = reader.NextArg();
        const auto it = arg_map_.find(s);
        return it == arg_map_.end() ? std::optional<T>() : it->second;
    }

   private:
    const std::map<std::string, T> arg_map_;
    const std::string meaning_;
};

// Require: the argument can be convert to a number in [<Min>, <Max>], or no more arguments and <Optional> is true
// Return: the converted number wrapped in std::optional, or an empty std::optional if no more arguments
template <typename T, T Min = std::numeric_limits<T>::min(), T Max = std::numeric_limits<T>::max(),
          bool Optional = false>
requires std::is_arithmetic_v<T> class ArithChecker
        : public MsgArgChecker<std::conditional_t<Optional, std::optional<T>, T>>
{
   public:
    ArithChecker(const std::string meaning = "数字") : meaning_(meaning) { static_assert(Max >= Min, "Invalid Range"); }
    virtual ~ArithChecker() {}
    virtual std::string FormatInfo() const override
    {
        return "<" + meaning_ + "：" + std::to_string(Min) + "~" + std::to_string(Max) + ">";
    }
    virtual std::string ExampleInfo() const override { return std::to_string((Min + Max) / 2); }
    virtual std::optional<std::conditional_t<Optional, std::optional<T>, T>> Check(MsgReader& reader) const
    {
        if (!reader.HasNext()) {
            return std::optional<T>();
        }
        std::stringstream ss;
        ss << reader.NextArg();
        if (T value; ss >> value && Min <= value && value <= Max) {
            return value;
        } else {
            return {};
        };
    }

   private:
    const std::string meaning_;
};

// Require: the argument can be convert to a number in [<Min>, <Max>], or no more arguments and <Optional> is true
// Return: the converted number wrapped in std::optional, or an empty std::optional if no more arguments
template <typename T, typename Min, typename Max, bool Optional = false>
        requires(std::is_same_v<T, std::decay_t<Min>> || std::is_invocable_r_v<T, Min>) &&
        (std::is_same_v<T, std::decay_t<Max>> || std::is_invocable_r_v<T, Max>)class DynamicArithChecker
        : public MsgArgChecker<std::conditional_t<Optional, std::optional<T>, T>>
{
   public:
    DynamicArithChecker(Min&& min, Max&& max, const std::string meaning = "数字")
            : min_(std::forward<Min>(min)), max_(std::forward<Max>(max)), meaning_(meaning)
    {
    }
    virtual ~DynamicArithChecker() {}
    virtual std::string FormatInfo() const override
    {
        return "<" + meaning_ + "：" + std::to_string(Bound_(min_)) + "~" + std::to_string(Bound_(max_)) + ">";
    }
    virtual std::string ExampleInfo() const override { return std::to_string((Bound_(min_) + Bound_(max_)) / 2); }
    virtual std::optional<std::conditional_t<Optional, std::optional<T>, T>> Check(MsgReader& reader) const
    {
        if (!reader.HasNext()) {
            return std::optional<T>();
        }
        std::stringstream ss;
        ss << reader.NextArg();
        if (T value; ss >> value && Bound_(min_) <= value && value <= Bound_(max_)) {
            return value;
        } else {
            return {};
        };
    }

   private:
    template <typename B>
    requires std::is_same_v<T, std::decay_t<B>> static T Bound_(B&& bound)
    {
        return bound;
    }

    template <typename B>
    requires std::is_invocable_r_v<T, B> static T Bound_(B&& bound)
    {
        return bound();
    }

    const std::string meaning_;
    const Min min_;
    const Max max_;
};

// Require: type argument can be convert to type <T>, or no more arguments and <Optional> is true
// Return: the converted object <T> wrapped in std::optional, or an empty std::optional if no more arguments
template <typename T, bool Optional = false>
class BasicChecker : public MsgArgChecker<std::conditional_t<Optional, std::optional<T>, T>>
{
   public:
    BasicChecker(const std::string meaning = "对象") : meaning_(meaning) {}
    virtual ~BasicChecker() {}
    virtual std::string FormatInfo() const override { return "<" + meaning_ + ">"; }
    virtual std::string ExampleInfo() const override { return std::to_string(T()); }
    virtual std::optional<std::conditional_t<Optional, std::optional<T>, T>> Check(MsgReader& reader) const
    {
        if (!reader.HasNext()) {
            return std::optional<T>();
        }
        std::stringstream ss;
        ss << reader.NextArg();
        if (T value; ss >> value) {
            return value;
        } else {
            return {};
        };
    }

   private:
    const std::string meaning_;
};

using VoidChecker = MsgArgChecker<void>;

// Require: the argument is equal to <const_arg>
// Return: nothing
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

template <typename> class Command;

template <typename UserResult, typename ...UserArgs>
class Command<UserResult(UserArgs...)>
{
  private:
    class Base_
    {
    protected:
        using CommandResult = typename std::conditional_t<std::is_void_v<UserResult>, bool, std::optional<UserResult>>;

    public:
        Base_() {}
        virtual ~Base_() {}
        virtual CommandResult CallIfValid(MsgReader& msg_reader, UserArgs... user_args) const = 0;
        virtual std::string Info(const bool with_example = false) const = 0;
    };

    template <typename Callback, typename... Checkers>
    class Impl_ : public Base_
    {
        using CommandResult = typename std::conditional_t<std::is_void_v<UserResult>, bool, std::optional<UserResult>>;
        using UserFunc = UserResult(UserArgs...);

      private:
        static CommandResult NotMatch()
        {
            if constexpr (std::is_void_v<UserResult>) {
                return false;
            } else {
                return std::nullopt;
            }
        }

        template <unsigned N, typename ...Args>
        CommandResult CallIfValidParseArgs(MsgReader& msg_reader, Args&&... args) const
        {
            if constexpr (N == std::tuple_size<decltype(checkers_)>::value) {
                if (msg_reader.HasNext()) {
                    return NotMatch();
                }
                if constexpr (std::is_void_v<UserResult>) {
                    callback_(std::forward<Args>(args)...);
                    return true;
                } else {
                    return callback_(std::forward<Args>(args)...);
                }
            } else {
                auto& checker = std::get<N>(checkers_);
                typedef typename std::decay<decltype(checker)>::type::arg_type checker_type;
                if constexpr (std::is_void<checker_type>::value) {
                    return checker.Check(msg_reader)
                                ? CallIfValidParseArgs<N + 1, Args...>(msg_reader, std::forward<Args>(args)...)
                                : NotMatch();
                } else {
                    std::optional value = checker.Check(msg_reader);
                    return value ? CallIfValidParseArgs<N + 1, Args..., const checker_type&>(msg_reader, std::forward<Args>(args)...,
                                                                                    std::move(*value))
                                : NotMatch();
                }
            }
        }

      public:
        Impl_(const char* const description, Callback&& callback, Checkers&&... checkers)
                : description_(description),
                callback_(std::forward<Callback>(callback)),
                checkers_(std::forward<Checkers>(checkers)...)
        {
        }

        // pass by value here if UserArgs is not reference
        virtual CommandResult CallIfValid(MsgReader& msg_reader, UserArgs... user_args) const override
        {
            msg_reader.Reset();
            return CallIfValidParseArgs<0>(msg_reader, std::forward<UserArgs&&>(user_args)...);
        }

        virtual std::string Info(const bool with_example) const override
        {
            std::stringstream ss;
            ss << description_ << std::endl;
            ss << "格式：";
            std::apply([this, &ss](const auto&... checkers) { (ss << ... << (checkers.FormatInfo() + " ")); }, checkers_);
            if (with_example) {
                ss << std::endl;
                ss << "例如：";
                std::apply([this, &ss](const auto&... checkers) { (ss << ... << (checkers.ExampleInfo() + " ")); },
                        checkers_);
            }
            return ss.str();
        }

      private:
        const char* const description_;
        const Callback callback_;
        const std::tuple<Checkers...> checkers_;
    };

  public:
    template <typename Callback, typename... Checkers>
    Command(const char* const description, Callback&& callback, Checkers&&... checkers)
    : cmd_(std::make_shared<Impl_<Callback, Checkers...>>(description, std::forward<Callback>(callback), std::forward<Checkers>(checkers)...))
    {
    }

    Command(const Command&) = default;
    Command(Command&&) = default;

    template <typename ...Args>
    auto CallIfValid(Args&&... args) const { return cmd_->CallIfValid(std::forward<Args>(args)...); }

    auto Info() const { return cmd_->Info(); }

  private:
    std::shared_ptr<Base_> cmd_;
};

