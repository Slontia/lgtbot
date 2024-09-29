// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <string>
#include <filesystem>
#include <cassert>

#include <sys/stat.h>
#include <dirent.h>

#include "utility/log.h"

#ifdef TEST_BOT
inline bool enable_markdown_to_image = false;
#else
inline bool enable_markdown_to_image = true;
#endif

inline const std::string k_markdown2image_path = (std::filesystem::current_path() / "markdown2image").string(); // TODO: config

inline int MarkdownToImage(const std::string& markdown, const std::string& path, const uint32_t width)
{
    assert(!path.empty());
    if (!enable_markdown_to_image) {
        return false;
    }
    if (std::filesystem::exists(path)) {
        // TODO: print debug msg
        // return 0;
    }
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    const std::string cmd = k_markdown2image_path + " --output " + path + " --width " + std::to_string(width) + " --nowith_css --noprint_info";
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

inline int CharToImage(const char ch, const std::string& path)
{
    return MarkdownToImage(std::string("<style>html,body{color:#fdf3dd; background:#783623;}</style> <p align=\"middle\"><font size=\"6\"><b>") + ch + "</b></font></p>", path, 85);
}
