#include "match_process/ipc_frame.h"

#include <cstdint>
#include <vector>

namespace {

bool WriteAll(FILE* out, const void* data, const size_t len)
{
    const auto* p = static_cast<const unsigned char*>(data);
    size_t off = 0;
    while (off < len) {
        const size_t n = fwrite(p + off, 1, len - off, out);
        if (n == 0) {
            // With SIGPIPE ignored (see utility/process_signals.h), fwrite typically fails instead of killing the process;
            // callers treat false as a closed / broken IPC stream.
            return false;
        }
        off += n;
    }
    return fflush(out) == 0;
}

bool ReadAll(FILE* in, void* data, const size_t len)
{
    auto* p = static_cast<unsigned char*>(data);
    size_t off = 0;
    while (off < len) {
        const size_t n = fread(p + off, 1, len - off, in);
        if (n == 0) {
            return false;
        }
        off += n;
    }
    return true;
}

} // namespace

bool WriteFrame(FILE* const out, const std::string& payload)
{
    const auto len = static_cast<uint32_t>(payload.size());
    unsigned char hdr[4] = {
        static_cast<unsigned char>(len >> 24),
        static_cast<unsigned char>(len >> 16),
        static_cast<unsigned char>(len >> 8),
        static_cast<unsigned char>(len),
    };
    return WriteAll(out, hdr, sizeof(hdr)) && WriteAll(out, payload.data(), payload.size());
}

bool ReadFrame(FILE* const in, std::string& payload_out)
{
    unsigned char hdr[4];
    if (!ReadAll(in, hdr, sizeof(hdr))) {
        return false;
    }
    const uint32_t len = (uint32_t(hdr[0]) << 24) | (uint32_t(hdr[1]) << 16) | (uint32_t(hdr[2]) << 8) | uint32_t(hdr[3]);
    if (len > 64 * 1024 * 1024) {
        return false;
    }
    std::vector<char> buf(len);
    if (len != 0 && !ReadAll(in, buf.data(), len)) {
        return false;
    }
    payload_out.assign(buf.begin(), buf.end());
    return true;
}
