#include "bot_core/subprocess.h"

#include <cstdint>
#include <cstring>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace {

#ifdef _WIN32

static std::wstring QuoteWinArg(const std::string& s)
{
    std::wstring r = L"\"";
    for (const unsigned char c : s) {
        if (c == '"') {
            r += L"\\\"";
        } else {
            r += static_cast<wchar_t>(c);
        }
    }
    r += L'"';
    return r;
}

bool SpawnWindows(std::vector<std::string> argv, FILE** stdin_write, FILE** stdout_read, void** proc_out, std::string& error_out)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdin_rd = nullptr;
    HANDLE stdin_wr = nullptr;
    HANDLE stdout_rd = nullptr;
    HANDLE stdout_wr = nullptr;
    if (!CreatePipe(&stdin_rd, &stdin_wr, &sa, 0) || !CreatePipe(&stdout_rd, &stdout_wr, &sa, 0)) {
        error_out = "CreatePipe failed";
        return false;
    }
    if (!SetHandleInformation(stdin_wr, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(stdout_rd, HANDLE_FLAG_INHERIT, 0)) {
        error_out = "SetHandleInformation failed";
        CloseHandle(stdin_rd);
        CloseHandle(stdin_wr);
        CloseHandle(stdout_rd);
        CloseHandle(stdout_wr);
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdin_rd;
    si.hStdOutput = stdout_wr;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::wstring cmd;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i) {
            cmd += L' ';
        }
        cmd += QuoteWinArg(argv[i]);
    }

    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutable_cmd(cmd.begin(), cmd.end());
    mutable_cmd.push_back(L'\0');
    if (!CreateProcessW(nullptr, mutable_cmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        error_out = "CreateProcessW failed";
        CloseHandle(stdin_rd);
        CloseHandle(stdin_wr);
        CloseHandle(stdout_rd);
        CloseHandle(stdout_wr);
        return false;
    }
    CloseHandle(stdin_rd);
    CloseHandle(stdout_wr);
    CloseHandle(pi.hThread);

    const int fd_in = _open_osfhandle(reinterpret_cast<intptr_t>(stdin_wr), _O_WRONLY);
    const int fd_out = _open_osfhandle(reinterpret_cast<intptr_t>(stdout_rd), _O_RDONLY);
    if (fd_in < 0 || fd_out < 0) {
        error_out = "_open_osfhandle failed";
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(stdin_wr);
        CloseHandle(stdout_rd);
        return false;
    }
    *stdin_write = _fdopen(fd_in, "wb");
    *stdout_read = _fdopen(fd_out, "rb");
    *proc_out = pi.hProcess;
    return *stdin_write && *stdout_read;
}

#else

bool SpawnPosix(std::vector<std::string> argv, FILE** stdin_write, FILE** stdout_read, int* pid_out, std::string& error_out)
{
    int p2c[2];
    int c2p[2];
    if (pipe(p2c) != 0 || pipe(c2p) != 0) {
        error_out = "pipe failed";
        return false;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        error_out = "fork failed";
        close(p2c[0]);
        close(p2c[1]);
        close(c2p[0]);
        close(c2p[1]);
        return false;
    }
    if (pid == 0) {
        close(p2c[1]);
        close(c2p[0]);
        dup2(p2c[0], STDIN_FILENO);
        dup2(c2p[1], STDOUT_FILENO);
        close(p2c[0]);
        close(c2p[1]);

        std::vector<char*> args;
        args.reserve(argv.size() + 1);
        for (auto& s : argv) {
            args.push_back(s.data());
        }
        args.push_back(nullptr);
        execvp(args[0], args.data());
        _exit(127);
    }
    close(p2c[0]);
    close(c2p[1]);
    *pid_out = static_cast<int>(pid);
    *stdin_write = fdopen(p2c[1], "wb");
    *stdout_read = fdopen(c2p[0], "rb");
    if (!*stdin_write || !*stdout_read) {
        error_out = "fdopen failed";
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
        return false;
    }
    return true;
}

#endif

} // namespace

Subprocess::Subprocess(std::vector<std::string> argv, std::string& error_out)
{
    if (argv.empty() || argv[0].empty()) {
        error_out = "empty argv";
        return;
    }
#ifdef _WIN32
    void* proc = nullptr;
    if (!SpawnWindows(std::move(argv), &child_stdin_, &child_stdout_, &proc, error_out)) {
        return;
    }
    process_handle_ = proc;
#else
    int pid = -1;
    if (!SpawnPosix(std::move(argv), &child_stdin_, &child_stdout_, &pid, error_out)) {
        return;
    }
    pid_ = pid;
#endif
    ok_ = true;
}

Subprocess::~Subprocess()
{
    request_stop();
    wait_exit();
    if (child_stdin_) {
        fclose(child_stdin_);
    }
    if (child_stdout_) {
        fclose(child_stdout_);
    }
#ifdef _WIN32
    if (process_handle_) {
        CloseHandle(process_handle_);
    }
#endif
}

void Subprocess::request_stop()
{
#ifdef _WIN32
    if (process_handle_) {
        TerminateProcess(process_handle_, 1);
    }
#else
    if (pid_ > 0) {
        kill(pid_, SIGTERM);
    }
#endif
}

void Subprocess::wait_exit()
{
#ifdef _WIN32
    if (process_handle_) {
        WaitForSingleObject(process_handle_, INFINITE);
    }
#else
    if (pid_ > 0) {
        int st = 0;
        waitpid(pid_, &st, 0);
        pid_ = -1;
    }
#endif
}
