// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <string>
#include <filesystem>

#include <sys/stat.h>
#include <dirent.h>

#include "utility/log.h"

#ifdef TEST_BOT
static bool enable_markdown_to_image = false;
#else
static bool enable_markdown_to_image = true;
#endif

inline const std::filesystem::path k_markdown2image_path = std::filesystem::current_path() / "markdown2image"; // TODO: config

inline std::filesystem::path ImageAbsPath(const std::filesystem::path& rel_path)
{
    return (std::filesystem::current_path() / ".image" / "gen" / rel_path) += ".png";
}

inline int MarkdownToImage(const std::string& markdown, const std::filesystem::path& rel_path, const uint32_t width)
{
    if (!enable_markdown_to_image) {
        return false;
    }
    const auto abs_path = ImageAbsPath(rel_path);
    if (std::filesystem::exists(abs_path)) {
        // TODO: print debug msg
        // return 0;
    }
    std::filesystem::create_directories(abs_path.parent_path());
    const std::string cmd = k_markdown2image_path.string() + " --output " + abs_path.string() + " --width " + std::to_string(width) + " --nowith_css --noprint_info";
    FILE* fp = popen(cmd.c_str(), "w");
    if (fp == nullptr) {
        ErrorLog() << "Draw image failed cmd=\'" << cmd;
        return -1;
    }
    DebugLog() << "Draw image succeed cmd=\'" << cmd;
    fputs(markdown.c_str(), fp);
    pclose(fp);
    return 0;
}

inline int CharToImage(const char ch, const std::filesystem::path& rel_path)
{
    return MarkdownToImage(std::string("<style>html,body{color:#fdf3dd; background:#783623;}</style> <p align=\"middle\"><font size=\"6\"><b>") + ch + "</b></font></p>", rel_path, 85);
}
