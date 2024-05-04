// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <regex>
#include <fstream>
#include <cstring>
#include <ranges>
#include <span>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#define HINSTANCE void*
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#else
static_assert(false, "Not support OS");
#endif

#include "utility/log.h"
#include "bot_core/db_manager.h"
#include "bot_core/match.h"
#include "bot_core/msg_sender.h"
#include "game_framework/game_main.h"
#include "nlohmann/json.hpp"

// TODO: use std::ranges::views::split
static std::set<UserID> SplitIdsByComma(const std::string_view& str)
{
    if (str.empty()) {
        return {};
    }
    std::set<UserID> ids;
    std::string::size_type begin = 0;
    while (true) {
        if (begin == str.size()) {
            break;
        }
        const auto end = str.find_first_of(',', begin);
        if (end == std::string::npos) {
            ids.emplace(str.substr(begin));
            break;
        }
        ids.emplace(str.substr(begin, end - begin));
        begin = end + 1;
    }
    for (const auto& id : ids) {
        InfoLog() << "Load idr: " << id;
    }
    return ids;
}

static auto FillAchievements(const std::span<const lgtbot::game::GameAchievement> achievements)
{
    std::vector<GameHandle::Achievement> result;
    std::ranges::transform(achievements, std::back_inserter(result), [](const lgtbot::game::GameAchievement& achievement)
            {
                return GameHandle::Achievement{.name_ = achievement.name_, .description_ = achievement.description_};
            });
    return result;
}

static void LoadGame(HINSTANCE mod, GameHandleMap& game_handles)
{
    if (!mod) {
#ifdef __linux__
        ErrorLog() << "Load mod failed: " << dlerror();
#else
        ErrorLog() << "Load mod failed";
#endif
        return;
    }
    const auto load_proc = [&mod](const char* const name)
    {
        const auto proc = GetProcAddress(mod, name);
        if (!proc) {
#ifdef __linux__
            std::cerr << dlerror() << std::endl;
#endif
            throw std::runtime_error(std::string("load proc ") + name + " from module failed");
        }
        return proc;
    };
    try {
        const lgtbot::game::GameInfo game_info = reinterpret_cast<lgtbot::game::GameInfo(*)()>(load_proc("GetGameInfo"))();
        game_handles.emplace(std::piecewise_construct, std::forward_as_tuple(game_info.game_name_), std::forward_as_tuple(
                    GameHandle::BasicInfo{
                        .name_ = game_info.game_name_,
                        .module_name_ = game_info.module_name_,
                        .developer_ = game_info.developer_,
                        .description_ = game_info.description_,
                        .rule_ = game_info.rule_,
                        .achievements_ = FillAchievements(std::span(game_info.achievements_.data_, game_info.achievements_.size_)),
                        .max_player_num_fn_ = reinterpret_cast<GameHandle::max_player_num_handler>(load_proc("MaxPlayerNum")),
                        .multiple_fn_ = reinterpret_cast<GameHandle::multiple_handler>(load_proc("Multiple")),
                        .handle_rule_command_fn_ = reinterpret_cast<GameHandle::rule_command_handler>(load_proc("HandleRuleCommand")),
                        .handle_init_options_command_fn_ = reinterpret_cast<GameHandle::init_options_command_handler>(load_proc("HandleInitOptionsCommand")),
                    },
                    GameHandle::InternalHandler{
                        .game_options_allocator_ = reinterpret_cast<GameHandle::game_options_allocator>(load_proc("NewGameOptions")),
                        .game_options_deleter_ = reinterpret_cast<GameHandle::game_options_deleter>(load_proc("DeleteGameOptions")),
                        .main_stage_allocator_ = reinterpret_cast<GameHandle::main_stage_allocator>(load_proc("NewMainStage")),
                        .main_stage_deleter_ = reinterpret_cast<GameHandle::main_stage_deleter>(load_proc("DeleteMainStage")),
                        .mod_guard_ = [mod] { FreeLibrary(mod); },
                    },
                    game_info.default_generic_options_
                    ));
    } catch (const std::exception& e) {
        ErrorLog() << "Load mod failed: " << e.what();
        return;
    }
    InfoLog() << "Loaded successfully!";
}

// TODO: use std::expect
static std::variant<GameHandleMap, const char*> LoadGameModules(const char* const games_path)
{
    GameHandleMap game_handles;
    if (games_path == nullptr) {
        return game_handles;
    }
#ifdef _WIN32
    WIN32_FIND_DATA file_data;
    HANDLE file_handle = FindFirstFile((std::string(games_path) + "\\*.dll").c_str(), &file_data);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return "LoadGameModules: find first file failed";
    }
    do {
        const auto dll_path = std::string(games_path) + "\\" + file_data.cFileName;
        LoadGame(LoadLibrary(dll_path.c_str()), game_handles);
    } while (FindNextFile(file_handle, &file_data));
    FindClose(file_handle);
    InfoLog() << "Load module count: " << game_handles.size();
#elif __linux__
    DIR* d = opendir(games_path);
    if (!d) {
        return "LoadGameModules: open directory failed";
    }
    const std::regex base_regex(".*so");
    std::smatch base_match;
    struct stat st;
    for (dirent* dp = nullptr; (dp = readdir(d)) != NULL;) {
        std::string name(dp->d_name);
        if (std::regex_match(name, base_match, base_regex); base_match.empty()) {
            DebugLog() << "Find irrelevant file " << name << ", skip";
            continue;
        }
        InfoLog() << "Loading library " << name;
        LoadGame(dlopen((std::string(games_path) + "/" + name).c_str(), RTLD_LAZY), game_handles);
    }
    InfoLog() << "Loading finished.";
    closedir(d);
#endif
    if (game_handles.empty()) {
        return "LoadGameModules: find no games";
    }
    return game_handles;
}

// TODO: use std::expect
static std::variant<nlohmann::json, const char*> LoadConfig(const char* const conf_path, GameHandleMap& game_handles,
        MutableBotOption& bot_options)
{
    if (!conf_path) {
        return nlohmann::json{};
    }
    if (!std::filesystem::exists(conf_path)) {
        std::ofstream file(conf_path); // create file if not exists
        if (!file) {
            ErrorLog() << "LoadConfig create config file failed, reason: '" << std::strerror(errno) << "', conf_path: '"
                    << conf_path << "'";
            return "LoadConfig: create configuration file failed";
        }
        file << "{}";
    }
    std::ifstream f(conf_path);
    if (!f) {
        ErrorLog() << "LoadConfig open config file failed, reason: '" << std::strerror(errno) << "', conf_path: '"
                   << conf_path << "'";
        return "LoadConfig: open configuration file failed";
    }
    InfoLog() << "LoadConfig handle config file: " << conf_path;
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        ErrorLog() << "LoadConfig parse config file failed, conf_path: \"" << conf_path << "\", errmsg: " << "\"" << e.what() << "\"";
        return "LoadConfig: parse configuration file failed";
    }
    for (const auto& [option_name, value] : j["bot"]["options"].items()) {
        MsgReader msg_reader(value.get<std::string>());
        if (bot_options.SetOption(option_name, msg_reader)) {
            InfoLog() << "LoadConfig set bot option successfully: " << option_name << " " << value;
        } else {
            ErrorLog() << "LoadConfig set bot option failed: " << option_name << " " << value;
        }
    }
    for (const auto& [game_name, game_json] : j["games"].items()) {
        const auto it = game_handles.find(game_name);
        if (it == game_handles.end()) {
            ErrorLog() << "LoadConfig game '" << game_name << "' not found";
            continue;
        }
        auto locked_option = it->second.DefaultGameOptions().Lock();
        for (const auto& [option_name, value] : game_json["options"].items()) {
            std::string option_str = option_name + " " + value.get<std::string>();
            if (locked_option->game_options_->SetOption(option_str.c_str())) {
                InfoLog() << "LoadConfig set game '" << game_name << "' option successfully: " << option_str;
            } else {
                ErrorLog() << "LoadConfig set game '" << game_name << "' option failed: " << option_str;
            }
        }
    }
    return j;
}

BotCtx::BotCtx(std::string game_path,
               std::string conf_path,
               std::string image_path,
               LGTBot_Callback callbacks,
               GameHandleMap game_handles,
               std::set<UserID> admins,
#ifdef WITH_SQLITE
               std::unique_ptr<DBManagerBase> db_manager,
#endif
               MutableBotOption mutable_bot_options,
               nlohmann::json config_json,
               void* const handler)
    : game_path_(std::move(game_path))
    , conf_path_(std::move(conf_path))
    , image_path_(std::move(image_path))
    , callbacks_(std::move(callbacks))
    , game_handles_(std::move(game_handles))
    , admins_(std::move(admins))
#ifdef WITH_SQLITE
    , db_manager_(std::move(db_manager))
#endif
    , mutable_bot_options_(std::move(mutable_bot_options))
    , config_json_(std::move(config_json))
    , match_manager_(*this)
    , handler_(handler)
{
}

std::variant<BotCtx*, const char*> BotCtx::Create(const LGTBot_Option& options)
{
    auto game_handles = LoadGameModules(options.game_path_);
    if (const char* const* const errmsg = std::get_if<const char*>(&game_handles)) {
        return *errmsg;
    }
#ifdef WITH_SQLITE
    std::unique_ptr<DBManagerBase> db_manager;
    if (options.db_path_ && !(db_manager = SQLiteDBManager::UseDB(options.db_path_))) {
        return "use database failed";
    }
#endif
    MutableBotOption bot_options;
    auto config_json = LoadConfig(options.conf_path_, std::get<GameHandleMap>(game_handles),
            bot_options);
    if (const char* const* const errmsg = std::get_if<const char*>(&config_json)) {
        return *errmsg;
    }
    for (const void* const* p = reinterpret_cast<const void* const*>(&options.callbacks_);
            p < reinterpret_cast<const void* const*>(&options.callbacks_ + 1);
            ++p) {
        if (!*p) {
            return "some of the callback is NULL";
        }
    }
    return new BotCtx(
            options.game_path_ ? options.game_path_ : "",
            options.conf_path_ ? options.conf_path_ : "",
            options.image_path_ ? options.image_path_ : (std::filesystem::current_path() / ".lgtbot_image").string(),
            options.callbacks_,
            std::move(std::get<GameHandleMap>(game_handles)),
            options.admins_ ? SplitIdsByComma(options.admins_) : std::set<UserID>{},
#ifdef WITH_SQLITE
            std::move(db_manager),
#endif
            std::move(bot_options),
            std::move(std::get<nlohmann::json>(config_json)),
            options.handler_
            );
}

static bool SaveConfig_(nlohmann::json& json, std::string_view conf_path)
{
    if (conf_path.empty()) {
        return true;
    }
    std::ofstream f(conf_path.data());
    if (!f) {
        ErrorLog() << "SaveConfig open config file failed, reason: '" << std::strerror(errno) << "', conf_path: '"
                   << conf_path << "'";
        return false;
    }
    InfoLog() << "SaveConfig handle config file: " << conf_path;
    f << json.dump(4);
    return true;
}

static void UpdateConfig_(nlohmann::json& json, const std::string& option_name, const std::vector<std::string>& option_args)
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
    try {
        auto locked_config_json = config_json_.Lock();
        UpdateConfig_((*locked_config_json)["bot"]["options"], option_name, option_args);
        return SaveConfig_((*locked_config_json), conf_path_);
    } catch (const std::exception& e) {
        ErrorLog() << "UpdateBotConfig failed: " << e.what();
        return false;
    }
}

bool BotCtx::UpdateGameConfig(const std::string& game_name, const std::string& option_name,
        const std::vector<std::string>& option_args)
{
    try {
        auto locked_config_json = config_json_.Lock();
        UpdateConfig_((*locked_config_json)["games"][game_name]["options"], option_name, option_args);
        return SaveConfig_((*locked_config_json), conf_path_);
    } catch (const std::exception& e) {
        ErrorLog() << "UpdateGameConfig failed: " << e.what();
        return false;
    }
}

bool BotCtx::UpdateGameDefaultFormal(const std::string& game_name, const bool is_formal)
{
    try {
        auto locked_config_json = config_json_.Lock();
        (*locked_config_json)["games"][game_name]["is_formal"] = is_formal;
        return SaveConfig_((*locked_config_json), conf_path_);
    } catch (const std::exception& e) {
        ErrorLog() << "UpdateGameFormal failed: " << e.what();
        return false;
    }
}

std::string BotCtx::GetUserAvatar(const char* const user_id, const int32_t size) const
{
    const auto path = (std::filesystem::absolute(image_path_) / "avatar" / user_id) += ".png";
    std::filesystem::create_directories(path.parent_path());
    const std::string path_str = path.string();
    if (!callbacks_.download_user_avatar(handler_, user_id, path_str.c_str())) {
        CharToImage(user_id[0], path_str);
    }
    return "<img src=\"file:///" + path_str + "\" style=\"width:" + std::to_string(size) + "px; height:" +
        std::to_string(size) + "px; border-radius:50%; vertical-align: middle;\"/>";
}

MsgSender BotCtx::MakeMsgSender(const UserID& user_id, Match* const match) const
{
    return MsgSender(handler_, image_path_, callbacks_, user_id, match);
}

MsgSender BotCtx::MakeMsgSender(const GroupID& group_id, Match* const match) const
{
    return MsgSender(handler_, image_path_, callbacks_, group_id, match);
}
