// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <string>
#include <filesystem>
#include <cassert>

#include <sys/stat.h>

#include "utility/log.h"

#ifdef _WIN32
#include <cstdio>
#else
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <thread>
#endif

#ifdef TEST_BOT
inline bool enable_markdown_to_image = false;
#else
inline bool enable_markdown_to_image = true;
#endif

inline const std::string k_markdown2image_path = (std::filesystem::current_path() / "markdown2image").string(); // TODO: config

#ifndef _WIN32

namespace image_internal {

// markdown2image (Qt WebEngine) can wedge. A render is often performed while holding a match's state lock,
// so an unbounded popen()/pclose() here used to freeze the match and every global command that iterates matches. Bound the whole render instead.
inline constexpr int k_render_timeout_ms = 30 * 1000;

inline bool WaitPidUntil(const pid_t pid, const std::chrono::steady_clock::time_point deadline)
{
    while (true) {
        int status = 0;
        const pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                ErrorLog() << "markdown2image exited with status " << WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                ErrorLog() << "markdown2image killed by signal " << WTERMSIG(status);
            }
            return true;
        }
        if (r < 0 && errno != EINTR) {
            return true; // ECHILD: already reaped elsewhere, nothing left to wait for
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

inline void KillAndReap(const pid_t pid)
{
    kill(pid, SIGTERM);
    if (WaitPidUntil(pid, std::chrono::steady_clock::now() + std::chrono::milliseconds(2000))) {
        return;
    }
    kill(pid, SIGKILL);
    (void)waitpid(pid, nullptr, 0);
}

} // namespace image_internal

inline int MarkdownToImage(const std::string& markdown, const std::string& path, const uint32_t width)
{
    assert(!path.empty());
    if (!enable_markdown_to_image) {
        return false;
    }
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());

    // Build argv before fork: only async-signal-safe calls are allowed in the child.
    const std::string width_str = std::to_string(width);
    const char* const argv[] = {
        k_markdown2image_path.c_str(),
        "--output", path.c_str(),
        "--width", width_str.c_str(),
        "--nowith_css", "--noprint_info",
        nullptr,
    };

    int fds[2] = {-1, -1};
    if (pipe(fds) != 0) {
        ErrorLog() << "Draw image failed: pipe errno=" << errno << " path=" << path;
        return -1;
    }
    // CLOEXEC on both ends so concurrently spawned processes cannot inherit the write end and keep the renderer's stdin open forever.
    fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    fcntl(fds[1], F_SETFD, FD_CLOEXEC);

    const pid_t pid = fork();
    if (pid < 0) {
        ErrorLog() << "Draw image failed: fork errno=" << errno << " path=" << path;
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    if (pid == 0) {
        dup2(fds[0], STDIN_FILENO); // clears CLOEXEC on fd 0 only
        execv(argv[0], const_cast<char* const*>(argv));
        _exit(127);
    }
    close(fds[0]);
    DebugLog() << "Draw image start path=" << path << " width=" << width << " pid=" << pid;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(image_internal::k_render_timeout_ms);
    fcntl(fds[1], F_SETFL, O_NONBLOCK);
    const char* data = markdown.data();
    size_t left = markdown.size();
    bool write_ok = true;
    while (left > 0) {
        const auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
        if (remain <= 0) {
            write_ok = false;
            break;
        }
        struct pollfd pf = {fds[1], POLLOUT, 0};
        const int pr = poll(&pf, 1, static_cast<int>(remain));
        if (pr < 0 && errno == EINTR) {
            continue;
        }
        if (pr <= 0) {
            write_ok = false; // timeout or poll error
            break;
        }
        const ssize_t n = write(fds[1], data, left);
        if (n > 0) {
            data += n;
            left -= n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
            continue;
        }
        write_ok = false; // EPIPE: renderer died; SIGPIPE is ignored process-wide
        break;
    }
    close(fds[1]);

    if (!write_ok) {
        ErrorLog() << "Draw image failed: feeding markdown timed out or broke, killing renderer pid=" << pid
                   << " path=" << path;
        image_internal::KillAndReap(pid);
        return -1;
    }
    if (!image_internal::WaitPidUntil(pid, deadline)) {
        ErrorLog() << "Draw image failed: renderer timed out, killing pid=" << pid << " path=" << path;
        image_internal::KillAndReap(pid);
        return -1;
    }
    return 0;
}

#else // _WIN32

inline int MarkdownToImage(const std::string& markdown, const std::string& path, const uint32_t width)
{
    assert(!path.empty());
    if (!enable_markdown_to_image) {
        return false;
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

#endif // _WIN32

inline int CharToImage(const char ch, const std::string& path)
{
    return MarkdownToImage(std::string("<style>html,body{color:#fdf3dd; background:#783623;}</style> <p align=\"middle\"><font size=\"6\"><b>") + ch + "</b></font></p>", path, 85);
}
