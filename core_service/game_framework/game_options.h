// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once
#include <array>
#include <string_view>

#include "game_framework/game_main.h"
#include "utility/msg_checker.h"

#ifndef GAME_MODULE_NAME
#error GAME_MODULE_NAME is not defined
#endif

#ifndef GAME_OPTION_FILENAME
#error GAME_OPTION_FILENAME is not defined
#endif

// Include the 'options.h' in global namespace. We can include some headers in 'options.h' with 'INIT_OPTION_DEPEND` macro.
#define INIT_OPTION_DEPEND
#define EXTEND_OPTION(_1, _2, _3, _4)
#define EXTEND_MODE(_1)
#include GAME_OPTION_FILENAME
#undef EXTEND_MODE
#undef EXTEND_OPTION
#undef INIT_OPTION_DEPEND

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#define OPTION_CLASSNAME CustomOptions
#define OPTION_FILENAME GAME_OPTION_FILENAME
#include "utility/extend_option.h"
#undef OPTION_CLASSNAME
#undef OPTION_FILENAME

class GameOptions : public GameOptionsBase, public CustomOptions
{
  public:
    GameOptions() = default;
    GameOptions(const GameOptions&) = default;
    GameOptions(GameOptions&&) = default;

    virtual bool SetOption(const char* const msg) override
    {
        MsgReader msg_reader(msg);
        return CustomOptions::SetOption(msg_reader);
    }

    virtual const char* Info(const bool with_example, const bool with_html_syntax, const char* const prefix) const override
    {
        thread_local static std::string info;
        return (info = CustomOptions::Info(with_example, with_html_syntax, prefix)).c_str();
    }

    virtual const char* const* ShortInfo() const override
    {
        thread_local static std::array<std::string, Count()> str_content;
        thread_local static std::array<const char*, Count() + 1> c_str_content{nullptr};
        uint32_t i = 0;
        Foreach([&](const char* const /*description*/, const char* const name, const auto& checker, const auto& value)
                {
                    str_content[i] = std::string(name) + " " + checker.ArgString(value);
                    c_str_content[i] = str_content[i].c_str();
                    ++i;
                    return false;
                });
        return c_str_content.data();
    }

    virtual GameOptionsBase* Copy() const override { return new GameOptions(*this); }
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
