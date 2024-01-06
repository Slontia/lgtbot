// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <map>
#include <set>
#include <string>

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

static constexpr const uint32_t k_hand_tile_num_ = 13;

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
    if (str.size() > k_hand_tile_num_ * 2) {
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

static TileSet GetTilesFrom(TileSet& src, const std::string_view str, std::string& errstr)
{
    TileSet tiles;
    const auto insert = [&](const TileSet::iterator it)
        {
            tiles.emplace(*it);
            src.erase(it);
        };
    const auto decoded_str = DecodeTilesString(str, errstr);
    assert(decoded_str.size() % 2 == 0);
    for (uint32_t i = 0; i < decoded_str.size(); i += 2) {
        if (const auto it = src.find(TileIdent(decoded_str[i], decoded_str[i + 1])); it != src.end()) {
            // we found it
            insert(it);
            continue;
        }
        if (std::islower(decoded_str[i + 1])) {
            const auto it = src.find(TileIdent(decoded_str[i], std::toupper(decoded_str[i + 1])));
            if (it != src.end()) {
                // we found a toumei tile instead
                insert(it);
                continue;
            }
        }
        if (decoded_str[i] == '5') {
            const auto it = src.find(TileIdent('0', std::toupper(decoded_str[i + 1])));
            if (it != src.end()) {
                // we found a red dora tile instead
                insert(it);
                continue;
            }
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
    return "![](file:///" + image_path + "/" + static_cast<char>(style) + "_" + tile.to_simple_string() + ".png)";
}
static std::string BackImage(const std::string& image_path, const TileStyle style)
{
    return "![](file:///" + image_path + "/" + static_cast<char>(style) + "_back.png)";
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

static std::string DoraHtml(const std::string& image_path, const bool show_inner_dora, const std::vector<std::pair<Tile, Tile>>& doras, const bool with_inner_dora)
{
    const std::string head_str = "<center>\n\n" + BackImage(image_path, TileStyle::FORWARD) + BackImage(image_path, TileStyle::FORWARD);
    std::string outer_str;
    std::string inner_str;
    for (const auto& [dora, inner_dora] : doras) {
        outer_str += Image(image_path, dora, TileStyle::FORWARD);
        inner_str += show_inner_dora ? Image(image_path, inner_dora, TileStyle::FORWARD) : BackImage(image_path, TileStyle::FORWARD);
    }
    std::string tail_str;
    for (auto i = doras.size(); i < 5; ++i) {
        tail_str += BackImage(image_path, TileStyle::FORWARD);
    }
    tail_str += "\n\n</center>";
    std::string final_str = head_str + outer_str + tail_str;
    if (with_inner_dora) {
        final_str += "\n\n" + head_str + inner_str + tail_str;
    }
    return final_str;
}

static std::string HandHtml(const std::string& image_path, const TileSet& hand, const TileStyle style)
{
    std::string str;
    html::Table table(1, k_hand_tile_num_);
    table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
    uint32_t i = 0;
    for (const auto& tile : hand) {
        table.Get(0, i).SetContent(Image(image_path, tile, style));
        ++i;
    }
    str += table.ToString();
    return str;
}

static std::string HandHtmlBack(const std::string& image_path, const TileSet& hand)
{
    std::string str;
    html::Table table(1, k_hand_tile_num_);
    table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
    uint32_t i = 0;
    for (const auto& tile : hand) {
        if (tile.toumei) {
            table.Get(0, i).SetContent(Image(image_path, tile, TileStyle::SMALL_HAND));
            ++i;
        }
    }
    for (; i < k_hand_tile_num_; ++i) {
        table.Get(0, i).SetContent(BackImage(image_path, TileStyle::SMALL_HAND));
    }
    str += table.ToString();
    return str;
}

static std::string LoserHtml(const PlayerDesc& loser_desc) {
    return "<font size=\"4\"> " HTML_COLOR_FONT_HEADER(blue) " **放铳者：" + loser_desc.small_avatar_ +
        "&nbsp;&nbsp; " + loser_desc.name_ + "** " HTML_FONT_TAIL " </font>";
}

static std::string YakusHtml(const std::string& image_path, const Tile& tile, const CounterResult& counter, const std::vector<std::string>& texts = {}, const std::vector<Yaku>& except_yakus = {})
{
    std::string str = "<style> img { vertical-align: text-bottom; } </style>\n\n";
    html::Table title_table(1, 1); // markdown format image cannot be in <center> block in same line
    title_table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
    //title_table.SetRowStyle(" style=\"min-width:50px; vertical-align: middle;\" ");
    title_table.Get(0, 0).SetContent(Image(image_path, tile, TileStyle::FORWARD) + "<font size=\"5\"> <b> &nbsp;&nbsp; " + (
            counter.score1 == 32000 * 4  ? "四倍役满" :
            counter.score1 == 32000 * 3  ? "三倍役满" :
            counter.score1 == 32000 * 2  ? "两倍役满" :
            counter.score1 == 32000      ? "役满" :
            counter.score1 == 24000      ? "三倍满" :
            counter.score1 == 16000      ? "倍满" :
            counter.score1 == 12000      ? "跳满" :
            counter.score1 == 8000       ? "满贯" : "") + " " +
            (counter.fan > 0 ? std::to_string(counter.fu) + " 符 " + std::to_string(counter.fan) + " 番" : "") +
        " " + std::to_string(counter.score1) + " 点 </b> </font>");
    for (const auto& text : texts) {
        title_table.AppendRow();
        title_table.Get(title_table.Row() - 1, 0).SetContent(text);
    }
    str += "<br />\n\n" + title_table.ToString();
    std::map<Yaku, uint32_t> yaku_counts;
    for (const auto& yaku : counter.yakus) {
        ++(yaku_counts.emplace(yaku, 0).first->second);
    }
    html::Table table(0, 4);
    table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" width=\"300\"");
    table.SetRowStyle(" align=\"left\" ");
    bool newline = false;
    for (const auto& [yaku, count] : yaku_counts) {
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

} // namespace mahjong

} // namespace game_util

} // namespace lgtbot
