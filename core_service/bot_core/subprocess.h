#pragma once

#include <cstdio>
#include <string>
#include <vector>

// Spawns a child process with stdin/stdout redirected to pipes (parent side: write to child stdin, read child stdout).
class Subprocess
{
  public:
    Subprocess(std::vector<std::string> argv, std::string& error_out);
    ~Subprocess();

    Subprocess(const Subprocess&) = delete;
    Subprocess& operator=(const Subprocess&) = delete;

    [[nodiscard]] bool ok() const { return ok_; }

    [[nodiscard]] FILE* child_stdin() const { return child_stdin_; }
    [[nodiscard]] FILE* child_stdout() const { return child_stdout_; }

    // Close the write end of the stdin pipe (causes the child to get EOF on its stdin).
    // Safe to call multiple times.  ~Subprocess will not double-close.
    void close_stdin()
    {
        if (child_stdin_) {
            fclose(child_stdin_);
            child_stdin_ = nullptr;
        }
    }

    void request_stop();
    void wait_exit();

  private:
    bool ok_{false};
    FILE* child_stdin_{nullptr};
    FILE* child_stdout_{nullptr};
#ifdef _WIN32
    void* process_handle_{nullptr};
#else
    int pid_{-1};
#endif
};
