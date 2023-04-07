// Copyright (c) 2023-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <fstream>
#include <cstring>

#include "bot_core/bot_ctx.h"
#include "bot_core/game_handle.h"
#include "utility/log.h"
#include "nlohmann/json.hpp"
#include "game_framework/game_main.h"

bool BotCtx::LoadConfig_()
{
    if (!conf_path_) {
        return true;
    }
    std::ifstream f(conf_path_);
    if (!f) {
        ErrorLog() << "LoadConfig open config file failed, reason: '" << std::strerror(errno) << "', conf_path_: '"
                   << conf_path_ << "'";
        return false;
    }
    InfoLog() << "LoadConfig handle config file: " << conf_path_;
    nlohmann::json j;
    try {
        f >> j;
    } catch (...) {
        ErrorLog() << "LoadConfig parse config file failed conf_path: '" << conf_path_ << "'";
        return false;
    }
    {
        auto locked_option = option().Lock();
        for (const auto& [option_name, value] : j["bot"]["options"].items()) {
            MsgReader msg_reader(value.get<std::string>());
            if (locked_option.Get().SetOption(option_name, msg_reader)) {
                InfoLog() << "LoadConfig set bot option successfully: " << option_name << " " << value;
            } else {
                ErrorLog() << "LoadConfig set bot option failed: " << option_name << " " << value;
            }
        }
    }
    for (const auto& [game_name, game_json] : j["games"].items()) {
        const auto it = game_handles_.find(game_name);
        if (it == game_handles_.end()) {
            ErrorLog() << "LoadConfig game '" << game_name << "' not found";
            continue;
        }
        if (const auto json_it = game_json.find("multiple"); json_it != game_json.end()) {
            it->second->multiple_ = json_it->get<uint32_t>();
            InfoLog() << "LoadConfig set game '" << game_name << "' multiple successfully: " << it->second->multiple_;
        }
        auto locked_option = it->second->game_options_.Lock();
        for (const auto& [option_name, value] : game_json["options"].items()) {
            std::string option_str = option_name + " " + value.get<std::string>();
            if (locked_option.Get()->SetOption(option_str.c_str())) {
                InfoLog() << "LoadConfig set game '" << game_name << "' option successfully: " << option_str;
            } else {
                ErrorLog() << "LoadConfig set game '" << game_name << "' option failed: " << option_str;
            }
        }
    }
    return true;
}

static bool SaveConfig_(nlohmann::json& json, const std::filesystem::path::value_type* conf_path)
{
    if (!conf_path) {
        return true;
    }
    std::ofstream f(conf_path);
    if (!f) {
        ErrorLog() << "SaveConfig open config file failed, reason: '" << std::strerror(errno) << "', conf_path: '"
                   << conf_path << "'";
        return false;
    }
    InfoLog() << "SaveConfig handle config file: " << conf_path;
    f << json.dump(4);
    return true;
}

static void UpdateConfig_(nlohmann::json& json, const std::filesystem::path::value_type* conf_path,
        const std::string& option_name, const std::vector<std::string>& option_args)
{
    if (option_args.empty()) {
        json[option_name] = "";
        return;
    }
    std::string option_value = option_args[0];
    for (size_t i = 1; i < option_args.size(); ++i) {
        option_value += " " + option_args[i];
    }
    json[option_name] = std::move(option_value);
}

bool BotCtx::UpdateBotConfig(const std::string& option_name, const std::vector<std::string>& option_args)
{
    auto locked_config_json = config_json_.Lock();
    UpdateConfig_(locked_config_json.Get()["bot"]["options"], conf_path_, option_name, option_args);
    return SaveConfig_(locked_config_json.Get(), conf_path_);
}

bool BotCtx::UpdateGameConfig(const std::string& game_name, const std::string& option_name,
        const std::vector<std::string>& option_args)
{
    auto locked_config_json = config_json_.Lock();
    UpdateConfig_(locked_config_json.Get()["games"][game_name]["options"], conf_path_, option_name, option_args);
    return SaveConfig_(locked_config_json.Get(), conf_path_);
}

bool BotCtx::UpdateGameMultiple(const std::string& game_name, const uint32_t multiple)
{
    auto locked_config_json = config_json_.Lock();
    locked_config_json.Get()["games"][game_name]["multiple"] = multiple;
    return SaveConfig_(locked_config_json.Get(), conf_path_);
}
