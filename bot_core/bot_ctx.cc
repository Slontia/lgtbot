// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <regex>
#include <fstream>
#include <cstring>

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

    typedef bool(*GetGameInfo)(lgtbot::game::GameInfo* game_info);

    const auto load_proc = [&mod](const char* const name)
    {
        const auto proc = GetProcAddress(mod, name);
        if (!proc) {
            ErrorLog() << "Load proc " << name << " from module failed";
#ifdef __linux__
            std::cerr << dlerror() << std::endl;
#endif
        }
        return proc;
    };

    const auto game_info_fn = (GetGameInfo)load_proc("GetGameInfo");
    const auto game_options_allocator_fn = (GameHandle::game_options_allocator)load_proc("NewGameOptions");
    const auto game_options_deleter_fn = (GameHandle::game_options_deleter)load_proc("DeleteGameOptions");
    const auto main_stage_allocator_fn = (GameHandle::main_stage_allocator)load_proc("NewMainStage");
    const auto main_stage_deleter_fn = (GameHandle::main_stage_deleter)load_proc("DeleteMainStage");

    if (!game_info_fn || !game_options_allocator_fn || !game_options_deleter_fn || !main_stage_allocator_fn || !main_stage_deleter_fn) {
        return;
    }

    const char* rule = nullptr;
    const char* module_name = nullptr;
    uint64_t max_player = 0;
    lgtbot::game::GameInfo game_info;
    if (!game_info_fn(&game_info)) {
        ErrorLog() << "Load failed: Cannot get game game";
        return;
    }
    std::vector<GameHandle::Achievement> achievements;
    for (const lgtbot::game::GameAchievement* achievement = game_info.achievements_; achievement->name_; ++achievement) {
        achievements.emplace_back(achievement->name_, achievement->description_);
    }
    game_handles.emplace(
            game_info.game_name_,
            std::make_unique<GameHandle>(game_info.game_name_, game_info.module_name_, game_info.max_player_,
                game_info.rule_, std::move(achievements), game_info.multiple_, game_info.developer_, game_info.description_,
                game_options_allocator_fn, game_options_deleter_fn, main_stage_allocator_fn, main_stage_deleter_fn,
                [mod] { FreeLibrary(mod); }));
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
    } catch (...) {
        ErrorLog() << "LoadConfig parse config file failed conf_path: '" << conf_path << "'";
        return false;
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
    return j;
}

BotCtx::BotCtx(std::string game_path,
               std::string conf_path,
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
    std::unique_ptr<DBManagerBase> db_manager;
    if (options.db_path_ && !(db_manager = SQLiteDBManager::UseDB(options.db_path_))) {
        return "use database failed";
    }
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
    auto locked_config_json = config_json_.Lock();
    UpdateConfig_(locked_config_json.Get()["bot"]["options"], option_name, option_args);
    return SaveConfig_(locked_config_json.Get(), conf_path_);
}

bool BotCtx::UpdateGameConfig(const std::string& game_name, const std::string& option_name,
        const std::vector<std::string>& option_args)
{
    auto locked_config_json = config_json_.Lock();
    UpdateConfig_(locked_config_json.Get()["games"][game_name]["options"], option_name, option_args);
    return SaveConfig_(locked_config_json.Get(), conf_path_);
}

bool BotCtx::UpdateGameMultiple(const std::string& game_name, const uint32_t multiple)
{
    auto locked_config_json = config_json_.Lock();
    locked_config_json.Get()["games"][game_name]["multiple"] = multiple;
    return SaveConfig_(locked_config_json.Get(), conf_path_);
}

std::string BotCtx::GetUserAvatar(const char* const user_id, const int32_t size) const
{
    const auto path = (std::filesystem::current_path() / ".image" / "avatar" / user_id) += ".png";
    std::filesystem::create_directories(path.parent_path());
    const std::string path_str = path.string();
    if (!callbacks_.download_user_avatar(handler_, user_id, path_str.c_str())) {
        CharToImage(user_id[0], path_str);
    }
    return "<img src=\"file://" + path_str + "\" style=\"width:" + std::to_string(size) + "px; height:" +
        std::to_string(size) + "px; border-radius:50%; vertical-align: middle;\"/>";
}

MsgSender BotCtx::MakeMsgSender(const UserID& user_id, Match* const match) const
{
    return MsgSender(handler_, callbacks_, user_id, match);
}

MsgSender BotCtx::MakeMsgSender(const GroupID& group_id, Match* const match) const
{
    return MsgSender(handler_, callbacks_, group_id, match);
}
