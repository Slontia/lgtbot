#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct reproc_t;

namespace lgtbot::core {

// Spawns a child process: stdin/stdout reproc pipes for IPC; stderr inherited for logs.
class Subprocess
{
  public:
    Subprocess(const std::vector<std::string>& argv);
    ~Subprocess();

    Subprocess(const Subprocess&) = delete;
    Subprocess& operator=(const Subprocess&) = delete;
    Subprocess(Subprocess&& rhs) noexcept
        : process_(std::move(rhs.process_))
        , cancelled_(rhs.cancelled_.load(std::memory_order_relaxed)) {}
    Subprocess& operator=(Subprocess&& rhs) noexcept
    {
        process_ = std::move(rhs.process_);
        cancelled_.store(rhs.cancelled_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    [[nodiscard]] bool Ok() const { return static_cast<bool>(process_); }

    // Length-prefixed frame write/read on stdin/stdout pipes.
    [[nodiscard]] bool Write(const std::string& payload);
    [[nodiscard]] bool Read(std::string& payload_out);

    // Idempotent, thread-safe. Sends SIGTERM to the child without waiting so any
    // concurrent Read/Write returns promptly with an error. Safe to call from any
    // thread while another thread is blocked in Read/Write. Full teardown still
    // happens in the destructor via Close().
    void Cancel() noexcept;

    // Close IPC stdin, terminate the child and reap. Does not close stdout; readers observe EOF/EPIPE.
    void Close() noexcept;

  private:
    struct ReprocDeleter
    {
        void operator()(reproc_t* process) const noexcept;
    };

    [[nodiscard]] static std::unique_ptr<reproc_t, ReprocDeleter> Start_(const std::vector<std::string>& argv);

    std::unique_ptr<reproc_t, ReprocDeleter> process_;
    std::atomic<bool> cancelled_{false};
};

} // namespace lgtbot::core
