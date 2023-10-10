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
#define GAME_OPTION_FILENAME "options.h"
#endif

// Include the 'options.h' in global namespace. We can include some headers in 'options.h' with 'INIT_OPTION_DEPEND` macro.
#define INIT_OPTION_DEPEND
#define EXTEND_OPTION(_1, _2, _3, _4)
#include GAME_OPTION_FILENAME
#undef EXTEND_OPTION
#undef INIT_OPTION_DEPEND

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#define OPTION_CLASSNAME MyGameOption
#define OPTION_FILENAME GAME_OPTION_FILENAME
#include "utility/extend_option.h"
#undef OPTION_CLASSNAME
#undef OPTION_FILENAME

class GameOption : public GameOptionBase, public MyGameOption
{
  public:
    virtual bool SetOption(const char* const msg) override
    {
        MsgReader msg_reader(msg);
        return MyGameOption::SetOption(msg_reader);
    }

    virtual void SetResourceDir(const char* const resource_dir)
    {
        resource_dir_ = resource_dir;
    }

    virtual void SetPlayerNum(const uint64_t player_num)
    {
        player_num_ = player_num;
    }

    virtual void SetSavedImageDir(const char* const saved_image_dir)
    {
        saved_image_dir_ = saved_image_dir;
    }

    virtual uint64_t PlayerNum() const { return player_num_; }

    virtual const char* ResourceDir() const { return resource_dir_.c_str(); }

    virtual const char* SavedImageDir() const { return saved_image_dir_.c_str(); }

    virtual const char* Info(const bool with_example, const bool with_html_syntax, const char* const prefix) const override
    {
        thread_local static std::string info;
        return (info = MyGameOption::Info(with_example, with_html_syntax, prefix)).c_str();
    }

    virtual const char* Status() const override
    {
        thread_local static std::string info;
        return (info = StatusInfo()).c_str();
    }

    virtual GameOptionBase* Copy() const override
    {
        return new GameOption(*this);
    }

    virtual const char* const* Content() const override
    {
        thread_local static std::array<std::string, Count()> str_content;
        thread_local static std::array<const char*, Count() + 1> c_str_content{nullptr};
        uint32_t i = 0;
        Foreach([&](const char* const /*description*/, const char* const name, const auto& checker, const auto& value)
                {
                    str_content[i] = std::string(name) + " " + checker.ArgString(value);
                    c_str_content[i] = str_content[i].c_str();
                    return false;
                });
        return c_str_content.data();
    }

    // The member functions to be realized by developers in mygame.cc

    std::string StatusInfo() const;

    virtual bool ToValid(MsgSenderBase& reply);

    virtual uint64_t BestPlayerNum() const;

  private:
    uint64_t player_num_ = 0;
    std::string resource_dir_;
    std::string saved_image_dir_;
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
