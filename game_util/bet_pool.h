// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cassert>
#include <iostream>

template <typename T>
struct CallBetPoolInfo
{
    int64_t coins_;
    T obj_;
};

struct CallBetPoolResult
{
    int64_t total_coins_ = 0;
    std::set<uint64_t> winner_ids_;
    std::set<uint64_t> participant_ids_;
};

template <typename T>
std::vector<CallBetPoolResult> CallBetPool(const std::map<uint64_t, CallBetPoolInfo<T>>& infos)
{
    std::vector<CallBetPoolResult> result;

    // prepare datas
    std::map<T, std::set<uint64_t>> rank_map; // value is id
    struct CoinInfo
    {
        uint64_t id_;
        int64_t remain_coins_;
        const T* obj_;
    };
    std::vector<CoinInfo> coin_infos;
    std::set<uint64_t> remain_ids;
    for (const auto& [id, info] : infos) {
        rank_map[info.obj_].emplace(id);
        coin_infos.emplace_back(CoinInfo{.id_ = id, .remain_coins_ = info.coins_, .obj_ = &info.obj_});
        remain_ids.emplace(id);
    }
    std::sort(coin_infos.begin(), coin_infos.end(), [](const auto& _1, const auto& _2) { return _1.remain_coins_ < _2.remain_coins_; });

    // calculate result
    for (auto it = coin_infos.begin(); it != coin_infos.end(); ++it) {
        auto& info = *it;
        if (info.remain_coins_ > 0) {
            result.emplace_back(CallBetPoolResult{
                    .total_coins_ = info.remain_coins_ * static_cast<int64_t>(remain_ids.size()),
                    .winner_ids_ = rank_map.rbegin()->second,
                    .participant_ids_ = remain_ids
                });
            std::for_each(it, coin_infos.end(),
                    [coins = info.remain_coins_](auto& each_info) { each_info.remain_coins_ -= coins; });
        }
        if (const auto rank_map_it = rank_map.find(*info.obj_); rank_map_it->second.size() == 1) {
            rank_map.erase(rank_map_it);
        } else {
            rank_map_it->second.erase(info.id_);
        }
        remain_ids.erase(info.id_);
    }
    assert(rank_map.empty());
    assert(remain_ids.empty());
    return result;
}
