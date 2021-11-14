#pragma once
#include <array>
#include <string_view>

#include "game_framework/game_main.h"
#include "utility/msg_checker.h"

#define OPTION_(name) OPTION_##name
#define CHECKER_(name) (std::get<OPTION_(name) * 2 + 1>(options_))
#define VALUE_(name) (std::get<OPTION_(name) * 2>(options_))
#define GET_VALUE(name) Option2Value<GameOption::OPTION_##name>()

class GameOption : public GameOptionBase
{
   public:
    enum Option : int {
        INVALID_OPTION = -1,
#define GAME_OPTION(_0, name, _1, _2) OPTION_(name),
#include "options.h"
#undef GAME_OPTION
        MAX_OPTION
    };

    template <typename T>
    class foo;
    GameOption() : GameOptionBase(Option::MAX_OPTION), options_
	 {
#define GAME_OPTION(_0, _1, checker, default_value) default_value, std::move(checker),
#include "options.h"
#undef GAME_OPTION
	 }, infos_
	 {
#define GAME_OPTION(description, name, _0, _1)                                                                \
    [this]() -> std::pair<std::string, std::string> {                                                         \
        const auto head = description "\n    - 格式：" #name " ";                                                   \
        const auto tail = "\n    - 例如：" #name " " + CHECKER_(name).ExampleInfo();                                \
        return {head + CHECKER_(name).FormatInfo() + tail, head + CHECKER_(name).ColoredFormatInfo() + tail}; \
    }(),
#include "options.h"
#undef GAME_OPTION
	 } {};

    template <Option op>
    const auto& Option2Value() const
    {
        static_assert(op != Option::INVALID_OPTION, "Unexpected option");
        return std::get<op * 2>(options_);
    }

    virtual bool SetOption(const char* const msg) override
    {
        MsgReader msg_reader(msg);
#define GAME_OPTION(_0, name, _1, _2)                                                    \
    static_assert(!std::is_void_v<std::decay<decltype(CHECKER_(name))>::type::arg_type>, \
                  "Checker cannot be void checker");                                     \
    if (msg_reader.Reset(); #name == msg_reader.NextArg()) {                             \
        auto value = CHECKER_(name).Check(msg_reader);                                   \
        if (value.has_value() && !msg_reader.HasNext()) {                                \
            VALUE_(name) = *value;                                                       \
            return true;                                                                 \
        }                                                                                \
    }
#include "options.h"
#undef GAME_OPTION
        return false;
    }

    virtual void SetResourceDir(const char* const resource_dir) { resource_dir_ = resource_dir; }
    virtual const char* ResourceDir() const { return resource_dir_.c_str(); }
    virtual const char* Info(const uint64_t index) const override { return infos_[index].first.c_str(); }
    virtual const char* ColoredInfo(const uint64_t index) const override { return infos_[index].second.c_str(); }

    virtual const char* Status() const override
    {
        thread_local static std::string info;
        info = StatusInfo();
        return info.c_str();
    }

    std::string StatusInfo() const;
    virtual bool IsValid(MsgSenderBase& reply) const;
    virtual uint64_t BestPlayerNum() const;

   private:
    decltype(std::tuple{
#define GAME_OPTION(_0, _1, checker, default_value)                             \
    static_cast<std::decay<decltype(checker)>::type::arg_type>(default_value), checker,
#include "options.h"
#undef GAME_OPTION
    }) options_;
    std::array<std::pair<std::string, std::string>, Option::MAX_OPTION> infos_;
    std::string resource_dir_;
};

#undef OPTION_
#undef VALUE_
#undef CHECKER_
