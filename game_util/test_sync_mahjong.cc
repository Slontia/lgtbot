// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/sync_mahjong.h"

#include <array>
#include <ranges>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

static constexpr const uint32_t k_hand_tile_num = 13;

using namespace lgtbot::game_util::mahjong;

struct PlayerIdentity
{
    std::array<Tile, k_yama_tile_num> yama_;
    std::array<Tile, 13> hand_;
    std::array<std::pair<Tile, Tile>, 4> doras_;
};

struct TestSyncMahjong : public testing::Test
{
    std::unique_ptr<SyncMahjongGamePlayer> MakePlayer(const PlayerIdentity& identity)
    {
        TileSet hand;
        std::transform(identity.hand_.begin(), identity.hand_.end(), std::inserter(hand, hand.end()),
                [](const Tile& tile) { return tile; });
        return std::make_unique<SyncMahjongGamePlayer>(
            "", // image_path
            std::vector<PlayerDesc>(4), // player_descs
            0, // player_id
            identity.yama_,
            hand,
            identity.doras_
        );
    }
};


TEST_F(TestSyncMahjong, kiri_priority)
{
    std::unique_ptr<SyncMahjongGamePlayer> player = MakePlayer(PlayerIdentity{
                .hand_{
                    Tile{.tile = _5s, .red_dora = true, .toumei = false},
                    Tile{.tile = _5s, .red_dora = false, .toumei = false},
                },
            });
    player->round_ = 1;
    player->StartNormalStage();
    EXPECT_TRUE(player->Kiri("5s", false));
    ASSERT_EQ((Tile{.tile = _5s, .red_dora = false, .toumei = false}), player->LastKiriTile());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
