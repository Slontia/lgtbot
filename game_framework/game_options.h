#pragma once
#include <array>
#include <string_view>

#include "utility/msg_checker.h"

#define OPTION_(name) OPTION_##name
#define CHECKER_(name) (*std::get<OPTION_(name) * 2 + 1>(options_))
#define VALUE_(name) (std::get<OPTION_(name) * 2>(options_))
#define GET_VALUE(name) Option2Value<GameOption::OPTION_##name>()

class GameOption
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
    GameOption() : options_
	 {
#define GAME_OPTION(_0, _1, checker, default_value) default_value, std::move(checker),
#include "options.h"
#undef GAME_OPTION
	 }, infos_
	 {
#define GAME_OPTION(description, name, _0, _1)                                      \
    [this] {                                                                        \
        std::stringstream ss;                                                       \
        ss << description << std::endl;                                             \
        ss << "格式：" << #name << " " << CHECKER_(name).FormatInfo() << std::endl; \
        ss << "例如：" << #name << " " << CHECKER_(name).ExampleInfo();             \
        return ss.str();                                                            \
    }(),
#include "options.h"
#undef GAME_OPTION
	 } {};

    void SetPlayerNum(const uint64_t player_num) { player_num_ = player_num; }

    template <Option op>
    const auto& Option2Value() const
    {
        static_assert(op != Option::INVALID_OPTION, "Unexpected option");
        return std::get<op * 2>(options_);
    }

    bool SetOption(MsgReader& msg_reader)
    {
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

    const auto Infos() const { return infos_; }

    const std::string StatusInfo() const;

    const uint64_t PlayerNum() const { return player_num_; }

   private:
    decltype(std::tuple{
#define GAME_OPTION(_0, _1, checker, default_value)                             \
    static_cast<std::decay<decltype(*checker)>::type::arg_type>(default_value), \
            static_cast<std::unique_ptr<MsgArgChecker<std::decay<decltype(*checker)>::type::arg_type>>>(checker),
#include "options.h"
#undef GAME_OPTION
    }) options_;
    std::array<std::string, Option::MAX_OPTION> infos_;
    uint64_t player_num_;
};

#undef OPTION_
#undef VALUE_
#undef CHECKER_
