#include "bot_core/subprocess.h"

#include <reproc/reproc.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "match_process/ipc_frame.h"
#include "utility/log.h"

namespace {

std::string JoinArgv(const std::vector<std::string>& argv)
{
    std::string joined;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i != 0) {
            joined += ' ';
        }
        joined += argv[i];
    }
    return joined;
}

bool WriteAll(reproc_t* process, const void* data, const size_t len)
{
    const auto* const p = static_cast<const uint8_t*>(data);
    size_t off = 0;
    while (off < len) {
        const int r = reproc_write(process, p + off, len - off);
        if (r <= 0) {
            return false;
        }
        off += static_cast<size_t>(r);
    }
    return true;
}

bool ReadAll(reproc_t* process, void* data, const size_t len)
{
    auto* const p = static_cast<uint8_t*>(data);
    size_t off = 0;
    while (off < len) {
        const int r = reproc_read(process, REPROC_STREAM_OUT, p + off, len - off);
        if (r <= 0) {
            return false;
        }
        off += static_cast<size_t>(r);
    }
    return true;
}

} // namespace

std::unique_ptr<reproc_t, Subprocess::ReprocDeleter> Subprocess::Start_(const std::vector<std::string>& argv)
{
    std::vector<const char*> arg_ptrs;
    arg_ptrs.reserve(argv.size() + 1);
    for (const auto& s : argv) {
        arg_ptrs.push_back(s.c_str());
    }
    arg_ptrs.push_back(nullptr);

    std::unique_ptr<reproc_t, ReprocDeleter> process(reproc_new());
    if (!process) {
        ErrorLog() << "Subprocess: reproc_new failed, argv=" << JoinArgv(argv);
        return {};
    }

    // stdin/stdout -> reproc pipes (IPC); stderr -> parent (logs).
    // Note: nested designated initializers (.redirect.err.type = ...) are a C99/clang extension rejected by GCC in C++, so assign the fields instead.
    reproc_options options{};
    options.redirect.err.type = REPROC_REDIRECT_PARENT;
    options.stop = reproc_stop_actions{
        {REPROC_STOP_TERMINATE, 0},
        {REPROC_STOP_KILL, 0},
        {REPROC_STOP_WAIT, REPROC_INFINITE},
    };
    if (const int r = reproc_start(process.get(), arg_ptrs.data(), options); r < 0) {
        ErrorLog() << "Subprocess: reproc_start failed, r=" << r << " (" << reproc_strerror(r) << "), argv=" << JoinArgv(argv);
        return {};
    }

    return process;
}

void Subprocess::ReprocDeleter::operator()(reproc_t* const process) const noexcept
{
    reproc_destroy(process);
}

Subprocess::Subprocess(const std::vector<std::string>& argv) : process_(Start_(argv))
{
}

Subprocess::~Subprocess()
{
    Close();
}

bool Subprocess::Write(const std::string& payload)
{
    if (!process_) {
        ErrorLog() << "Subprocess: Write called with no running process";
        return false;
    }
    if (!IpcFramePayloadSizeValid(payload.size())) {
        ErrorLog() << "Subprocess: Write payload too large, pid=" << reproc_pid(process_.get())
                   << ", payload_size=" << payload.size() << ", max=" << kMaxIpcFramePayloadSize;
        return false;
    }
    const auto len = static_cast<uint32_t>(payload.size());
    const unsigned char hdr[4] = {
        static_cast<unsigned char>(len >> 24),
        static_cast<unsigned char>(len >> 16),
        static_cast<unsigned char>(len >> 8),
        static_cast<unsigned char>(len),
    };
    if (!WriteAll(process_.get(), hdr, sizeof(hdr))) {
        ErrorLog() << "Subprocess: Write failed (frame header), pid=" << reproc_pid(process_.get())
                   << ", payload_size=" << payload.size();
        return false;
    }
    if (!WriteAll(process_.get(), payload.data(), payload.size())) {
        ErrorLog() << "Subprocess: Write failed (frame payload), pid=" << reproc_pid(process_.get())
                   << ", payload_size=" << payload.size();
        return false;
    }
    return true;
}

bool Subprocess::Read(std::string& payload_out)
{
    if (!process_) {
        ErrorLog() << "Subprocess: Read called with no running process";
        return false;
    }
    unsigned char hdr[4];
    if (!ReadAll(process_.get(), hdr, sizeof(hdr))) {
        ErrorLog() << "Subprocess: Read failed (frame header), pid=" << reproc_pid(process_.get());
        return false;
    }
    const uint32_t len = (uint32_t(hdr[0]) << 24) | (uint32_t(hdr[1]) << 16) | (uint32_t(hdr[2]) << 8) | uint32_t(hdr[3]);
    if (!IpcFramePayloadSizeValid(len)) {
        ErrorLog() << "Subprocess: Read frame length too large, len=" << len << ", pid=" << reproc_pid(process_.get())
                   << ", max=" << kMaxIpcFramePayloadSize;
        return false;
    }
    std::vector<char> buf(len);
    if (len != 0 && !ReadAll(process_.get(), buf.data(), len)) {
        ErrorLog() << "Subprocess: Read failed (frame payload), pid=" << reproc_pid(process_.get()) << ", payload_len=" << len;
        return false;
    }
    payload_out.assign(buf.begin(), buf.end());
    return true;
}

void Subprocess::Close() noexcept
{
    reproc_t* const process = process_.get();
    if (!process) {
        return;
    }
    const int pid = reproc_pid(process);

    if (const int close_in = reproc_close(process, REPROC_STREAM_IN); close_in < 0) {
        ErrorLog() << "Subprocess: reproc_close(IN) failed, r=" << close_in << " (" << reproc_strerror(close_in)
                   << "), pid=" << pid;
    }

    const reproc_stop_actions stop{
        {REPROC_STOP_TERMINATE, 1000},
        {REPROC_STOP_KILL, 1000},
        {REPROC_STOP_WAIT, 2000},
    };
    if (const int stop_r = reproc_stop(process, stop); stop_r < 0) {
        ErrorLog() << "Subprocess: reproc_stop failed, r=" << stop_r << " (" << reproc_strerror(stop_r) << "), pid=" << pid;
    }
}
