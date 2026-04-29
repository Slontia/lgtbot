#pragma once

#include <cstdio>
#include <string>

// Length-prefixed binary frames: 4-byte big-endian length + payload bytes.
// The payload interpretation (JSON or protobuf) is left to the caller.
bool WriteFrame(FILE* out, const std::string& payload);

// Returns false on EOF or malformed length.
bool ReadFrame(FILE* in, std::string& payload_out);
