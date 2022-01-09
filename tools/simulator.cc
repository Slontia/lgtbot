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

#include "bot_core/bot_core.h"
#include "bot_core/msg_sender.h"

#if __linux__
#include "linenoise/linenoise.h"
#endif

DEFINE_string(game_path, "plugins", "The path of game modules");
DEFINE_string(history_filename, ".simulator_history.txt", "The file saving history commands");
DEFINE_bool(color, true, "Enable color");
DEFINE_uint64(bot_uid, 114514, "The UserID of bot");
DEFINE_uint64(admin_uid, 1, "The UserID of administor");

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
    Messager(const uint64_t id, const bool is_uid) : id_(id), is_uid_(is_uid) {}
    const uint64_t id_;
    const bool is_uid_;
    std::stringstream ss_;
};

void* OpenMessager(const uint64_t id, const bool is_uid)
{
    return new Messager(id, is_uid);
}

void MessagerPostText(void* p, const char* data, uint64_t len)
{
    Messager* const messager = static_cast<Messager*>(p);
    messager->ss_ << std::string_view(data, len);
}

void MessagerPostUser(void* p, uint64_t uid, bool is_at)
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

const char* GetUserName(const uint64_t uid, const uint64_t* const group_id)
{
    thread_local static std::string str;
    if (group_id == nullptr) {
        str = std::to_string(uid);
    } else {
        str = std::to_string(uid) + "(gid=" + std::to_string(*group_id) + ")";
    }
    return str.c_str();
}

auto init_bot(int argc, char** argv) { const char* errmsg = nullptr; }

std::pair<std::string_view, std::string_view> cut(const std::string_view line)
{
    if (const auto start_pos = line.find_first_not_of(' '); start_pos == std::string_view::npos) {
        return {std::string_view(), std::string_view()};
    } else if (const auto end_pos = line.find_first_of(' ', start_pos); end_pos == std::string_view::npos) {
        return {line.substr(start_pos), std::string_view()};
    } else {
        return {line.substr(start_pos, end_pos - start_pos), line.substr(end_pos)};
    }
};

bool handle_request(void* bot, const std::string_view line)
{
    const auto [gid_s, gid_remain_s] = cut(line);
    const auto [uid_s, request_s] = cut(gid_remain_s);

    if (gid_s.empty() || uid_s.empty() || request_s.empty()) {
        Error() << "Invalid request format" << std::endl;
        return false;
    }

    std::optional<uint64_t> gid;
    uint64_t uid = 0;

    try {
        if (gid_s != "-") {
            gid = std::stoull(gid_s.data());
        }
    } catch (const std::invalid_argument& e) {
        Error() << "Invalid GroupID \'" << gid_s << "\', can only be integer or \'-\' (which means send private message)" << std::endl;
        ;
        return false;
    }

    try {
        uid = std::stoull(uid_s.data());
    } catch (const std::invalid_argument& e) {
        Error() << "Invalid UserID \'" << uid_s << "\', can only be integer" << std::endl;
        ;
        return false;
    }

    ErrCode rc = gid.has_value() ? BOT_API::HandlePublicRequest(bot, gid.value(), uid, request_s.data())
                                 : BOT_API::HandlePrivateRequest(bot, uid, request_s.data());
    std::cout << ErrCodeColor(rc) << "Error Code: " << errcode2str(rc) << Default() << std::endl;

    return true;
}

int main(int argc, char** argv)
{
    //std::locale::global(std::locale("")); // this line can make number with comma
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    const uint64_t admins[2] = { FLAGS_admin_uid, 0 };
    const BotOption option {
        .this_uid_ = FLAGS_bot_uid,
        .game_path_ = FLAGS_game_path.c_str(),
        .image_path_ = "/image_path/",
        .admins_ = admins,
#ifdef WITH_SQLITE
        .db_path_ = std::filesystem::path(FLAGS_db_path).c_str(),
#endif
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
        } else if (line == "quit") {
#if __linux__
            linenoiseFree(line_cstr);
#endif
            std::cout << "Bye." << std::endl;
            break;
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

    BOT_API::Release(bot);

    return 0;
}
