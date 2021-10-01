#include "game_main.h"

#ifdef _WIN32
#include "resource_loader.h"

#define IDR_TEXT1_RULE 101

#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE 103
#define _APS_NEXT_COMMAND_VALUE 40001
#define _APS_NEXT_CONTROL_VALUE 1001
#define _APS_NEXT_SYMED_VALUE 101
#endif
#endif

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

extern char _binary_rule_md_start[];

const char* Rule() { return _binary_rule_md_start; }

#endif
