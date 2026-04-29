// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "match_process/child_config_session.h"

#include <cstdio>

#include "utility/process_signals.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

// argv[1] = path to game dynamic library
// argv[2] = path to game config file (optional, may be empty string)
int main(const int argc, char** argv)
{
    lgtbot::InstallDefaultSignalHandlersOnce();
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    if (argc < 2) {
        return 2;
    }
    const std::string lib_path = argv[1];
    const std::string conf_path = (argc >= 3) ? argv[2] : "";

    ChildConfigSession session(stdin, stdout);
    std::string err;
    if (!session.LoadModule(lib_path, conf_path, err)) {
        std::fprintf(stderr, "config_runner: failed to load module: %s\n", err.c_str());
        return 1;
    }
    return session.RunLoop();
}
