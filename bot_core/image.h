#pragma once

#include <string>
#include <filesystem>

#include <sys/stat.h>
#include <dirent.h>

#include "log.h"

inline const std::filesystem::path k_markdown2image_path = std::filesystem::current_path() / "markdown2image"; // TODO: config

inline std::filesystem::path ImageAbsPath(const std::filesystem::path& rel_path)
{
    return (std::filesystem::current_path() / ".image"/ rel_path) += ".png";
}

inline int MarkdownToImage(const std::string& markdown, const std::filesystem::path& rel_path)
{
#ifndef TEST_BOT
    const auto abs_path = ImageAbsPath(rel_path);
    if (std::filesystem::exists(abs_path)) {
        // TODO: print debug msg
        // return 0;
    }
    std::filesystem::create_directories(abs_path.parent_path());
    const std::string cmd = k_markdown2image_path.string() + " --output " + abs_path.string() + " --width 700";
    FILE* fp = popen(cmd.c_str(), "w");
    if (fp == nullptr) {
        ErrorLog() << "popen markdown2image failed cmd=\'" << cmd;
        return -1;
    }
    DebugLog() << "popen markdown2image cmd=\'" << cmd;
    fputs(markdown.c_str(), fp);
    pclose(fp);
#endif
    return 0;
}

