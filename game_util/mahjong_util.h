// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <random>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <span>
#include <optional>

#include "Mahjong/Table.h"

#include "utility/html.h"

static auto operator<=>(const Tile& _1, const Tile& _2)
{
    return _1.tile != _2.tile         ? _1.tile <=> _2.tile         :
           _1.red_dora != _2.red_dora ? _1.red_dora <=> _2.red_dora : _1.toumei <=> _2.toumei;
}

static auto operator==(const Tile& _1, const Tile& _2)
{
    return _1.tile == _2.tile && _1.red_dora == _2.red_dora && _1.toumei == _2.toumei;
}

namespace lgtbot {

namespace game_util {

namespace mahjong {

static constexpr const uint32_t k_hand_tile_num = 13;
static constexpr const uint32_t k_tile_type_num = 34;

struct PlayerDesc
{
    std::string name_;
    std::string big_avatar_;
    std::string small_avatar_;
    Wind wind_ = Wind::East;
    int32_t base_point_ = 25000;
};

struct TileIdent
{
    operator Tile() const
    {
        return Tile{
            .tile = static_cast<BaseTile>((digit_ == '0' ? 4 : digit_ - '1') + 9 * (std::string("mspzMSPZ").find_first_of(color_) % 4)),
            .red_dora = (digit_ == '0'),
            .toumei = static_cast<bool>(std::isupper(color_)),
        };
    }

    char digit_;
    char color_;
};

static std::string wind2str(const Wind wind)
{
    switch (wind) {
        case Wind::East: return "东";
        case Wind::West: return "西";
        case Wind::South: return "南";
        case Wind::North: return "北";
        default: assert(false);
    }
    return "";
}

using TileSet = std::multiset<Tile, std::less<Tile>>;

// input: 1t3st45m
// output: 1s3S4M5m
static std::string DecodeTilesString(const std::string_view str, std::string& errstr)
{
    std::string output;
    std::string digits;
    if (str.size() > k_hand_tile_num * 2) {
        errstr = "麻将字符串过长，请小于 26 个字符";
        return {};
    }
    bool has_number_tile_num = false; // 0,8,9 can only appear in m,s,p
    bool read_toumei = false;
    for (const char c : str) {
        if (read_toumei && !std::isdigit(c)) {
            errstr = "\'t\' 后面需要跟数字";
            return {};
        }
        if (c == 't') {
            read_toumei = true;
            continue;
        }
        if (std::isdigit(c)) {
            if (read_toumei) {
                digits += 't';
                read_toumei = false;
            }
            digits += c;
            has_number_tile_num |= (c == '0' || c == '8' || c == '9');
            continue;
        }
        if (c != 'm' && c != 's' && c != 'p' && c != 'z') {
            errstr = "非法的花色 \'";
            errstr += c;
            errstr += "\'";
            return {};
        }
        if (has_number_tile_num && c == 'z') {
            errstr = "字牌（花色 \'z\'）的数字只能为 1~7";
            return {};
        }
        if (digits.empty()) {
            errstr = "花色 \'";
            errstr += c;
            errstr += "\' 之前必须有数字";
            return {};
        }
        for (const char digit_c : digits) {
            if (digit_c == 't') {
                read_toumei = true;
            } else {
                output += digit_c;
                output += read_toumei ? (c - 'a' + 'A') : c;
                read_toumei = false;
            }
        }
        digits.clear();
        has_number_tile_num = false;
    }
    if (!digits.empty()) {
        errstr = "末尾的数字 \"";
        errstr += digits;
        errstr += "\' 没有花色";
        return {};
    }
    return output;
}

static void GenerateTilesFromDecodedString(const std::string_view decoded_str, std::array<Tile, k_tile_type_num * 4>& tiles) {
    for (uint32_t i = 0; i < decoded_str.size() && i < k_tile_type_num * 4 * 2; i += 2) {
        tiles[i / 2] = TileIdent(decoded_str[i], decoded_str[i + 1]);
    }
}

enum class GetTileMode { EXACT, FUZZY, PREFER_RED_DORA };
static TileSet GetTilesFrom(TileSet& src, const std::string_view str, std::string& errstr, const GetTileMode mode = GetTileMode::FUZZY)
{
    TileSet tiles;
    const auto decoded_str = DecodeTilesString(str, errstr);
    assert(decoded_str.size() % 2 == 0);
    const auto find_tile = [&src](const char num, const char suit)
        {
            return src.find(TileIdent(num, suit));
        };
    const auto find_toumei_tile = [&src](const char num, const char suit)
        {
            return std::islower(suit) ? src.find(TileIdent(num, std::toupper(suit))) : src.end();
        };
    const auto find_red_tile = [&src](const char num, const char suit)
        {
            return num == '5' ? src.find(TileIdent('0', suit)) : src.end();
        };
    const auto find_toumei_red_tile = [&src](const char num, const char suit)
        {
            return std::islower(suit) && num == '5' ? src.find(TileIdent('0', std::toupper(suit))) : src.end();
        };
    const auto insert_if_found = [&](const auto it)
        {
            if (it == src.end()) {
                return false;
            }
            tiles.emplace(*it);
            src.erase(it);
            return true;
        };
    const auto insert_if_found_multi = [&](const char num, const char suit, const auto ...find_fn)
        {
            return (insert_if_found(find_fn(num, suit)) || ...);
        };
    for (uint32_t i = 0; i < decoded_str.size(); i += 2) {
        if ((mode == GetTileMode::PREFER_RED_DORA && insert_if_found_multi(decoded_str[i], decoded_str[i + 1],
                        find_red_tile, find_toumei_red_tile, find_tile, find_toumei_tile)) ||
            (mode == GetTileMode::FUZZY && insert_if_found_multi(decoded_str[i], decoded_str[i + 1],
                        find_tile, find_toumei_tile, find_red_tile, find_toumei_red_tile)) ||
            (mode == GetTileMode::EXACT && insert_if_found_multi(decoded_str[i], decoded_str[i + 1],
                        find_tile))) {
            continue;
        }
        errstr = "没有足够的 \"";
        if (std::isupper(decoded_str[i + 1])) {
            errstr += {'t', decoded_str[i], static_cast<char>(std::tolower(decoded_str[i + 1]))};
        } else {
            errstr += {decoded_str[i], decoded_str[i + 1]};
        }
        errstr += "\" 可以取出";
        src.insert(tiles.begin(), tiles.end());
        return {};
    }
    return tiles;
}

enum class TileStyle { HAND = '0', FORWARD = '1', LEFT = '2', SMALL_HAND = '3' };

static std::string Image(const std::string& image_path, const Tile& tile, const TileStyle style)
{
    return "<img src=\"file:///" + image_path + "/" + static_cast<char>(style) + "_" + tile.to_simple_string() + ".png\"/>";
}
static std::string BackImage(const std::string& image_path, const TileStyle style)
{
    return "<img src=\"file:///" + image_path + "/" + static_cast<char>(style) + "_back.png\"/>";
}

static std::string TitleHtml(const std::string& table_name, const int round)
{
    return "<center><font size=\"7\">" + table_name + "</font></center> \n\n " +
        "<center><font size=\"6\"> 第 " + std::to_string(round) + " 巡 </font></center>";
}

static std::string PlayerNameHtml(const PlayerDesc& player_desc, const int32_t point)
{
    std::string s = "## " + player_desc.big_avatar_ + "&nbsp;&nbsp; " + wind2str(player_desc.wind_) + "家：" + player_desc.name_ +
        "（" + std::to_string(player_desc.base_point_);
    if (point > 0) {
        s += HTML_COLOR_FONT_HEADER(green) " + " + std::to_string(point) + HTML_FONT_TAIL;
    } else if (point < 0) {
        s += HTML_COLOR_FONT_HEADER(red) " - " + std::to_string(-point) + HTML_FONT_TAIL;
    }
    s += "）";
    return s;
}

static std::string DoraHtml(const std::string& image_path, const std::span<const std::pair<Tile, Tile>>& doras, const bool with_inner_dora)
{
    std::string s = "<center>\n\n";
    const auto append_empty_doras = [&]()
        {
            constexpr int32_t k_max_dora_num = 5;
            assert(doras.size() < k_max_dora_num);
            for (auto i = doras.size(); i < k_max_dora_num; ++i) {
                s += "<img src=\"file:///" + image_path + "/empty_dora.png\"/>";
            }
        };
    for (const auto& [dora, _] : doras) {
        s += Image(image_path, dora, TileStyle::FORWARD);
    }
    append_empty_doras();
    if (with_inner_dora) {
        s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
        for (const auto& [_, inner_dora] : doras) {
            s += Image(image_path, inner_dora, TileStyle::FORWARD);
        }
        append_empty_doras();
    }
    s += "\n\n</center>";
    return s;
}

static std::string HandHtml(const std::string& image_path, const TileSet& hand, const TileStyle style, const std::optional<Tile>& tsumo = std::nullopt)
{
    std::string str = "<div nowrap>";
    for (const auto& tile : hand) {
        str += Image(image_path, tile, style);
    }
    if (tsumo.has_value()) {
        str += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
        str += Image(image_path, *tsumo, style);
    }
    return str + "</div>";
}

static std::string HandHtmlBack(const std::string& image_path, const TileSet& hand)
{
    std::string s = "<div nowrap>";
    uint32_t i = 0;
    for (const auto& tile : hand) {
        if (tile.toumei) {
            s += Image(image_path, tile, TileStyle::SMALL_HAND);
            ++i;
        }
    }
    for (; i < hand.size(); ++i) {
        s += BackImage(image_path, TileStyle::SMALL_HAND);
    }
    return s + "</div>";
}

static std::string LoserHtml(const PlayerDesc& loser_desc) {
    return "<font size=\"4\"> " HTML_COLOR_FONT_HEADER(blue) " **放铳者：" + loser_desc.small_avatar_ +
        "&nbsp;&nbsp; " + loser_desc.name_ + "** " HTML_FONT_TAIL " </font>";
}

static std::string YakusHtml(const std::string& image_path, const Tile& tile, const std::string& score_info,
        const std::vector<Yaku>& yakus, const std::vector<std::string>& texts = {}, const std::vector<Yaku>& except_yakus = {})
{
    std::string str = "<style> img { vertical-align: text-bottom; } </style>\n\n";
    html::Table title_table(1, 1); // markdown format image cannot be in <center> block in same line
    title_table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
    //title_table.SetRowStyle(" style=\"min-width:50px; vertical-align: middle;\" ");
    title_table.Get(0, 0).SetContent(Image(image_path, tile, TileStyle::FORWARD) + "<font size=\"5\"> <b> &nbsp;&nbsp; " + score_info + " </b> </font>");
    for (const auto& text : texts) {
        title_table.AppendRow();
        title_table.Get(title_table.Row() - 1, 0).SetContent(text);
    }
    str += "<br />\n\n" + title_table.ToString();
    std::map<Yaku, uint32_t> yaku_counts;
    for (const auto& yaku : yakus) {
        ++(yaku_counts.emplace(yaku, 0).first->second);
    }
    html::Table table(0, 4);
    table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" width=\"300\"");
    table.SetRowStyle(" align=\"left\" ");
    bool newline = false;
    for (const auto& [yaku, count] : yaku_counts) {
        if (std::ranges::find(except_yakus, yaku) != except_yakus.end()) {
            continue;
        }
        if (newline = !newline) {
            table.AppendRow();
        }
        const uint32_t offset = newline ? 0 : 2;
        table.Get(table.Row() - 1, offset + 0).SetContent(" **" + yaku_to_string(yaku) + "** ");
        table.Get(table.Row() - 1, offset + 1).SetContent(" **" + (
                yaku > Yaku::None && yaku < Yaku::一番 ? std::to_string(1) + " 番" :
                yaku > Yaku::一番 && yaku < Yaku::二番 ? std::to_string(2) + " 番" :
                yaku > Yaku::二番 && yaku < Yaku::三番 ? std::to_string(3) + " 番" :
                yaku > Yaku::三番 && yaku < Yaku::五番 ? std::to_string(5) + " 番" :
                yaku > Yaku::五番 && yaku < Yaku::六番 ? std::to_string(6) + " 番" : "") +
            (count > 1 ? " × " + std::to_string(count) : "") + "** ");
    }
    str += table.ToString();
    return str;
}

struct TilesOption
{
    bool with_red_dora_ = false;
    uint32_t with_toumei_ = 0;
    std::string seed_;
};

inline void ShuffleTiles(const TilesOption& option, std::array<Tile, k_tile_type_num * 4>& tiles) {
    for (uint32_t i = 0; i < k_tile_type_num * 4; ++i) {
        tiles[i].tile = static_cast<BaseTile>(i % k_tile_type_num);
        tiles[i].red_dora = false;
        tiles[i].toumei = i < option.with_toumei_ * k_tile_type_num;
    }
    if (option.with_red_dora_) {
        tiles[4].red_dora = true;
        tiles[13].red_dora = true;
        tiles[22].red_dora = true;
    }

    // shuffle tiles
    std::variant<std::random_device, std::seed_seq> rd;
    std::mt19937 g([&]
        {
            if (option.seed_.empty()) {
                auto& real_rd = rd.emplace<std::random_device>();
                return std::mt19937(real_rd());
            } else {
                auto& real_rd = rd.emplace<std::seed_seq>(option.seed_.begin(), option.seed_.end());
                return std::mt19937(real_rd);
            }
        }());
    std::shuffle(tiles.begin(), tiles.end(), g);
}

} // namespace mahjong

} // namespace game_util

} // namespace lgtbot
