// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_main.h"

#ifdef _WIN32

#include <Windows.h>

#define IDR_TEXT1_RULE 101

#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE 103
#define _APS_NEXT_COMMAND_VALUE 40001
#define _APS_NEXT_CONTROL_VALUE 1001
#define _APS_NEXT_SYMED_VALUE 101
#endif
#endif

#elif __linux__

extern char _binary_rule_md_start[];

#endif

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#ifdef _WIN32

HMODULE hModule;

std::string LoadText(const int idr_rule, const char* type)
{
    if (hModule == NULL) return "hmod is NULL";

    HRSRC hRsrc = FindResource(hModule, MAKEINTRESOURCE(idr_rule), type);
    if (NULL == hRsrc) return "FindResource failed: " + std::to_string(GetLastError());

    DWORD dwSize = SizeofResource(hModule, hRsrc);
    if (0 == dwSize) return "SizeofResource failed: " + std::to_string(GetLastError());

    HGLOBAL hGlobal = LoadResource(hModule, hRsrc);
    if (NULL == hGlobal) return "LoadResource failed: " + std::to_string(GetLastError());

    LPVOID pBuffer = LockResource(hGlobal);
    if (NULL == pBuffer) return "LockResource failed: " + std::to_string(GetLastError());

    std::string rule(static_cast<char*>(pBuffer));

    GlobalUnlock(hGlobal);

    return rule;
}

const char* Rule()
{
    static std::string rule = LoadText(IDR_TEXT1_RULE, TEXT("Text"));
    return rule.c_str();
}

#elif __linux__

const char* Rule() { return _binary_rule_md_start; }

#endif

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
