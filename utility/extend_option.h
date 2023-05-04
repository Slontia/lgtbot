// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#if !defined(OPTION_FILENAME) || !defined(OPTION_CLASSNAME)
#error OPTION_FILENAME and OPTION_CLASSNAME is not defined
#endif

#ifndef GET_OPTION_VALUE
#define GET_OPTION_VALUE(option, name) (option).template Option2Value<std::decay_t<decltype(option)>::OPTION_##name>()
#endif

#ifndef GET_VALUE
#define GET_VALUE(name) GET_OPTION_VALUE(*this, name)
#endif

#define OPTION_(name) OPTION_##name
#define CHECKER_(name) (std::get<OPTION_(name) * 2 + 1>(options_))
#define VALUE_(name) (std::get<OPTION_(name) * 2>(options_))

class OPTION_CLASSNAME
{
  public:
    enum Option : int {
        INVALID_OPTION = -1,
#define EXTEND_OPTION(_0, name, _1, _2) OPTION_(name),
#include OPTION_FILENAME
#undef EXTEND_OPTION
        MAX_OPTION
    };

    OPTION_CLASSNAME() : options_{
#define EXTEND_OPTION(_0, _1, checker, default_value) \
        default_value, std::move(checker),
#include OPTION_FILENAME
#undef EXTEND_OPTION
        }
    {
#define EXTEND_OPTION(_0, name, _1, _2) \
        static_assert(!std::is_void_v<std::decay_t<decltype(CHECKER_(name))>::arg_type>, \
                "Checker cannot be void checker");
#include OPTION_FILENAME
#undef EXTEND_OPTION
    }

    OPTION_CLASSNAME(const OPTION_CLASSNAME&) = default;

    OPTION_CLASSNAME(OPTION_CLASSNAME&&) = default;

    template <Option op>
    auto& Option2Value()
    {
        static_assert(op != Option::INVALID_OPTION, "Unexpected option");
        return std::get<op * 2>(options_);
    }

    template <Option op>
    const auto& Option2Value() const
    {
        static_assert(op != Option::INVALID_OPTION, "Unexpected option");
        return std::get<op * 2>(options_);
    }

    bool SetOption(MsgReader& msg_reader)
    {
        if (!msg_reader.HasNext()) {
            return false;
        }
        const auto& find_name = msg_reader.NextArg();
        return SetOption(find_name, msg_reader);
    }

    bool SetOption(const std::string_view find_name, MsgReader& msg_reader)
    {
        const auto reader_it = msg_reader.Iterator();
        bool find = false;
        Find(find_name, [&](const char* const description, const auto& checker, auto& value)
                {
                    msg_reader.Reset(reader_it);
                    auto new_value = checker.Check(msg_reader);
                    if (new_value.has_value() && !msg_reader.HasNext()) {
                        value = *new_value;
                        find = true;
                    }
                });
        return find;
    }

    template <typename Fn>
    bool Find(const std::string_view find_name, Fn&& fn)
    {
        // TODO: use std::map to optimize
        return Foreach([&](const char* const description, const char* const name, const auto& checker, auto& value)
                {
                    if (name == find_name) {
                        fn(description, checker, value);
                        return true; // break;
                    }
                    return false; // continue;
                });
    }

    template <typename Fn>
    bool Foreach(Fn&& fn)
    {
#define EXTEND_OPTION(description, name, _1, _2) \
        if (fn(description, #name, CHECKER_(name), VALUE_(name))) { \
            return true; \
        }
#include OPTION_FILENAME
#undef EXTEND_OPTION
        return false;
    }

    // TODO: use deducing this to support const version
    template <typename Fn>
    bool Foreach(Fn&& fn) const
    {
#define EXTEND_OPTION(description, name, _1, _2) \
        if (fn(description, #name, CHECKER_(name), VALUE_(name))) { \
            return true; \
        }
#include OPTION_FILENAME
#undef EXTEND_OPTION
        return false;
    }

    static constexpr uint32_t Count() { return Option::MAX_OPTION; }

    std::string Info() const { return Info_<false>(); }

    std::string ColoredInfo() const { return Info_<true>(); }

  private:
    template <bool WITH_COLOR>
    std::string Info_() const
    {
        std::string outstr;
        int i = 0;
        Foreach([&](const char* const description, const char* const name, const auto& checker, const auto& value)
                {
                    outstr += "\n" + std::to_string(i++) + ". ";
                    outstr += description;
                    // format
                    outstr += "\n    - 格式：";
                    outstr += name;
                    outstr += " ";
                    outstr += WITH_COLOR ? checker.ColoredFormatInfo() : checker.FormatInfo();
                    // example
                    outstr += "\n    - 例如：";
                    outstr += name;
                    outstr += " ";
                    outstr += checker.ExampleInfo();
                    // current value
                    outstr += "\n    - 当前：";
                    if (WITH_COLOR) {
                        outstr += HTML_COLOR_FONT_HEADER(red);
                    }
                    outstr += checker.ArgString(value);
                    if (WITH_COLOR) {
                        outstr += HTML_FONT_TAIL;
                    }
                    return false;
                });
        return outstr;
    }

    decltype(std::tuple{
#define EXTEND_OPTION(_0, _1, checker, default_value) \
    static_cast<std::decay<decltype(checker)>::type::arg_type>(default_value), checker,
#include OPTION_FILENAME
#undef EXTEND_OPTION
    }) options_;
};

#undef OPTION_
#undef VALUE_
#undef CHECKER_
