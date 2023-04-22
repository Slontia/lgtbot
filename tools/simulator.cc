// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <gflags/gflags.h>

#include <cstring>
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
DEFINE_string(conf_path, "", "The path of the configuration file");

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

// ============================
//      Messager Callbacks
// ============================

void HandleMessages(void* handler, const char* const id, const int is_uid, const LGTBot_Message* messages, const size_t size)
{
    std::string s;
    if (is_uid) {
        s.append(Blue());
        s.append("[BOT -> USER_");
    } else {
        s.append(Purple());
        s.append("[BOT -> GROUP_");
    }
    s.append(id);
    s.append("]\n");
    s.append(Default());
    for (size_t i = 0; i < size; ++i) {
        const auto& msg = messages[i];
        switch (msg.type_) {
        case LGTBOT_MSG_TEXT:
            s.append(msg.str_);
            break;
        case LGTBOT_MSG_USER_MENTION:
            s.append(LightPink());
            s.append("@");
            s.append(msg.str_);
            s.append(Default());
            break;
        case LGTBOT_MSG_IMAGE:
            s.append("[image=");
            s.append(msg.str_);
            s.append("]");
            break;
        default:
            assert(false);
        }
    }
    std::cout << s << std::endl;
}

void GetUserName(void* handler, char* const buffer, const size_t size, const char* const user_id)
{
    strncpy(buffer, user_id, size);
}

void GetUserNameInGroup(void* handler, char* const buffer, const size_t size, const char* group_id, const char* const user_id)
{
    snprintf(buffer, size, "%s(gid=%s)", user_id, group_id);
}

std::string ImageAbsPath(const std::string_view rel_path);

int DownloadUserAvatar(void* handler, const char* const uid_str, const char* const dest_filename)
{
    const std::string avatar_filename = std::string("avatar_") + uid_str;
    if (CharToImage(uid_str[0], avatar_filename) != 0) {
        std::cerr << "Generate avatar failed for user: " << uid_str << std::endl;
        return false;
    }
    std::filesystem::copy(ImageAbsPath(avatar_filename), dest_filename, std::filesystem::copy_options::update_existing);
    return true;
}

// ============================
//       Handle Requests
// ============================

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

    ErrCode rc = gid != "-" ? LGTBot_HandlePublicRequest(bot, gid.data(), uid.data(), request_s.data())
                            : LGTBot_HandlePrivateRequest(bot, uid.data(), request_s.data());
    std::cout << ErrCodeColor(rc) << "Error Code: " << errcode2str(rc) << Default() << std::endl;

    return true;
}

int main(int argc, char** argv)
{
    //std::locale::global(std::locale("")); // this line can make number with comma
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    const LGTBot_Option option{
        .game_path_ = FLAGS_game_path.c_str(),
        .admins_ = FLAGS_admin_uid.c_str(),
#ifdef WITH_SQLITE
        .db_path_ = FLAGS_db_path.empty() ? nullptr : FLAGS_db_path.c_str(),
#endif
        .conf_path_ = FLAGS_conf_path.empty() ? nullptr : FLAGS_conf_path.c_str(),
        .callbacks_ = LGTBot_Callback{
            .get_user_name = GetUserName,
            .get_user_name_in_group = GetUserNameInGroup,
            .download_user_avatar = DownloadUserAvatar,
            .handle_messages = HandleMessages,
        },
    };
    const char* errmsg = nullptr;
    auto bot = LGTBot_Create(&option, &errmsg);
    if (!bot) {
        std::cerr << "[ERROR] Boot simulator failed: " << errmsg << std::endl;
        return 1;
    }

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
            if (LGTBot_ReleaseIfNoProcessingGames(bot)) {
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
