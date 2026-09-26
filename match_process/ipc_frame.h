#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <string>

inline constexpr uint32_t kMaxIpcFramePayloadSize = 64u * 1024u * 1024u;

[[nodiscard]] inline bool IpcFramePayloadSizeValid(const size_t nbytes)
{
    return nbytes <= kMaxIpcFramePayloadSize;
}

// Length-prefixed binary frames: 4-byte big-endian length + payload bytes.
// The payload interpretation (JSON or protobuf) is left to the caller.
bool WriteFrame(FILE* out, const std::string& payload);

// Returns false on EOF or malformed length.
bool ReadFrame(FILE* in, std::string& payload_out);
