#include "resource_loader.h"

HMODULE hModule;

std::string LoadText(const int idr_rule, const WCHAR* type)
{
  if (hModule == NULL)
    return "hmod is NULL";

  HRSRC hRsrc = FindResource(hModule, MAKEINTRESOURCE(idr_rule), type);
  if (NULL == hRsrc)
    return "FindResource failed: " + std::to_string(GetLastError());

  DWORD dwSize = SizeofResource(hModule, hRsrc);
  if (0 == dwSize)
    return "SizeofResource failed: " + std::to_string(GetLastError());

  HGLOBAL hGlobal = LoadResource(hModule, hRsrc);
  if (NULL == hGlobal)
    return "LoadResource failed: " + std::to_string(GetLastError());

  LPVOID pBuffer = LockResource(hGlobal);
  if (NULL == pBuffer)
    return "LockResource failed: " + std::to_string(GetLastError());

  std::string rule(static_cast<char*>(pBuffer));

  GlobalUnlock(hGlobal);

  return rule;
}