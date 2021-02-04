#pragma once
#ifdef _WIN32
#include <windows.h>

#elif __linux__
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
using HINSTANCE = void*;
inline auto LoadLibrary(const char* const path) { return dlopen(path, RTLD_LAZY); }
inline void FreeLibrary(HINSTANCE mod) { dlclose(mod); }
inline auto GetProcAddress(HINSTANCE mod, const char* const funcname) { return dlsym(mod, funcname); }

#else
static_assert(false, "Not support OS");

#endif

