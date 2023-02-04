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

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// MyGameOption is a developer-defined class, so it should be put into GAME_MODULE_NAME namespace.

#define OPTION_CLASSNAME MyGameOption
#define OPTION_FILENAME GAME_OPTION_FILENAME
#include "utility/extend_option.h"
#undef OPTION_CLASSNAME
#undef OPTION_FILENAME

class GameOption : public GameOptionBase, public MyGameOption
{
  public:
    GameOption() : GameOptionBase(Count()) {};

    virtual ~GameOption() {}

    virtual bool SetOption(const char* const msg) override
    {
        MsgReader msg_reader(msg);
        return MyGameOption::SetOption(msg_reader);
    }

    virtual void SetResourceDir(const std::filesystem::path::value_type* const resource_dir)
    {
        std::basic_string<std::filesystem::path::value_type> resource_dir_str(resource_dir);
        resource_dir_ = std::string(resource_dir_str.begin(), resource_dir_str.end());
    }

    virtual const char* ResourceDir() const { return resource_dir_.c_str(); }

    virtual const char* Info(const uint64_t index) const override { return MyGameOption::Info(index); }

    virtual const char* ColoredInfo(const uint64_t index) const override { return MyGameOption::ColoredInfo(index); }

    virtual const char* Status() const override
    {
        thread_local static std::string info;
        info = StatusInfo();
        return info.c_str();
    }

    // The member functions to be realized by developers in mygame.cc

    std::string StatusInfo() const;

    virtual bool ToValid(MsgSenderBase& reply);

    virtual uint64_t BestPlayerNum() const;

  private:
    std::string resource_dir_;
};

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
