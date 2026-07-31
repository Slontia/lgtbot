#pragma once

#include <memory>
#include <string>
#include <vector>

struct reproc_t;

// Spawns a child process: stdin/stdout reproc pipes for IPC; stderr inherited for logs.
class Subprocess
{
  public:
    Subprocess(const std::vector<std::string>& argv);
    ~Subprocess();

    Subprocess(const Subprocess&) = delete;
    Subprocess& operator=(const Subprocess&) = delete;
    Subprocess(Subprocess&&) noexcept = default;
    Subprocess& operator=(Subprocess&&) noexcept = default;

    [[nodiscard]] bool Ok() const { return static_cast<bool>(process_); }

    // Length-prefixed frame write/read on stdin/stdout pipes.
    [[nodiscard]] bool Write(const std::string& payload);
    [[nodiscard]] bool Read(std::string& payload_out);

    // Close IPC stdin, terminate the child and reap. Does not close stdout; readers observe EOF/EPIPE.
    void Close() noexcept;

  private:
    struct ReprocDeleter
    {
        void operator()(reproc_t* process) const noexcept;
    };

    [[nodiscard]] static std::unique_ptr<reproc_t, ReprocDeleter> Start_(const std::vector<std::string>& argv);

    std::unique_ptr<reproc_t, ReprocDeleter> process_;
};
