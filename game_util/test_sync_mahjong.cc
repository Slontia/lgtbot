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
    player->StartNormalStage(1);
    EXPECT_TRUE(player->Kiri("5s", false));
    ASSERT_EQ((Tile{.tile = _5s, .red_dora = false, .toumei = false}), player->LastKiriTile());
}

static constexpr uint32_t k_player_id = 0;

struct PlayerTiles
{
    struct Furu
    {
        std::vector<BaseTile> tiles_;
        Fulu::Type type_;
        int take_{0};
    };
    std::vector<BaseTile> hand_;
    std::vector<Furu> furus_;
    std::optional<BaseTile> ron_tile_;
};

static CounterResult GetCounter(PlayerTiles& player_tiles)
{
    Table table;
    table.dora_spec = 0;
    table.庄家 = k_player_id;
    table.last_action = Action::pass;
    table.players[k_player_id].first_round = false;
    table.players[k_player_id].riichi = false;
    table.players[k_player_id].double_riichi = false;
    table.players[k_player_id].亲家 = true;
    table.players[k_player_id].一发 = false;
    table.players[k_player_id].门清 = true;

    static const auto basetile_to_tile = [](const BaseTile basetile) { return Tile{.tile = basetile, .red_dora = false, .toumei = false}; };

    static const auto emplace_tiles = [](const std::vector<BaseTile>& basetiles, std::vector<Tile>& tiles, std::vector<Tile*>& tile_pointers)
        {
            std::ranges::transform(basetiles, std::back_inserter(tiles), basetile_to_tile);
            std::ranges::transform(tiles, std::back_inserter(tile_pointers), [](Tile& tile) { return &tile; });
        };

    std::vector<Tile> hand;
    emplace_tiles(player_tiles.hand_, hand, table.players[k_player_id].hand);

    std::vector<std::vector<Tile>> furu_tiles;
    std::ranges::transform(player_tiles.furus_, std::back_inserter(table.players[k_player_id].副露s),
            [&furu_tiles](const PlayerTiles::Furu& furu)
                {
                    Fulu fulu{
                        .take = furu.take_,
                        .type = furu.type_,
                    };
                    furu_tiles.emplace_back();
                    emplace_tiles(furu.tiles_, furu_tiles.back(), fulu.tiles);
                    return fulu;
                });

    Tile ron_tile;
    if (player_tiles.ron_tile_.has_value()) {
        ron_tile = basetile_to_tile(*player_tiles.ron_tile_);
    }
    return yaku_counter(&table, k_player_id, player_tiles.ron_tile_.has_value() ? &ron_tile : nullptr,
            false /*枪杠*/, false /*枪暗杠*/, Wind::East /*自风*/, Wind::East /*场风*/);
}

TEST(TestSyncMahjongYaku, all_green)
{
    PlayerTiles player_tiles{
        .hand_{_2s, _3s, _4s, _6s, _6s},
        .furus_{
            {
                .tiles_{发, 发, 发, 发},
                .type_ = Fulu::Type::大明杠,
            },
            {
                .tiles_{_8s, _8s, _8s},
                .type_ = Fulu::Type::Pon,
            },
            {
                .tiles_{_2s, _3s, _4s},
                .type_ = Fulu::Type::Chi,
            },
        },
    };
    const auto counter = GetCounter(player_tiles);
    EXPECT_EQ(1, std::ranges::count(counter.yakus, Yaku::绿一色));
}

TEST(TestSyncMahjongYaku, all_terminals_7_pairs)
{
    PlayerTiles player_tiles{
        .hand_{_1s, _1s, _9s, _9s, _1m, _1m, _9m, _9m, _1p, _1p, 白, 白, 发, 发},
    };
    const auto counter = GetCounter(player_tiles);
    EXPECT_EQ(1, std::ranges::count(counter.yakus, Yaku::七对子));
    EXPECT_EQ(1, std::ranges::count(counter.yakus, Yaku::混老头));
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
