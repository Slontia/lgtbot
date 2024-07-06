// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

template <size_t k_size, typename Fn>
static constexpr auto MakeArray(const Fn fn)
{
    const auto f = [fn]<size_t ...k_indexes>(const std::index_sequence<k_indexes...>) { return std::array{fn(k_indexes)...}; };
    return f(std::make_index_sequence<k_size>());
}

