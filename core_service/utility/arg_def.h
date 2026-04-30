// Copyright (c) 2026-present LGTBot contributors. All rights reserved.
//
// Local mirror of proto ArgDef / CommandDef (Phase 2); Phase 3 may replace with
// generated protobuf messages.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

struct VoidArgDef {
    std::string keyword;
};
struct BoolArgDef {
    std::string true_str;
    std::string false_str;
};
struct AltArgDef {
    std::vector<std::string> options;
};
struct IntArgDef {
    int64_t min = 0;
    int64_t max = 0;
    std::string meaning;
};
struct TextArgDef {
    std::string meaning;
};

struct ArgDef;

struct OptArgDef {
    std::unique_ptr<ArgDef> inner;
};
struct RepArgDef {
    std::unique_ptr<ArgDef> inner;
};

using ArgVariant = std::variant<VoidArgDef, BoolArgDef, AltArgDef, IntArgDef, TextArgDef,
                                std::unique_ptr<OptArgDef>, std::unique_ptr<RepArgDef>>;

inline ArgVariant CloneArgVariant(const ArgVariant& src);

struct ArgDef {
    std::string format_info;
    std::string example;
    ArgVariant arg;

    ArgDef() = default;
    ArgDef(const ArgDef& other);
    ArgDef& operator=(const ArgDef& other);
    ArgDef(ArgDef&&) noexcept = default;
    ArgDef& operator=(ArgDef&&) noexcept = default;

    static ArgDef TextFallback(const std::string& format_info, const std::string& example)
    {
        ArgDef d;
        d.format_info = format_info;
        d.example = example;
        d.arg = TextArgDef{"?"};
        return d;
    }
};

inline ArgVariant CloneArgVariant(const ArgVariant& src)
{
    return std::visit(
            [](const auto& alt) -> ArgVariant
            {
                using T = std::decay_t<decltype(alt)>;
                if constexpr (std::is_same_v<T, VoidArgDef>) {
                    return alt;
                } else if constexpr (std::is_same_v<T, BoolArgDef>) {
                    return alt;
                } else if constexpr (std::is_same_v<T, AltArgDef>) {
                    return alt;
                } else if constexpr (std::is_same_v<T, IntArgDef>) {
                    return alt;
                } else if constexpr (std::is_same_v<T, TextArgDef>) {
                    return alt;
                } else if constexpr (std::is_same_v<T, std::unique_ptr<OptArgDef>>) {
                    auto p = std::make_unique<OptArgDef>();
                    if (alt && alt->inner) {
                        p->inner = std::make_unique<ArgDef>(*alt->inner);
                    }
                    return p;
                } else if constexpr (std::is_same_v<T, std::unique_ptr<RepArgDef>>) {
                    auto p = std::make_unique<RepArgDef>();
                    if (alt && alt->inner) {
                        p->inner = std::make_unique<ArgDef>(*alt->inner);
                    }
                    return p;
                } else {
                    return alt;
                }
            },
            src);
}

inline ArgDef::ArgDef(const ArgDef& other)
    : format_info(other.format_info)
    , example(other.example)
    , arg(CloneArgVariant(other.arg))
{
}

inline ArgDef& ArgDef::operator=(const ArgDef& other)
{
    if (this != &other) {
        format_info = other.format_info;
        example = other.example;
        arg = CloneArgVariant(other.arg);
    }
    return *this;
}

struct CommandDefEntry {
    std::string description;
    std::vector<ArgDef> args;
    bool is_visible = true;
};
