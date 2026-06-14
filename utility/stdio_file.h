// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cstdio>
#include <utility>

// Owns a stdio FILE* and closes it on destruction. Movable, not copyable.
class StdioFile
{
  public:
    StdioFile() = default;

    explicit StdioFile(FILE* const file) : file_(file) {}

    ~StdioFile() { reset(); }

    StdioFile(const StdioFile&) = delete;
    StdioFile& operator=(const StdioFile&) = delete;

    StdioFile(StdioFile&& other) noexcept : file_(std::exchange(other.file_, nullptr)) {}

    StdioFile& operator=(StdioFile&& other) noexcept
    {
        if (this != &other) {
            reset();
            file_ = std::exchange(other.file_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] FILE* get() const { return file_; }

    [[nodiscard]] explicit operator bool() const { return file_ != nullptr; }

    FILE* release()
    {
        return std::exchange(file_, nullptr);
    }

    void reset(FILE* const file = nullptr)
    {
        if (file_) {
            std::fclose(file_);
        }
        file_ = file;
    }

  private:
    FILE* file_{nullptr};
};
