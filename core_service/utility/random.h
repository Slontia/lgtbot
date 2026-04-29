// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cstdint>
#include <iterator>
#include <random>
#include <string_view>
#include <utility>

// Returns a seeded mt19937 using std::seed_seq, matching libstdc++ behavior.
// If seed is empty, uses std::random_device for non-deterministic output.
inline std::mt19937 MakeRng(const std::string_view seed)
{
    if (seed.empty()) {
        return std::mt19937(std::random_device{}());
    }
    std::seed_seq seq(seed.begin(), seed.end());
    return std::mt19937(seq);
}

// Lemire's nearly divisionless algorithm matching libstdc++ uniform_int_distribution
// for mt19937 (urngrange == UINT32_MAX). Returns unbiased value in [0, range).
// Reference: https://arxiv.org/abs/1805.10941
inline uint32_t RandBelow_(std::mt19937& g, const uint32_t range)
{
    uint64_t product = static_cast<uint64_t>(g()) * static_cast<uint64_t>(range);
    uint32_t low = static_cast<uint32_t>(product);
    if (low < range) {
        const uint32_t threshold = static_cast<uint32_t>(-range) % range;
        while (low < threshold) {
            product = static_cast<uint64_t>(g()) * static_cast<uint64_t>(range);
            low = static_cast<uint32_t>(product);
        }
    }
    return static_cast<uint32_t>(product >> 32);
}

// Matches libstdc++ __gen_two_uniform_ints: generates one value in [0, b0*b1)
// and splits it into a pair (x/b1, x%b1), consuming only one RNG call.
inline std::pair<uint32_t, uint32_t> GenTwoUniformInts_(std::mt19937& g, const uint32_t b0, const uint32_t b1)
{
    const uint32_t x = RandBelow_(g, b0 * b1);
    return {x / b1, x % b1};
}

// Matches libstdc++ std::shuffle behavior exactly for mt19937.
// For small ranges (urngrange/urange >= urange, always true for mt19937 + reasonable sizes),
// uses the paired-swap path that generates two positions per RNG call.
template <typename RandomIt>
void SeededShuffle(RandomIt first, RandomIt last, std::mt19937& g)
{
    using diff_t = typename std::iterator_traits<RandomIt>::difference_type;
    const diff_t n = last - first;
    if (n <= 1) {
        return;
    }

    // libstdc++ shuffle fast path: urngrange/urange >= urange
    // For mt19937, urngrange = UINT32_MAX. This holds when n <= 65536.
    // For game use cases this is always true; fall back to simple path otherwise.
    if (static_cast<uint64_t>(n) * n <= static_cast<uint64_t>(UINT32_MAX) + 1) {
        RandomIt i = first + 1;
        if (n % 2 == 0) {
            // odd number of swaps: handle first element separately
            std::iter_swap(i++, first + RandBelow_(g, 2));
        }
        while (i != last) {
            const diff_t swap_range = i - first + 1;
            const auto [p0, p1] = GenTwoUniformInts_(g, static_cast<uint32_t>(swap_range), static_cast<uint32_t>(swap_range + 1));
            std::iter_swap(i++, first + p0);
            std::iter_swap(i++, first + p1);
        }
    } else {
        // Fallback: standard Fisher-Yates with Lemire for large arrays
        for (diff_t i = n - 1; i > 0; --i) {
            std::iter_swap(first + i, first + RandBelow_(g, static_cast<uint32_t>(i + 1)));
        }
    }
}

// Deterministic uniform integer in [min, max] matching libstdc++ uniform_int_distribution.
inline uint32_t RandInt(std::mt19937& g, const uint32_t min, const uint32_t max)
{
    return min + RandBelow_(g, max - min + 1);
}
