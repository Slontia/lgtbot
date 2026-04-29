// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <csignal>

namespace lgtbot {

// Call once per process (safe to call from multiple threads; first call wins).
//
// On platforms that define SIGPIPE (typical POSIX: Linux, macOS, MinGW): install SIG_IGN so
// writes to a pipe/socket whose peer has closed fail with errno instead of terminating the process.
// This matches how we handle config_runner / match child IPC when the child exits first.
//
// On MSVC and other environments where SIGPIPE is not defined: no-op — valid on Windows.
inline void InstallDefaultSignalHandlersOnce()
{
#if defined(SIGPIPE)
    static const int once = [] {
        (void)std::signal(SIGPIPE, SIG_IGN);
        return 0;
    }();
    (void)once;
#endif
}

} // namespace lgtbot
