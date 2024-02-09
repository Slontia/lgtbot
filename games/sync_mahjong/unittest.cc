// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"
#include "game_util/mahjong_util.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

struct Tiles {
    struct PlayerTiles {
        std::string yama_; // 18
        std::string hand_; // 13
    };
    PlayerTiles player_tiles_[4];
    std::string doras_;
};

std::string TilesToString(const Tiles& tiles)
{
    std::string s;
    const auto append_tile_str = [&](const auto& str, const size_t size)
        {
            std::string errstr;
            auto decoded_str = game_util::mahjong::DecodeTilesString(str, errstr);
            if (!errstr.empty()) {
                std::cout << "Invalid tile string: " << str << ", error message: " << errstr << std::endl;
                assert(false);
            }
            if (decoded_str.size() / 2 > size) {
                std::cout << "Tile string is longer than size " << size << ", tile string: " << str << ", decoded string: " << decoded_str << std::endl;
                assert(false);
            }
            s += decoded_str;
            for (size_t i = decoded_str.size() / 2; i < size; ++i) {
                s += "1m";
            }
        };
    append_tile_str(tiles.doras_, 8);
    for (const Tiles::PlayerTiles& player_tiles : tiles.player_tiles_) {
        append_tile_str(player_tiles.yama_, 19);
        append_tile_str(player_tiles.hand_, 13);
    }
    return s;
}

GAME_TEST(3, player_not_enough)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(4, nari_multiple_times)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z4p2s",
                            .hand_ = "13m22s333p444p123z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "2m",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "2s",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "3p",
                        },
                    }
                }));
    START_GAME();

    for (int pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 3, "摸切");

    for (int pid = 1; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸牌");
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(OK, 0, "碰 2s");
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(OK, 0, "杠 3p"); // obtain 4p
    ASSERT_PRI_MSG(OK, 0, "杠 4p"); // obtain 2s
    ASSERT_PRI_MSG(OK, 0, "杠 2s"); // obtain ??
    ASSERT_PRI_MSG(OK, 0, "2z");
    ASSERT_PRI_MSG(OK, 0, "吃 13m 2m");
    ASSERT_PRI_MSG(OK, 0, "3z");
    ASSERT_PRI_MSG(CONTINUE, 0, "结束");
}

GAME_TEST(4, nine_types_of_nine_tiles)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1s",
                            .hand_ = "19s19p19m12z34567m",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "3z",
                            .hand_ = "1s19p19m11234567z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "流局");
    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_PRI_MSG(OK, 1, "流局");
    ASSERT_PRI_MSG(OK, 2, "摸切");
    ASSERT_PRI_MSG(CHECKOUT, 3, "摸切");

    ASSERT_FINISHED(false);
}

GAME_TEST(4, four_winds_consecutively_kiri)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(OK, 1, "1z");
    ASSERT_PRI_MSG(OK, 2, "1z");
    ASSERT_PRI_MSG(CHECKOUT, 3, "1z");

    ASSERT_FINISHED(false);
}

GAME_TEST(4, after_kan_not_four_winds_consecutively_kiri)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1s1z",
                            .hand_ = "1s1s1s",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "杠 1s");
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(OK, 1, "1z");
    ASSERT_PRI_MSG(OK, 2, "1z");
    ASSERT_PRI_MSG(CONTINUE, 3, "1z");
}

GAME_TEST(4, four_players_richii)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "2z",
                            .hand_ = "19m19s19p1345677z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .hand_ = "123456789m123s1z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "123456789m123s1z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "123456789m123s1z",
                        },
                    }
                }));
    START_GAME();

    for (int pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 3, "摸切");

    for (int pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸牌");
        ASSERT_PRI_MSG(OK, pid, "立直 摸切");
    }
    ASSERT_PRI_MSG(OK, 3, "摸牌");
    ASSERT_PRI_MSG(CHECKOUT, 3, "立直 摸切");

    // the second game
    ASSERT_PRI_MSG(OK, 0, "自摸");
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(
            24000 + 96000 + 4000 /*richii_points for the first game*/ + 600 /*benchang*/,
            24000 - 32000 - 200,
            24000 - 32000 - 200,
            24000 - 32000 - 200);
}

GAME_TEST(4, four_players_richii_on_different_round)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "5z2z3z4z",
                            .hand_ = "123456789m123s1z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "5z2z3z4z",
                            .hand_ = "123456789m123s1z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "5z2z3z4z",
                            .hand_ = "123456789m123s1z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "5z2z3z4z",
                            .hand_ = "123456789m123s1z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_PRI_MSG(OK, 1, "摸切");
    ASSERT_PRI_MSG(OK, 2, "摸切");
    ASSERT_PRI_MSG(CONTINUE, 3, "立直 摸切");

    ASSERT_PRI_MSG(OK, 0, "摸牌");
    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_PRI_MSG(OK, 1, "摸牌");
    ASSERT_PRI_MSG(OK, 1, "摸切");
    ASSERT_PRI_MSG(OK, 2, "摸牌");
    ASSERT_PRI_MSG(OK, 2, "立直 摸切");
    ASSERT_PRI_MSG(CONTINUE, 3, "摸切");

    ASSERT_PRI_MSG(OK, 0, "摸牌");
    ASSERT_PRI_MSG(OK, 0, "立直 摸切");
    ASSERT_PRI_MSG(OK, 1, "摸牌");
    ASSERT_PRI_MSG(OK, 1, "立直 摸切");
    ASSERT_PRI_MSG(OK, 2, "摸切");
    ASSERT_PRI_MSG(CHECKOUT, 3, "摸切");

    ASSERT_FINISHED(false);
}

GAME_TEST(4, tenhu)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "3z",
                            .hand_ = "19s19p19m12z34567m",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "8p",
                            .hand_ = "123s555s234m456m8p",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "自摸");
    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_PRI_MSG(OK, 1, "自摸");
    ASSERT_PRI_MSG(OK, 2, "摸切");
    ASSERT_PRI_MSG(CHECKOUT, 3, "摸切");

    ASSERT_SCORE(25000 - 16000, 25000 + 48000, 25000 - 16000, 25000 - 16000);
}

GAME_TEST(4, three_players_tsumo_cause_nagashi)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z1z",
                            .hand_ = "19s19p19m1234567z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "2z2z",
                            .hand_ = "19s19p19m1234567z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3z3z",
                            .hand_ = "19s19p19m1234567z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "4z4z",
                            .hand_ = "19s19p19m1234567z",
                        },
                    }
                }));
    START_GAME();

    // skip the first round
    for (uint32_t pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 3, "摸切");

    for (uint32_t pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸牌");
        ASSERT_PRI_MSG(OK, pid, "自摸");
    }
    ASSERT_PRI_MSG(OK, 3, "摸牌");
    ASSERT_PRI_MSG(CHECKOUT, 3, "摸切"); // nagashi

    ASSERT_FINISHED(false);
}

GAME_TEST(4, three_players_ron_cause_nagashi)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "5m2z",
                            .hand_ = "119s19p19m234567z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "5m3z",
                            .hand_ = "119s19p19m134567z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "5m1z",
                            .hand_ = "119s19p19m124567z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "5m4z",
                        },
                    }
                }));
    START_GAME();

    // skip the first round
    for (uint32_t pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 3, "摸切");

    // the second round -- normal stage
    for (uint32_t pid = 0; pid < 3; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸牌");
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(OK, 3, "摸牌");
    ASSERT_PRI_MSG(CONTINUE, 3, "摸切");

    // the second round -- ron stage
    ASSERT_PRI_MSG(OK, 0, "荣");
    ASSERT_PRI_MSG(OK, 1, "荣");
    ASSERT_PRI_MSG(CHECKOUT, 2, "荣"); // nagashi

    ASSERT_FINISHED(false);
}

GAME_TEST(4, ron_players_obtain_last_game_riichi_points)
{
    ASSERT_PUB_MSG(OK, 0, "局数 2");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                            .hand_ = "119s19p19m234567z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "2z",
                            .hand_ = "119s19p19m134567z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "119s19p19m124567z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "119s19p19m123567z",
                        },
                    }
                }));
    START_GAME();

    // first game
    for (uint32_t pid = 1; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 0, "立直 摸切");

    for (uint32_t i = 0; i < 17; ++i) {
        for (uint32_t pid = 1; pid < 4; ++pid) {
            ASSERT_PRI_MSG(OK, pid, "摸牌");
            ASSERT_PRI_MSG(OK, pid, "摸切");
        }
        ASSERT_PRI_MSG(CONTINUE, 0, "摸切");
    }

    for (uint32_t pid = 1; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸牌");
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CHECKOUT, 0, "摸切");

    // second game
    ASSERT_PRI_MSG(OK, 0, "自摸");
    ASSERT_PRI_MSG(OK, 1, "自摸");
    ASSERT_PRI_MSG(OK, 2, "摸切");
    ASSERT_PRI_MSG(CHECKOUT, 3, "摸切");

    ASSERT_SCORE(
            25000 - 1000 /*first_game_riichi_point*/ + 64000 + 500 /*obtain_riichi_point*/ + 400 /*benchang*/,
            25000 + 64000 + 500 /*obtain_riichi_point*/ + 400 /*benchang*/,
            25000 - 64000 - 400 /*benchang*/,
            25000 - 64000 - 400 /*benchang*/);
}

GAME_TEST(4, nyanpai_nagashi_tinpai)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "2m", // prevent nagashi mangan
                            .hand_ = "119s19p19m234567z", // tinpai
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "2m", // prevent nagashi mangan
                            .hand_ = "147m258s369p1234z",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "2m", // prevent nagashi mangan
                            .hand_ = "147m258s369p1234z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "2m", // prevent nagashi mangan
                            .hand_ = "147m258s369p1234z",
                        },
                    }
                }));
    START_GAME();

    for (uint32_t i = 0; i < 18; ++i) {
        ASSERT_TIMEOUT(CONTINUE);
    }
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(28000, 24000, 24000, 24000);
}

GAME_TEST(4, nyanpai_nagashi_mangan)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "19m19s19p1234567123456z", // nagashi mangan
                            .hand_ = "1z2z", // prevent tinpai
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "19m19s19p1234567123456z", // nagashi mangan
                            .hand_ = "1z2z", // prevent tinpai
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "2m", // prevent nagashi mangan
                            .hand_ = "1z2z", // prevent tinpai
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "2m", // prevent nagashi mangan
                            .hand_ = "1z2z", // prevent tinpai
                        },
                    }
                }));
    START_GAME();

    for (uint32_t i = 0; i < 18; ++i) {
        ASSERT_TIMEOUT(CONTINUE);
    }
    ASSERT_TIMEOUT(CHECKOUT);

    ASSERT_SCORE(25000 + 12000 - 4000, 25000 + 12000 - 4000, 25000 - 4000 * 2, 25000 - 4000 * 2);
}

GAME_TEST(4, cannot_nari_after_richii)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .hand_ = "222s12m12312344p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "2s",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3m",
                        },
                    }
                }));
    START_GAME();

    for (uint32_t pid = 1; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 0, "立直 摸切");

    ASSERT_PRI_MSG(FAILED, 0, "吃 12m 3m");
    ASSERT_PRI_MSG(FAILED, 0, "碰 2s");
    ASSERT_PRI_MSG(FAILED, 0, "杠 2s");
}

GAME_TEST(4, nari_ron)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                            .hand_ = "3456677s45m345p1z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "6s",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3m",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                    },
                    .doras_ = "3z3z", // doras do not hit
                }));
    START_GAME();

    for (uint32_t pid = 1; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");

    for (uint32_t pid = 1; pid < 4; ++pid) {
        ASSERT_PRI_MSG(OK, pid, "摸牌");
        ASSERT_PRI_MSG(OK, pid, "摸切");
    }
    ASSERT_PRI_MSG(OK, 0, "碰 6s");
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(CHECKOUT, 0, "荣"); // 断幺 1 + 三色 1 = 30 符 2 番

    ASSERT_SCORE(28000, 24000, 24000, 24000);
}

GAME_TEST(4, cannot_nari_ron_after_chi)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                            .hand_ = "3456677s45m345p1z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "6s",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3m",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(OK, 0, "吃 45m 3m");
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(FAILED, 0, "荣");
}

GAME_TEST(4, cannot_ron_after_chi)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                            .hand_ = "3456677s45m345p1z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "5s6s",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3m1z",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "6m1z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(OK, 0, "吃 45m 3m");
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(CONTINUE, 0, "结束");

    ASSERT_PRI_MSG(FAILED, 0, "荣");
}

GAME_TEST(4, cannot_richii_when_not_tinpai)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "2z",
                            .hand_ = "3456677s45m345p1z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "立直 摸切");
}

GAME_TEST(4, cannot_richii_when_not_tinpai_after_kiri)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "6s",
                            .hand_ = "3456677s45m345p1z",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(FAILED, 0, "立直 摸切");
    ASSERT_PRI_MSG(FAILED, 0, "立直 7s");
    ASSERT_PRI_MSG(OK, 0, "立直 1z");
}

GAME_TEST(4, richii_nomi_cannot_ron_in_the_same_round_due_to_no_yakus)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .hand_ = "34566677s12m345p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "3m3m",
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                    },
                    .doras_ = "3z3z", // doras do not hit
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "立直 摸切");
    ASSERT_TIMEOUT(CONTINUE);

    // skip the ron stage

    ASSERT_PRI_MSG(FAILED, 0, "荣");
    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");

    ASSERT_PRI_MSG(CHECKOUT, 0, "荣"); // 两立直 一发 40 fu 3 fan

    ASSERT_SCORE(25000 + 7700, 25000 - 7700, 25000, 25000);
}

GAME_TEST(4, cannot_nari_ron_when_self_furutin)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                            .hand_ = "3456677s45m345p1z",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "6s",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3m",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(OK, 0, "碰 6s");
    ASSERT_PRI_MSG(OK, 0, "1z");
    ASSERT_PRI_MSG(FAILED, 0, "荣");
    ASSERT_PRI_MSG(CONTINUE, 0, "结束");
}

GAME_TEST(4, cannot_ron_when_self_furutin)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                            .hand_ = "34566677s45m345p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "6s",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "3m",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                    }
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(FAILED, 0, "荣");
    ASSERT_PRI_MSG(OK, 0, "摸牌");
    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");
}

GAME_TEST(4, cannot_ron_when_richii_furutin)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .hand_ = "34566677s45m345p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "1z3m6m",
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                    },
                    .doras_ = "3z3z", // doras do not hit
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "立直 摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");
    ASSERT_PRI_MSG(CONTINUE, 0, "结束"); // do not ron 3m

    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");

    ASSERT_PRI_MSG(FAILED, 0, "荣");
    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");
}

GAME_TEST(4, can_ron_after_jiantao_before_richii)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .hand_ = "34566677s45m345p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "1z3m6m",
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                    },
                    .doras_ = "3z3z", // doras do not hit
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(OK, 0, "摸牌");
    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");
    ASSERT_PRI_MSG(CONTINUE, 0, "结束"); // do not ron 3m

    ASSERT_PRI_MSG(OK, 0, "摸牌");
    ASSERT_PRI_MSG(CONTINUE, 0, "摸切");

    ASSERT_PRI_MSG(CHECKOUT, 0, "荣"); // 40 fu 1 fan

    ASSERT_SCORE(25000 + 2000, 25000 - 2000, 25000, 25000);
}

GAME_TEST(4, lingshang_with_kan_dora)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z3m",
                            .hand_ = "34566677s12m345p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "6s",
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // do not tinpai
                        },
                    },
                    .doras_ = "3z3z2p3p", // doras do not hit
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "摸切");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(OK, 0, "杠 6s");
    ASSERT_PRI_MSG(CHECKOUT, 0, "自摸"); // 岭上 dora1 40 fu 2 fan

    ASSERT_SCORE(25000 + 3900, 25000 - 1300, 25000 - 1300, 25000 - 1300);
}

GAME_TEST(4, ron_different_points)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "1z",
                            .hand_ = "2333345s123m123p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "1s", // 平和三色dora4
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "2s", // no yakus
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "4s", // 平和dora4
                        },
                    },
                    .doras_ = "2s", // doras do not hit
                }));
    START_GAME();

    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(CHECKOUT, 0, "荣");

    ASSERT_SCORE(25000 + 9000 + 6000, 25000 - 9000, 25000, 25000 - 6000);
}

GAME_TEST(4, do_not_kiri_red_dora)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                            .hand_ = "233334m0456s678p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                        [2] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                        [3] = Tiles::PlayerTiles {
                            .yama_ = "6m",
                        },
                    },
                    .doras_ = "5z", // doras do not hit
                }));
    START_GAME();

    ASSERT_PRI_MSG(OK, 0, "5s");
    ASSERT_TIMEOUT(CONTINUE);

    ASSERT_PRI_MSG(CHECKOUT, 0, "荣"); // 断幺, red dora, 40 fu 2 fan

    ASSERT_SCORE(25000 + 3900, 25000 - 1300, 25000 - 1300, 25000 - 1300);
}

GAME_TEST(4, cannot_richii_last_round)
{
    ASSERT_PUB_MSG(OK, 0, "局数 1");
    ASSERT_PUB_MSG(OK, 0, "配牌 " + TilesToString(Tiles{
                    .player_tiles_ = {
                        [0] = Tiles::PlayerTiles {
                            .hand_ = "233334m0456s678p",
                        },
                        [1] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // not tinpai
                        },
                        [2] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // not tinpai
                        },
                        [3] = Tiles::PlayerTiles {
                            .hand_ = "1z2z", // not tinpai
                        },
                    },
                }));
    START_GAME();

    for (uint32_t i = 0; i < 18; ++i) {
        ASSERT_TIMEOUT(CONTINUE);
    }

    ASSERT_PRI_MSG(OK, 0, "摸牌");
    ASSERT_PRI_MSG(FAILED, 0, "立直 摸切");
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
