#include "match_process/child_session.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "utility/process_signals.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(const int argc, char** argv)
{
    lgtbot::InstallDefaultSignalHandlersOnce();
    // Each match runs in a fresh subprocess, so the C PRNG must be re-seeded here.
    // Otherwise, every game library calling rand() without srand() gets the same sequence (default seed 1).
    std::srand(std::chrono::steady_clock::now().time_since_epoch().count());
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    if (argc < 2) {
        return 2;
    }
    ChildGameSession session(stdin, stdout);
    std::string err;
    if (!session.LoadModule(argv[1], err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    return session.RunLoop();
}
