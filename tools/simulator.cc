// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <gflags/gflags.h>

#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>

#include "bot_core/bot_core.h"
#include "bot_core/msg_sender.h"

#if __linux__
#include "linenoise/linenoise.h"
#endif

DEFINE_string(game_path, "plugins", "The path of game modules");
DEFINE_string(history_filename, ".simulator_history.txt", "The file saving history commands");
DEFINE_bool(color, true, "Enable color");
DEFINE_string(bot_uid, "this_bot", "The UserID of bot");
DEFINE_string(admin_uid, "admin", "The UserID of administor");
DEFINE_string(conf_file, "", "The path of the configuration file");

#ifdef WITH_SQLITE
DEFINE_string(db_path, "simulator.db", "Name of database");
#endif

const char* Red() { return FLAGS_color ? "\033[31m" : ""; }
const char* Green() { return FLAGS_color ? "\033[32m" : ""; }
const char* Yellow() { return FLAGS_color ? "\033[33m" : ""; }
const char* Blue() { return FLAGS_color ? "\033[34m" : ""; }
const char* Purple() { return FLAGS_color ? "\033[35m" : ""; }
const char* LightPink() { return FLAGS_color ? "\033[1;95m" : ""; }
const char* LightCyan() { return FLAGS_color ? "\033[96m" : ""; }
const char* Default() { return FLAGS_color ? "\033[0m" : ""; }

static const char* ErrCodeColor(const ErrCode rc)
{
    switch (rc) {
    case EC_OK:
    case EC_GAME_REQUEST_OK:
        return Green();
    case EC_GAME_REQUEST_CHECKOUT:
        return Yellow();
    default:
        return Red();
    }
}

std::ostream& Error() { return std::cerr << Red() << "[ERROR] " << Default(); }
std::ostream& Log() { return std::clog << Blue() << "[LOG] " << Default(); }

struct Messager
{
    Messager(const char* const id, const bool is_uid) : id_(id), is_uid_(is_uid) {}
    const std::string id_;
    const bool is_uid_;
    std::stringstream ss_;
};

void* OpenMessager(const char* const id, const bool is_uid)
{
    return new Messager(id, is_uid);
}

void MessagerPostText(void* const p, const char* const data, const uint64_t len)
{
    Messager* const messager = static_cast<Messager*>(p);
    messager->ss_ << std::string_view(data, len);
}

void MessagerPostUser(void* const p, const char* const uid, const bool is_at)
{
    Messager* const messager = static_cast<Messager*>(p);
    messager->ss_ << LightPink();
    if (is_at) {
        messager->ss_ << "@" << uid;
    } else if (messager->is_uid_) {
        messager->ss_ << uid;
    } else {
        messager->ss_ << uid << "(gid=" << messager->id_ << ")";
    }
    messager->ss_ << Default();
}

void MessagerPostImage(void* p, const std::filesystem::path::value_type* path)
{
    Messager* const messager = static_cast<Messager*>(p);
    std::basic_string<std::filesystem::path::value_type> path_str(path);
    messager->ss_ << "[image=" << std::string(path_str.begin(), path_str.end()) << "]";
}

void MessagerFlush(void* p)
{
    Messager* const messager = static_cast<Messager*>(p);
    if (messager->is_uid_) {
        std::cout << Blue() << "[BOT -> USER_" << messager->id_ << "]" << Default() << std::endl << messager->ss_.str() << std::endl;
    } else {
        std::cout << Purple() << "[BOT -> GROUP_" << messager->id_ << "]" << Default() << std::endl << messager->ss_.str() << std::endl;
    }
    messager->ss_.str("");
}

void CloseMessager(void* p)
{
    Messager* const messager = static_cast<Messager*>(p);
    delete messager;
}

const char* GetUserName(const char* uid, const char* const group_id)
{
    thread_local static std::string str;
    if (group_id == nullptr) {
        str = uid;
    } else {
        str = std::string(uid) + "(gid=" + std::string(group_id) + ")";
    }
    return str.c_str();
}

inline std::filesystem::path ImageAbsPath(const std::filesystem::path& rel_path);

bool DownloadUserAvatar(const char* const uid_str, const std::filesystem::path::value_type* const dest_filename)
{
    const std::string avatar_filename = std::string("avatar_") + uid_str;
    if (CharToImage(uid_str[0], avatar_filename) != 0) {
        std::cerr << "Generate avatar failed for user: " << uid_str << std::endl;
        return false;
    }
    std::filesystem::copy(ImageAbsPath(avatar_filename), dest_filename, std::filesystem::copy_options::update_existing);
    return true;
}

auto init_bot(int argc, char** argv) { const char* errmsg = nullptr; }

std::pair<std::string, std::string> cut(const std::string_view line)
{
    if (const auto start_pos = line.find_first_not_of(' '); start_pos == std::string_view::npos) {
        return {std::string(), std::string()};
    } else if (const auto end_pos = line.find_first_of(' ', start_pos); end_pos == std::string_view::npos) {
        return {std::string(line.substr(start_pos)), std::string()};
    } else {
        return {std::string(line.substr(start_pos, end_pos - start_pos)), std::string(line.substr(end_pos))};
    }
};

bool handle_request(void* bot, const std::string_view line)
{
    const auto [gid, gid_remain_s] = cut(line);
    const auto [uid, request_s] = cut(gid_remain_s);

    if (gid.empty() || uid.empty() || request_s.empty()) {
        Error() << "Invalid request format" << std::endl;
        return false;
    }

    if (uid == "-") {
        Error() << "Invalid UserID, should not be \"-\"" << std::endl;
        return false;
    }

    ErrCode rc = gid != "-" ? BOT_API::HandlePublicRequest(bot, gid.data(), uid.data(), request_s.data())
                            : BOT_API::HandlePrivateRequest(bot, uid.data(), request_s.data());
    std::cout << ErrCodeColor(rc) << "Error Code: " << errcode2str(rc) << Default() << std::endl;

    return true;
}

int main(int argc, char** argv)
{
    //std::locale::global(std::locale("")); // this line can make number with comma
    gflags::ParseCommandLineFlags(&argc, &argv, true);
#ifdef WITH_SQLITE
    const auto db_path = std::filesystem::path(FLAGS_db_path);
#endif
    const BotOption option {
        .this_uid_ = FLAGS_bot_uid.c_str(),
        .game_path_ = FLAGS_game_path.c_str(),
        .image_path_ = "/image_path/",
        .admins_ = FLAGS_admin_uid.c_str(),
#ifdef WITH_SQLITE
        .db_path_ = db_path.c_str(),
#endif
        .conf_path_ = FLAGS_conf_file.empty() ? nullptr : FLAGS_conf_file.c_str(),
    };
    auto bot = BOT_API::Init(&option);

#if __linux__
    linenoiseHistoryLoad(FLAGS_history_filename.c_str());
#endif

#if __linux__
    for (char* line_cstr = nullptr; (line_cstr = linenoise("Simulator>>> ")) != nullptr;) {
#else
    for (char line_cstr[1024] = {0}; std::cout << "Simulator>>> ", std::cin.getline(&line_cstr[0], 1024); ) {
#endif
        const std::string_view line(line_cstr);
        if (line.find_first_not_of(' ') == std::string_view::npos) {
            // do nothing
        } else if (line == "quit" || line == "exit") {
            if (BOT_API::ReleaseIfNoProcessingGames(bot)) {
                std::cout << "Bye." << std::endl;
#if __linux__
                linenoiseFree(line_cstr);
#endif
                break;
            } else {
                std::cout << "There are processing games, please try again later or quit by Ctrl-C." << std::endl;
            }
        } else if (handle_request(bot, line)) {
#if __linux__
            linenoiseHistoryAdd(line_cstr);
            linenoiseHistorySave(FLAGS_history_filename.c_str());
#endif
        } else {
            Error() << "Usage: <GroupID | \'-\'> <UserID> <arg1> <arg2> ..." << std::endl;
        }
#if __linux__
        linenoiseFree(line_cstr);
#endif
    }


    return 0;
}
