// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_util/mahjong_17_steps.h"

#include <ranges>

#include <gtest/gtest.h>
#include <gflags/gflags.h>

using namespace lgtbot::game_util::mahjong_17_steps;

class TestMahjong17Steps : public testing::Test
{
  public:
    TestMahjong17Steps()
        : table_(Mahjong17StepsOption{
                .dora_num_ = 0,
                .player_descs_{
                       PlayerDesc{"赤木", "", "", Wind::East, 25000},
                       PlayerDesc{"安冈", "", "", Wind::South, 25000},
                       PlayerDesc{"鹫巢", "", "", Wind::West, 25000},
                       PlayerDesc{"铃木", "", "", Wind::North, 25000},
                    }})
    {}
  protected:
    Mahjong17Steps table_;
};

TEST_F(TestMahjong17Steps, get_init_hand_tiles)
{
    const auto& tiles = table_.players_[0].hand_;
    ASSERT_EQ(0, tiles.size());
}

TEST_F(TestMahjong17Steps, add_tile_to_hand_exceed)
{
    const auto& tiles = table_.players_[0].yama_;
    auto it = tiles.begin();
    for (uint32_t i = 0; i < 13; ++i, ++it) {
        EXPECT_TRUE(table_.AddToHand(0, tiles.begin()->to_simple_string())) << table_.ErrorStr();
    }
    EXPECT_EQ(21, table_.players_[0].yama_.size());
    ASSERT_FALSE(table_.AddToHand(0, tiles.begin()->to_simple_string())) << table_.ErrorStr();
}

TEST_F(TestMahjong17Steps, add_tile_to_hand_not_has)
{
    const auto& yama = table_.players_[0].yama_;
    for (char d = '0'; d <= '9'; ++d) {
        for (char c : "msp") {
            char s[] = {d, c, 0};
            ASSERT_TRUE(yama.find(TileIdent(d, c)) != yama.end() || !table_.AddToHand(0, s)) << table_.ErrorStr();
        }
    }
    EXPECT_EQ(34, table_.players_[0].yama_.size());
}

TEST_F(TestMahjong17Steps, decode_invalid_z_color_tiles)
{
    std::string errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("0z", errstr).size()) << errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("8z", errstr).size()) << errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("9z", errstr).size()) << errstr;
    for (uint32_t i = 1; i < 8; ++i) {
        char s[] = {static_cast<char>('0' + i), 'z', 0};
        EXPECT_TRUE(Mahjong17Steps::DecodeTilesString_(s, errstr).size()) << errstr;
    }
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("0z1z", errstr).size()) << errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("1z0z", errstr).size()) << errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("01z", errstr).size()) << errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("10z", errstr).size()) << errstr;
    EXPECT_TRUE(Mahjong17Steps::DecodeTilesString_("1z2z", errstr).size()) << errstr;
    EXPECT_TRUE(Mahjong17Steps::DecodeTilesString_("12z", errstr).size()) << errstr;
}

TEST_F(TestMahjong17Steps, decode_too_long_tiles)
{
    std::string errstr;
    EXPECT_FALSE(Mahjong17Steps::DecodeTilesString_("0m1m2m3m4m5m6m7m8m9m0p1p2p3p4p5p6p7p8p9", errstr).size()) << errstr;
}

TEST_F(TestMahjong17Steps, get_more_same_tiles_from_yama)
{
    std::string str = table_.players_[0].yama_.begin()->to_simple_string();
    str = str + str + str + str + str;
    EXPECT_FALSE(table_.GetTilesFrom_(table_.players_[0].yama_, str).size()) << table_.ErrorStr();
    EXPECT_EQ(34, table_.players_[0].yama_.size());
}

TEST_F(TestMahjong17Steps, remove_from_hand)
{
    const auto& yama = table_.players_[0].yama_;
    const auto& hand = table_.players_[0].hand_;
    EXPECT_TRUE(table_.AddToHand(0, yama.begin()->to_simple_string())) << table_.ErrorStr();
    EXPECT_EQ(1, hand.size());
    EXPECT_EQ(33, yama.size());
    EXPECT_TRUE(table_.RemoveFromHand(0, hand.begin()->to_simple_string())) << table_.ErrorStr();
    EXPECT_EQ(0, hand.size());
    EXPECT_EQ(34, yama.size());
}

TEST_F(TestMahjong17Steps, remove_from_hand_not_exist)
{
    const auto& yama = table_.players_[0].yama_;
    const auto& hand = table_.players_[0].hand_;
    EXPECT_FALSE(table_.RemoveFromHand(0, yama.begin()->to_simple_string())) << table_.ErrorStr();
    EXPECT_EQ(0, hand.size());
    EXPECT_EQ(34, yama.size());
}

/*
 * Test Yakus
 */

TEST_F(TestMahjong17Steps, get_listen_info_纯九莲)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_6m, 0},
        Tile{BaseTile::_7m, 0},
        Tile{BaseTile::_8m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_9m, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(9, info.size());
    for (uint8_t basetile = BaseTile::_1m; basetile <= static_cast<uint8_t>(BaseTile::_9m); ++basetile) {
        const auto it = info.find(static_cast<BaseTile>(basetile));
        EXPECT_NE(it, info.end());
        EXPECT_EQ(1, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::九莲宝灯; }));
        EXPECT_EQ(32000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_普通九莲)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_6m, 0},
        Tile{BaseTile::_7m, 0},
        Tile{BaseTile::_7m, 0},
        Tile{BaseTile::_8m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_9m, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(3, info.size());
    {
        const auto it = info.find(BaseTile::_6m);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(7, it->second.fan);
        EXPECT_EQ(12000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_8m);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(4, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::一气通贯; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::一杯口; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(10, it->second.fan);
        EXPECT_EQ(16000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_9m);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(1, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::九莲宝灯; }));
        EXPECT_EQ(0, it->second.fan);
        EXPECT_EQ(32000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_国士无双十三面)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_9s, 0},
        Tile{BaseTile::_1p, 0},
        Tile{BaseTile::_9p, 0},
        Tile{BaseTile::east, 0},
        Tile{BaseTile::south, 0},
        Tile{BaseTile::west, 0},
        Tile{BaseTile::north, 0},
        Tile{BaseTile::白, 0},
        Tile{BaseTile::发, 0},
        Tile{BaseTile::中, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(13, info.size());
    for (const auto& tile : table_.players_[0].hand_) {
        const auto it = info.find(tile.tile);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(1, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::国士无双; }));
        EXPECT_EQ(32000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_国士无双)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_9s, 0},
        Tile{BaseTile::_1p, 0},
        Tile{BaseTile::_9p, 0},
        Tile{BaseTile::east, 0},
        Tile{BaseTile::south, 0},
        Tile{BaseTile::west, 0},
        Tile{BaseTile::north, 0},
        Tile{BaseTile::白, 0},
        Tile{BaseTile::白, 0},
        Tile{BaseTile::中, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(1, info.size());
    const auto it = info.find(BaseTile::发);
    EXPECT_NE(it, info.end());
    EXPECT_EQ(1, it->second.yakus.size());
    EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::国士无双; }));
    EXPECT_EQ(32000, it->second.score1);
}

TEST_F(TestMahjong17Steps, get_listen_info_绿一色四暗刻)
{
    // 2s 3s 4s 5s
    table_.players_[0].hand_ = {
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_4s, 0},
        Tile{BaseTile::_6s, 0},
        Tile{BaseTile::_6s, 0},
        Tile{BaseTile::_6s, 0},
        Tile{BaseTile::发, 0},
        Tile{BaseTile::发, 0},
        Tile{BaseTile::发, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(4, info.size());
    for (const auto& basetile : {BaseTile::_2s, BaseTile::_3s}) {
        const auto it = info.find(basetile);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(1, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::绿一色; }));
        EXPECT_EQ(32000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_5s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(4, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::役牌_发; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三暗刻; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::混一色; }));
        EXPECT_EQ(12000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_4s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::绿一色; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::四暗刻; }));
        EXPECT_EQ(64000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_字一色大四喜)
{
    // 2s 3s 4s 5s
    table_.players_[0].hand_ = {
        Tile{BaseTile::east, 0},
        Tile{BaseTile::east, 0},
        Tile{BaseTile::east, 0},
        Tile{BaseTile::west, 0},
        Tile{BaseTile::west, 0},
        Tile{BaseTile::west, 0},
        Tile{BaseTile::north, 0},
        Tile{BaseTile::north, 0},
        Tile{BaseTile::north, 0},
        Tile{BaseTile::south, 0},
        Tile{BaseTile::south, 0},
        Tile{BaseTile::发, 0},
        Tile{BaseTile::发, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(2, info.size());
    {
        const auto it = info.find(BaseTile::south);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::大四喜; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::字一色; }));
        EXPECT_EQ(96000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::发);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::小四喜; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::字一色; }));
        EXPECT_EQ(64000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_纯全三色一杯口)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_1p, 0},
        Tile{BaseTile::_2p, 0},
        Tile{BaseTile::_2p, 0},
        Tile{BaseTile::_3p, 0},
        Tile{BaseTile::_3p, 0},
        Tile{BaseTile::_9p, 0},
        Tile{BaseTile::_9p, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(2, info.size());
    {
        const auto it = info.find(BaseTile::_1p);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(5, it->second.yakus.size());
        //std::cout << yaku_to_string(it->second.yakus[0]) << std::endl;
        //std::cout << yaku_to_string(it->second.yakus[1]) << std::endl;
        //std::cout << yaku_to_string(it->second.yakus[2]) << std::endl;
        //std::cout << yaku_to_string(it->second.yakus[3]) << std::endl;
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::平和; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::一杯口; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三色同顺; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::纯全带幺九; }));
        EXPECT_EQ(30, it->second.fu);
        EXPECT_EQ(8, it->second.fan);
        EXPECT_EQ(16000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_4p);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(3, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::平和; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三色同顺; }));
        EXPECT_EQ(30, it->second.fu);
        EXPECT_EQ(4, it->second.fan);
        EXPECT_EQ(7700, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_七对子)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_7p, 0},
        Tile{BaseTile::_7p, 0},
        Tile{BaseTile::_8p, 0},
        Tile{BaseTile::_8p, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(1, info.size());
    {
        const auto it = info.find(BaseTile::_4m);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(3, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::断幺九; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::七对子; }));
        EXPECT_EQ(25, it->second.fu);
        EXPECT_EQ(4, it->second.fan);
        EXPECT_EQ(6400, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_二杯口)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_7p, 0},
        Tile{BaseTile::_7p, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(1, info.size());
    {
        const auto it = info.find(BaseTile::_3m);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::二杯口; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(4, it->second.fan);
        EXPECT_EQ(8000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_七对子_four_same)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3p, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_4m, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(0, info.size());
}

TEST_F(TestMahjong17Steps, get_listen_info_二杯口_four_same)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_4s, 0},
        Tile{BaseTile::_4s, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(3, info.size());
    {
        const auto it = info.find(BaseTile::_3s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(3, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::二杯口; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(10, it->second.fan);
        EXPECT_EQ(16000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_4s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(50, it->second.fu);
        EXPECT_EQ(7, it->second.fan);
        EXPECT_EQ(12000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_5s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(7, it->second.fan);
        EXPECT_EQ(12000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_not_count_inner_dora)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_4s, 0},
        Tile{BaseTile::_4s, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(3, info.size());
    {
        const auto it = info.find(BaseTile::_3s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(3, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::二杯口; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(10, it->second.fan);
        EXPECT_EQ(16000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_4s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(50, it->second.fu);
        EXPECT_EQ(7, it->second.fan);
        EXPECT_EQ(12000, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_5s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(7, it->second.fan);
        EXPECT_EQ(12000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_累计役满)
{
    table_.doras_.emplace_back(Tile{BaseTile::_3m, 0}, Tile{BaseTile::_1s});
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_6m, 0},
        Tile{BaseTile::_6m, 0},
        Tile{BaseTile::_6m, 0},
        Tile{BaseTile::_7m, 0},
        Tile{BaseTile::_7m, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(2, info.size());
    for (const auto& tile : {BaseTile::_1m, BaseTile::_7m}) {
        const auto it = info.find(tile);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(7, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三暗刻; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::对对和; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::清一色; }));
        EXPECT_EQ(3, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::宝牌; }));
        EXPECT_EQ(14, it->second.fan);
        EXPECT_EQ(32000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_四暗刻单骑)
{
    table_.players_[0].hand_ = {
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_6s, 0},
        Tile{BaseTile::_6s, 0},
        Tile{BaseTile::_6s, 0},
        Tile{BaseTile::_7s, 0},
        Tile{BaseTile::_7s, 0},
        Tile{BaseTile::_7s, 0},
        Tile{BaseTile::_8s, 0},
        Tile{BaseTile::_8s, 0},
        Tile{BaseTile::_8s, 0},
        Tile{BaseTile::_9s, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(4, info.size());
    {
        const auto it = info.find(BaseTile::_9s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(1, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::四暗刻; }));
        EXPECT_EQ(32000, it->second.score1);
    }
    for (const auto& tile : {BaseTile::_7s, BaseTile::_8s}) {
        const auto it = info.find(tile);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三暗刻; }));
        EXPECT_EQ(50, it->second.fu);
        EXPECT_EQ(3, it->second.fan);
        EXPECT_EQ(6400, it->second.score1);
    }
    {
        const auto it = info.find(BaseTile::_6s);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::一杯口; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(2, it->second.fan);
        EXPECT_EQ(2600, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_dora)
{
    table_.doras_.emplace_back(Tile{BaseTile::_6p, 0}, Tile{BaseTile::_8p, 0});
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_8p, 0},
        Tile{BaseTile::_9p, 0},
        Tile{BaseTile::_7p, 0},
        Tile{BaseTile::_7p, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(1, info.size());
    {
        const auto it = info.find(BaseTile::_7p);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(5, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(3, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::宝牌; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::里宝牌; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(5, it->second.fan);
        EXPECT_EQ(8000, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_一气通贯)
{
    table_.doras_.emplace_back(Tile{BaseTile::_7p, 0}, Tile{BaseTile::_8p, 0});
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_4m, 0},
        Tile{BaseTile::_5m, 0},
        Tile{BaseTile::_6m, 0},
        Tile{BaseTile::_7m, 0},
        Tile{BaseTile::_8m, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_4p, 0},
        Tile{BaseTile::_5p, 0},
        Tile{BaseTile::_6p, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(3, info.size());
    {
        for (const auto& tile : {BaseTile::_3m, BaseTile::_6m}) {
            const auto it = info.find(tile);
            EXPECT_NE(it, info.end());
            EXPECT_EQ(2, it->second.yakus.size());
            EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
            EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::平和; }));
            EXPECT_EQ(30, it->second.fu);
            EXPECT_EQ(2, it->second.fan);
            EXPECT_EQ(2000, it->second.score1);
        }
    }
    {
        const auto it = info.find(BaseTile::_9m);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(3, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::平和; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::一气通贯; }));
        EXPECT_EQ(30, it->second.fu);
        EXPECT_EQ(4, it->second.fan);
        EXPECT_EQ(7700, it->second.score1);
    }
}

TEST_F(TestMahjong17Steps, get_listen_info_not_平和)
{
    table_.doras_.emplace_back(Tile{BaseTile::_7p, 0}, Tile{BaseTile::_8p, 0});
    table_.players_[0].hand_ = {
        Tile{BaseTile::_1m, 0},
        Tile{BaseTile::_2m, 0},
        Tile{BaseTile::_3m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_1s, 0},
        Tile{BaseTile::_2s, 0},
        Tile{BaseTile::_3s, 0},
        Tile{BaseTile::_7s, 0},
        Tile{BaseTile::_8s, 0},
        Tile{BaseTile::_9s, 0},
        Tile{BaseTile::_1p, 0},
        Tile{BaseTile::_2p, 0},
        Tile{BaseTile::_3p, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(1, info.size());
    const auto it = info.find(BaseTile::_9m);
    EXPECT_NE(it, info.end());
    EXPECT_EQ(3, it->second.yakus.size());
    EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
    EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三色同顺; }));
    EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::纯全带幺九; }));
    EXPECT_EQ(40, it->second.fu);
    EXPECT_EQ(6, it->second.fan);
    EXPECT_EQ(12000, it->second.score1);
}

TEST_F(TestMahjong17Steps, get_listen_info_has_four_same_tile)
{
    table_.doras_.emplace_back(Tile{BaseTile::中, 0}, Tile{BaseTile::中, 0});
    table_.players_[0].hand_ = {
        Tile{BaseTile::_7m, 0},
        Tile{BaseTile::_8m, 0},
        Tile{BaseTile::_9m, 0},
        Tile{BaseTile::_7s, 0},
        Tile{BaseTile::_8s, 0},
        Tile{BaseTile::_9s, 0},
        Tile{BaseTile::_8p, 0},
        Tile{BaseTile::_8p, 0},
        Tile{BaseTile::_8p, 0},
        Tile{BaseTile::_8p, 0},
        Tile{BaseTile::_9p, 0},
        Tile{BaseTile::中, 0},
        Tile{BaseTile::中, 0},
    };
    auto info = table_.GetListenInfo_(0);
    EXPECT_EQ(1, info.size());
    {
        const auto it = info.find(BaseTile::_7p);
        EXPECT_NE(it, info.end());
        EXPECT_EQ(2, it->second.yakus.size());
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::立直; }));
        EXPECT_EQ(1, std::count_if(it->second.yakus.begin(), it->second.yakus.end(), [](const Yaku yaku) { return yaku == Yaku::三色同顺; }));
        EXPECT_EQ(40, it->second.fu);
        EXPECT_EQ(3, it->second.fan);
        EXPECT_EQ(5200, it->second.score1);
    }
}

class TestMahjong17StepsPlayer2 : public testing::Test
{
  public:
    TestMahjong17StepsPlayer2()
        : table_(Mahjong17StepsOption{
                .dora_num_ = 0, // keep zero because we will emplace_back to doras_ directly
                .ron_required_point_ = 8000,
                .player_descs_{
                       PlayerDesc{"赤木", "", "", Wind::East, 25000},
                       PlayerDesc{"鹫巢", "", "", Wind::West, 25000},
                    }})
    {
        table_.doras_.emplace_back(Tile{BaseTile::_6p, 0}, Tile{BaseTile::_7m, 0});
        table_.players_[0].hand_ = { // 8m
            Tile{BaseTile::_7s, 0},
            Tile{BaseTile::_8s, 0},
            Tile{BaseTile::_9s, 0},
            Tile{BaseTile::_7s, 0},
            Tile{BaseTile::_8s, 0},
            Tile{BaseTile::_9s, 0},
            Tile{BaseTile::_5p, 0},
            Tile{BaseTile::_6p, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_7m, 0},
            Tile{BaseTile::_8m, 0},
            Tile{BaseTile::_8m, 0},
            Tile{BaseTile::_9m, 0},
        };
        table_.players_[1].hand_ = { // 7p 9p
            Tile{BaseTile::_1s, 0},
            Tile{BaseTile::_2s, 0},
            Tile{BaseTile::_3s, 0},
            Tile{BaseTile::_2m, 0},
            Tile{BaseTile::_3m, 0},
            Tile{BaseTile::_4m, 0},
            Tile{BaseTile::_3m, 0},
            Tile{BaseTile::_4m, 0},
            Tile{BaseTile::_5m, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_9p, 0},
            Tile{BaseTile::_9p, 0},
        };
        table_.players_[0].listen_tiles_ = table_.GetListenInfo_(0);
        table_.players_[1].listen_tiles_ = table_.GetListenInfo_(1);
    }
  protected:
    Mahjong17Steps table_;
};

TEST_F(TestMahjong17StepsPlayer2, kiri_furutin_concurrently_cannot_ron)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::_7p, 0);
    table_.players_[1].kiri_.emplace(BaseTile::_7p, 0); // furutin
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
    table_.players_[0].kiri_.emplace(BaseTile::_7p, 0); // 1 furutin, cannot ron
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
}

TEST_F(TestMahjong17StepsPlayer2, kiri_furutin)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::中, 0);
    table_.players_[1].kiri_.emplace(BaseTile::_7p, 0); // furutin
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
    table_.players_[0].kiri_.emplace(BaseTile::_7p, 0); // 1 furutin, cannot ron
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
}

TEST_F(TestMahjong17StepsPlayer2, riichi_furutin)
{
    table_.round_ = 1;
    table_.players_[0].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 dora2, furutin
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
    table_.players_[0].kiri_.emplace(BaseTile::_7p, 0); // 1 furutin, cannot ron
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
}

TEST_F(TestMahjong17StepsPlayer2, ron_一发)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 一发 dora2
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::HAS_RON, table_.RoundOver());
    EXPECT_EQ(-8000, table_.players_[0].point_);
    EXPECT_EQ(8000, table_.players_[1].point_);
}

TEST_F(TestMahjong17StepsPlayer2, ron_河底)
{
    table_.round_ = 0;
    for (uint32_t i = 0; i < 16; ++i) {
        table_.players_[0].kiri_.emplace(BaseTile::_1p, 0);
        table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
        EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
    }
    table_.players_[0].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 河底 dora2
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::HAS_RON, table_.RoundOver());
    EXPECT_EQ(-8000, table_.players_[0].point_);
    EXPECT_EQ(8000, table_.players_[1].point_);
}

TEST_F(TestMahjong17StepsPlayer2, ron_not_count_inner_dora)
{
    table_.round_ = 1; // no 一发
    table_.players_[0].kiri_.emplace(BaseTile::中, 0);
    table_.players_[1].kiri_.emplace(BaseTile::_8m, 0); // 0 ron, 立直 一杯口 dora1 (里dora3)
    EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
}

TEST_F(TestMahjong17StepsPlayer2, all_round_finish)
{
    table_.round_ = 0;
    for (uint32_t i = 0; i < 16; ++i) {
        table_.players_[0].kiri_.emplace(BaseTile::_1p, 0);
        table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
        EXPECT_EQ(Mahjong17Steps::GameState::CONTINUE, table_.RoundOver());
    }
    table_.players_[0].kiri_.emplace(BaseTile::_1p, 0);
    table_.players_[1].kiri_.emplace(BaseTile::_1p, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::FLOW, table_.RoundOver());
    EXPECT_EQ(0, table_.players_[0].point_);
    EXPECT_EQ(0, table_.players_[1].point_);
}

TEST_F(TestMahjong17StepsPlayer2, ron_each_other)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 一发 dora2
    table_.players_[1].kiri_.emplace(BaseTile::_8m, 0); // 0 ron, 立直 一发 一杯口 dora1 (里dora3)
    EXPECT_EQ(Mahjong17Steps::GameState::HAS_RON, table_.RoundOver());
    EXPECT_EQ(0, table_.players_[0].point_);
    EXPECT_EQ(0, table_.players_[1].point_);
}

class TestMahjong17StepsPlayer3 : public testing::Test
{
  public:
    TestMahjong17StepsPlayer3()
        : table_(Mahjong17StepsOption{
                .with_inner_dora_ = false,
                .dora_num_ = 0, // keep zero because we will emplace_back to doras_ directly
                .ron_required_point_ = 8000,
                .player_descs_{
                       PlayerDesc{"赤木", "", "", Wind::East, 25000},
                       PlayerDesc{"鹫巢", "", "", Wind::West, 25000},
                       PlayerDesc{"福本", "", "", Wind::West, 25000},
                    }})
    {
        table_.doras_.emplace_back(Tile{BaseTile::_6p, 0}, Tile{BaseTile::_7m, 0});
        table_.players_[0].hand_ = { // 8m
            Tile{BaseTile::_7s, 0},
            Tile{BaseTile::_8s, 0},
            Tile{BaseTile::_9s, 0},
            Tile{BaseTile::_7s, 0},
            Tile{BaseTile::_8s, 0},
            Tile{BaseTile::_9s, 0},
            Tile{BaseTile::_5p, 0},
            Tile{BaseTile::_6p, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_7m, 0},
            Tile{BaseTile::_8m, 0},
            Tile{BaseTile::_8m, 0},
            Tile{BaseTile::_9m, 0},
        };
        table_.players_[1].hand_ = { // 7p 9p
            Tile{BaseTile::_1s, 0},
            Tile{BaseTile::_2s, 0},
            Tile{BaseTile::_3s, 0},
            Tile{BaseTile::_2m, 0},
            Tile{BaseTile::_3m, 0},
            Tile{BaseTile::_4m, 0},
            Tile{BaseTile::_2m, 0},
            Tile{BaseTile::_3m, 0},
            Tile{BaseTile::_4m, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_9p, 0},
            Tile{BaseTile::_9p, 0},
        };
        table_.players_[2].hand_ = { // 8m
            Tile{BaseTile::_7s, 0},
            Tile{BaseTile::_8s, 0},
            Tile{BaseTile::_9s, 0},
            Tile{BaseTile::_7s, 0},
            Tile{BaseTile::_8s, 0},
            Tile{BaseTile::_9s, 0},
            Tile{BaseTile::_5p, 0},
            Tile{BaseTile::_6p, 0},
            Tile{BaseTile::_7p, 0},
            Tile{BaseTile::_7m, 0},
            Tile{BaseTile::_8m, 0},
            Tile{BaseTile::_8m, 0},
            Tile{BaseTile::_9m, 0},
        };
        table_.players_[0].listen_tiles_ = table_.GetListenInfo_(0);
        table_.players_[1].listen_tiles_ = table_.GetListenInfo_(1);
        table_.players_[2].listen_tiles_ = table_.GetListenInfo_(2);
    }
  protected:
    Mahjong17Steps table_;
};

TEST_F(TestMahjong17StepsPlayer3, one_ron_two_same)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 一发 dora2
    table_.players_[1].kiri_.emplace(BaseTile::中, 0);
    table_.players_[2].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 一发 dora2
    EXPECT_EQ(Mahjong17Steps::GameState::HAS_RON, table_.RoundOver());
    EXPECT_EQ(-4000, table_.players_[0].point_);
    EXPECT_EQ(8000, table_.players_[1].point_);
    EXPECT_EQ(-4000, table_.players_[2].point_);
}

TEST_F(TestMahjong17StepsPlayer3, one_ron_two_diff)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::中, 0);
    table_.players_[1].kiri_.emplace(BaseTile::_8m, 0); // 0 1 ron, 立直 一发 一杯口 dora1 (里dora3)
    table_.players_[2].kiri_.emplace(BaseTile::中, 0);
    EXPECT_EQ(Mahjong17Steps::GameState::HAS_RON, table_.RoundOver());
    EXPECT_EQ(8000, table_.players_[0].point_);
    EXPECT_EQ(-16000, table_.players_[1].point_);
    EXPECT_EQ(8000, table_.players_[2].point_);
}

TEST_F(TestMahjong17StepsPlayer3, two_ron_one)
{
    table_.round_ = 0;
    table_.players_[0].kiri_.emplace(BaseTile::_7p, 0); // 1 ron, 立直 一发 一杯口 dora3
    table_.players_[1].kiri_.emplace(BaseTile::中, 0);
    table_.players_[2].kiri_.emplace(BaseTile::_9p, 0); // 1 ron, 立直 一发 一杯口 dora2
    EXPECT_EQ(Mahjong17Steps::GameState::HAS_RON, table_.RoundOver());
    EXPECT_EQ(-12000, table_.players_[0].point_);
    EXPECT_EQ(12000, table_.players_[1].point_);
    EXPECT_EQ(0, table_.players_[2].point_);
}

void ShowImage()
{
    std::cout << "<!--" << std::endl;
    Mahjong17StepsOption option;
    option.image_path_ = "/home/bjcwgqm/projects/lgtbot/src/lgtbot/games/mahjong_17_steps/resource";
    option.seed_ = "sdfsf";
    option.name_ = "东四局";
    option.dora_num_ = 1;
    option.ron_required_point_ = 0;
    option.player_descs_.emplace_back("赤木茂", "", "", Wind::East, 31000);
    option.player_descs_.emplace_back("伊藤开司", "", "", Wind::West, 19000);
    option.player_descs_.emplace_back("天贵史", "", "", Wind::South, 25000);
    Mahjong17Steps table(option);
    table.AddToHand(0, "456s4466677p777z");
    table.AddToHand(1, "888m555p222z3366z");
    table.AddToHand(2, "77889s223344p55z");
    //table.AddToHand(0, "1m66s4466677p777z");
    //table.AddToHand(0, "2344666778p777z");
    //table.AddToHand(0, "66s4466677p777z");
    table.Kiri(0, "1s");
    table.Kiri(1, "4z");
    table.Kiri(2, "4z");
    table.RoundOver();
    table.Kiri(0, "6s");
    table.Kiri(1, "9s");
    table.Kiri(2, "4z");
    table.RoundOver();
    std::cout << "-->" << std::endl;
//    std::cout << "-->" << std::endl;
//    std::cout << table.PrepareHtml(0) << "\n\n";
//    std::cout << table.PrepareHtml(1) << "\n\n";
//    std::cout << table.PrepareHtml(2) << "\n\n";
    //std::cout << table.PublicHtml();
    std::cout << table.KiriHtml(0);

}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
    //ShowImage();
    return 0;
}
