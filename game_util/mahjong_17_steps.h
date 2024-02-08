// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "game_util/mahjong_util.h"

#include <ranges>
#include <algorithm>

#include "Mahjong/Rule.h"

#ifdef TEST_BOT
#define private public
#endif

namespace lgtbot {

namespace game_util {

namespace mahjong {

enum class ErrCode { OK, NOT_HAS, EXCEED_SIZE };

struct Mahjong17StepsOption
{
    TilesOption tile_option_;
    std::string name_;
    bool with_inner_dora_ = false;
    uint32_t dora_num_ = 0;
    uint32_t ron_required_point_ = 8000;
    std::string image_path_;
    std::vector<PlayerDesc> player_descs_;
};

static constexpr const uint32_t k_player_num_ = 4;

struct RonInfo
{
    uint64_t loser_ = 0;
    Tile tile_;
    CounterResult counter_;
};

class Mahjong17Steps
{
  private:
    static constexpr const uint32_t k_yama_tile_num_ = 34;
    static constexpr const uint32_t k_max_round_ = 17;

    /*
    struct TileWrapper
    {
        TileWrapper(Tile* tile) : tile_(tile) {}

        friend auto operator<=>(const TileWrapper& _1, const TileWrapper& _2)
        {
            return _1.tile_->tile == _2.tile_->tile ? _1.tile_->red_dora <=> _2.tile_->red_dora :
                                                      _1.tile_->tile <=> _2.tile_->tile;
        }

        friend auto operator==(const TileWrapper& _1, const TileWrapper& _2) { return _1.tile_->tile == _2.tile_->tile && _1.tile_->red_dora == _2.tile_->red_dora; }

        friend auto operator<=>(const TileWrapper& t, const TileIdent& ident)
        {
            Tile tile = ident;
            return t <=> TileWrapper(&tile);
        }

        Tile* tile_;
    };
    */

    struct Player
    {
        int32_t point_ = 0;

        // the sum of such tiles must be 34
        TileSet yama_;
        TileSet hand_;
        std::optional<Tile> kiri_;
        std::vector<Tile> river_;

        bool furutin_ = 0;
        std::map<BaseTile, CounterResult> listen_tiles_;
        std::vector<RonInfo> ron_infos_; // if not empty, means furutin
    };

  public:
    Mahjong17Steps(const Mahjong17StepsOption& option = Mahjong17StepsOption())
        : option_(option)
        , round_(0)
        , is_flow_(false)
    {
        // init tiles
        std::array<Tile, k_yama_tile_num_ * k_player_num_> tiles;
        ShuffleTiles(option_.tile_option_, tiles);

        // init doras
        for (uint32_t i = 0; i < option_.dora_num_; ++i) {
            doras_.emplace_back(tiles[N_TILES - 1 - i * 2], tiles[N_TILES - 1 - i * 2 - 1]);
        }

        // distrib tiles
        for (uint32_t pid = 0; pid < k_player_num_; ++pid) {
            for (uint32_t tile_idx = 0; tile_idx < k_yama_tile_num_; ++tile_idx) {
                players_[pid].yama_.emplace(tiles[pid * k_yama_tile_num_ + tile_idx]);
            }
        }
    }

    // Prepare state
    bool AddToHand(const uint64_t pid, const std::string_view str)
    {
        auto& hand = players_[pid].hand_;
        auto& yama = players_[pid].yama_;
        const auto tiles = GetTilesFrom(yama, str, errstr_);
        if (tiles.empty()) { // get failed
            return false;
        }
        if (hand.size() + tiles.size() > k_hand_tile_num) {
            errstr_ = "手牌数将大于 13 枚，您当前持有手牌 " + std::to_string(hand.size()) + " 枚，本次操作将添加 " + std::to_string(tiles.size()) + " 枚";
            yama.insert(tiles.begin(), tiles.end()); // rollback
            return false;
        }
        hand.insert(tiles.begin(), tiles.end());
        if (hand.size() == k_hand_tile_num) {
            players_[pid].listen_tiles_ = GetListenInfo_(pid);
        }
        return true;
    }

    // Prepare state
    void AddToHand(const uint64_t pid)
    {
        auto& hand = players_[pid].hand_;
        auto& yama = players_[pid].yama_;
        for (uint32_t i = hand.size(); i < k_hand_tile_num; ++i) {
            hand.emplace(*yama.begin());
            yama.erase(yama.begin());
        }
        players_[pid].listen_tiles_ = GetListenInfo_(pid);
    }

    // Prepare state
    bool RemoveFromHand(const uint64_t pid, const std::string_view str)
    {
        auto& hand = players_[pid].hand_;
        auto& yama = players_[pid].yama_;
        const auto tiles = GetTilesFrom(hand, str, errstr_);
        if (tiles.empty()) { // get failed, errstr has already be set
            return false;
        }
        yama.insert(tiles.begin(), tiles.end());
        players_[pid].listen_tiles_.clear();
        return true;
    }

    // Prepare state
    bool CheckHandValid(const uint64_t pid) const { return players_[pid].hand_.size() == k_hand_tile_num; }

    // Step state
    bool Kiri(const uint64_t pid, const std::string_view str)
    {
        Player& player = players_[pid];
        if (player.kiri_.has_value()) {
            errstr_ = "您已经切过牌了，切的是 " + player.kiri_->to_simple_string();
            return false;
        }
        if (!(str.size() == 2 || (str.size() == 3 && str[0] == 't'))) {
            errstr_ = "您只能切一张牌";
            return false;
        }
        const auto tiles = GetTilesFrom(player.yama_, str, errstr_);
        if (tiles.empty()) {
            return false;
        }
        assert(tiles.size() == 1);
        player.kiri_.emplace(*tiles.begin());
        return true;
    }

    // Step state
    enum class GameState { CONTINUE, HAS_RON, FLOW };
    GameState RoundOver()
    {
        ++round_;
        for (uint64_t pid = 0; pid < option_.player_descs_.size(); ++pid) {
            PrepareForRoundOver_(pid);
        }
        // check ron
        bool has_ron = false;
        for (uint64_t this_pid = 0; this_pid < option_.player_descs_.size(); ++this_pid) {
            Player& this_player = players_[this_pid];
            if (this_player.furutin_) {
                continue;
            }
            MakeRonInfoForRounOver_(this_pid);
            if (this_player.ron_infos_.empty()) {
                continue;
            }
            has_ron |= UpdatePointForRoundOver_(this_pid);
        }
        for (uint64_t pid = 0; pid < option_.player_descs_.size(); ++pid) {
            players_[pid].kiri_.reset();
        }
        return has_ron                ? GameState::HAS_RON                 :
               round_ == k_max_round_ ? (is_flow_ = true, GameState::FLOW) : GameState::CONTINUE;
    }

    std::set<Yaku> GetYakumanYakus(const uint64_t pid) const
    {
        std::set<Yaku> result;
        for (const RonInfo& ron_info : players_[pid].ron_infos_) {
            bool has_yakuman_yaku = false;
            for (const Yaku yaku : ron_info.counter_.yakus) {
                if (yaku > Yaku::满贯) {
                    has_yakuman_yaku = true;
                    result.emplace(yaku);
                }
            }
            if (!has_yakuman_yaku && ron_info.counter_.fan >= 13) {
                result.emplace(Yaku::役满); // indicate accumulate yakuman
            }
        }
        return result;
    }

    int32_t PointChange(const uint64_t pid) const { return players_[pid].point_; }

    // Step state
    std::string PrepareHtml(const uint64_t pid) const
    {
        const Player& player = players_[pid];
        std::string s = TitleHtml_() + "\n\n" + PlayerNameHtml_(pid) + "\n\n" + DoraHtml_(false) + "\n\n" + HandHtmlPrepare_(pid) + "\n\n";
        if (player.hand_.size() != k_hand_tile_num) {
            // no nothing
        } else if (player.listen_tiles_.empty()) {
            s += "<center>\n\n" HTML_COLOR_FONT_HEADER(red) " **未构成听牌牌型** " HTML_FONT_TAIL "\n\n</center>";
        } else {
            s += "<br />\n\n<center>听牌种类及得分（此处不计一发、河底、里宝牌等非确定役）</center>\n\n";
            for (auto [basetile, counter] : player.listen_tiles_) {
                counter = UpdateCounterResult_(counter, false, RonOccu::NORMAL, false);
                if (counter.score1 < option_.ron_required_point_) {
                    s += YakusHtml_(Tile{basetile, 0}, counter, std::vector<std::string>{
                                HTML_COLOR_FONT_HEADER(red) " **（警告：未达到起和点 " +
                                std::to_string(option_.ron_required_point_) + "，无法和牌）** " HTML_FONT_TAIL
                            }) + "\n\n";
                } else {
                    s += YakusHtml_(Tile{basetile, 0}, counter) + "\n\n";
                }
            }
            s += "<br />\n\n";
        }
        s += YamaHtml_(pid);
        return s + StyleHtml_();
    }

    std::string PrepareString(const uint64_t pid) const
    {
        const Player& player = players_[pid];
        std::string s = TitleString_() + "\n" + DoraString_() + "\n\n" + PlayerNameString_(pid) + "\n" + HandStringPrepare_(pid);
        if (player.hand_.size() != k_hand_tile_num) {
            // no nothing
        } else if (player.listen_tiles_.empty()) {
            s += "\n   - 未构成听牌牌型";
        } else {
            for (auto [basetile, counter] : player.listen_tiles_) {
                counter = UpdateCounterResult_(counter, false, RonOccu::NORMAL, false);
                s += "\n   - 听 ";
                s += Tile{basetile, 0}.to_simple_string();
                s += "：";
                s += std::to_string(counter.score1);
                s += " 点";
                if (counter.score1 < option_.ron_required_point_) {
                    s += "（无法和牌）";
                }
            }
        }
        s += "\n" + YamaString_(pid);
        return s;
    }

    // Step state
    std::string KiriHtml(const uint64_t pid)
    {
        const Player& player = players_[pid];
        const bool is_ron = !players_[pid].ron_infos_.empty();
        std::string s = TitleHtml_() + "\n\n" + PlayerNameHtml_(pid) + "\n\n";
        if (is_ron) {
            if (player.furutin_) {
                s += "<center> <font size=\"6\">\n\n " HTML_COLOR_FONT_HEADER(red) " **振&nbsp;&nbsp;听** "
                    HTML_FONT_TAIL "\n\n</font> </center>\n\n";
            } else {
                s += "<center> <font size=\"6\">\n\n " HTML_COLOR_FONT_HEADER(blue) " **和&nbsp;&nbsp;了** "
                    HTML_FONT_TAIL "\n\n</font> </center>\n\n";
            }
        }
        s += DoraHtml_(is_ron && !player.furutin_) + "\n\n" + HandHtmlAll_(pid, TileStyle::HAND) + "\n\n";
        if (is_ron) {
            s += RonInfoHtml_(pid) + "\n\n<br />\n\n";
        } else if (player.furutin_) {
            s += "<center>\n\n" HTML_COLOR_FONT_HEADER(red) " **振听中，无法荣和** " HTML_FONT_TAIL "\n\n</center>";
        } else if (player.listen_tiles_.empty()) {
            s += "<center>\n\n" HTML_COLOR_FONT_HEADER(red) " **未构成听牌牌型** " HTML_FONT_TAIL "\n\n</center>";
        }
        s += YamaHtml_(pid) + "\n\n";
        s += "<br />\n\n" + RiverHtml_(pid) + "\n\n";
        for (uint32_t other_pid = 0; other_pid < option_.player_descs_.size(); ++other_pid) {
            if (pid != other_pid) {
                s += PlayerHtml_(other_pid, false) + "\n\n";
            }
        }

        return s + StyleHtml_();
    }

    std::string KiriString(const uint64_t pid)
    {
        std::string s = TitleString_() + "\n" + DoraString_() + "\n\n" + PlayerNameString_(pid) + "\n" + HandStringAll_(pid) +
            "\n" + YamaString_(pid) + "\n" + RiverString_(pid);
        for (uint32_t other_pid = 0; other_pid < option_.player_descs_.size(); ++other_pid) {
            if (pid != other_pid) {
                s +=  "\n\n" + PlayerString_(other_pid);
            }
        }
        return s;
    }

    // Step state
    std::string PublicHtml() const
    {
        std::string s = TitleHtml_() + "\n\n" + DoraHtml_(false) + "\n\n";
        for (uint64_t pid = 0; pid < option_.player_descs_.size(); ++pid) {
            s += PlayerHtml_(pid, false) + "\n\n";
        }
        return s + StyleHtml_();
    }

    // Step state
    std::string PublicString() const
    {
        std::string s = TitleString_() + "\n" + DoraString_();
        for (uint64_t pid = 0; pid < option_.player_descs_.size(); ++pid) {
            s += "\n\n" + PlayerString_(pid);
        }
        return s;
    }

    // Step state
    std::string SpecHtml() const
    {
        std::string s = TitleHtml_() + "\n\n" + DoraHtml_(false) + "\n\n";
        for (uint64_t pid = 0; pid < option_.player_descs_.size(); ++pid) {
            s += PlayerHtml_(pid, true) + "\n\n";
        }
        return s + StyleHtml_();
    }

    // Any time
    const std::string& ErrorStr() const { return errstr_; }

  private:
    static const char* StyleHtml_()
    {
        return "\n\n<style>html,body{background:#c3e4f5;}</style>\n\n";
    }

    void PrepareForRoundOver_(const uint64_t pid)
    {
        Player& player = players_[pid];
        if (!player.kiri_.has_value()) {
            // default kiri the first tile in yama
            player.kiri_.emplace(*player.yama_.begin());
            player.yama_.erase(player.yama_.begin());
        }
        player.furutin_ |= player.listen_tiles_.contains(player.kiri_->tile);
        player.ron_infos_.clear();
        player.river_.emplace_back(*player.kiri_);
    }

    // fill ron_infos_ for the player
    void MakeRonInfoForRounOver_(const uint64_t this_pid)
    {
        Player& this_player = players_[this_pid];
        const RonOccu ron_occu = round_ == 1            ? RonOccu::ROUND_1  :
                                 round_ == k_max_round_ ? RonOccu::ROUND_17 : RonOccu::NORMAL;
        for (uint64_t other_pid = 0; other_pid < option_.player_descs_.size(); ++other_pid) {
            if (this_pid == other_pid) {
                continue;
            }
            const Player& other_player = players_[other_pid];
            const auto it = this_player.listen_tiles_.find(other_player.kiri_->tile);
            if (it != this_player.listen_tiles_.end()) {
                this_player.ron_infos_.emplace_back(other_pid, *other_player.kiri_,
                        UpdateCounterResult_(it->second, option_.with_inner_dora_, ron_occu, other_player.kiri_->red_dora));
            }
        }
    }

    bool UpdatePointForRoundOver_(const uint64_t this_pid)
    {
        Player& this_player = players_[this_pid];
        const uint32_t max_ron_point = std::max_element(this_player.ron_infos_.begin(), this_player.ron_infos_.end(),
                [](const auto& _1, const auto& _2) { return _1.counter_.score1 < _2.counter_.score1; })->counter_.score1;
        if (max_ron_point < option_.ron_required_point_) {
            // ron not achieve required point
            this_player.furutin_ = true;
            return false;
        }
        const uint32_t max_ron_count = std::ranges::count_if(this_player.ron_infos_,
                [&](const auto& info) { return info.counter_.score1 == max_ron_point; });
        for (auto& info : this_player.ron_infos_) {
            if (info.counter_.score1 == max_ron_point) {
                // sharing the points to multiple players who lose the game
                info.counter_.score2 = ((max_ron_point / 100 + max_ron_count - 1) / max_ron_count) * 100;
                this_player.point_ += info.counter_.score2;
                players_[info.loser_].point_ -= info.counter_.score2;
            } else {
                info.counter_.score2 = 0;
            }
        }
        return true;
    }

    std::string PlayerHtml_(const uint64_t pid, const bool show_all_info) const
    {
        std::string s;
        s += PlayerNameHtml_(pid) + "\n\n";
        if (!players_[pid].ron_infos_.empty() && !players_[pid].furutin_) {
            s += "<center> <font size=\"6\">\n\n " HTML_COLOR_FONT_HEADER(blue) " **和&nbsp;&nbsp;了** "
                HTML_FONT_TAIL "\n\n</font> </center>\n\n";
            s += DoraHtml_(true) + "\n\n" + HandHtmlAll_(pid, TileStyle::FORWARD) + "\n\n" + RonInfoHtml_(pid) + "\n\n<br />\n\n";
        } else if (is_flow_) {
            s += HandHtmlAll_(pid, TileStyle::FORWARD) + "\n\n<br />\n\n";
        } else if (show_all_info) {
            s += HandHtmlAll_(pid, TileStyle::SMALL_HAND) + "\n\n<br />\n\n";
        } else {
            s += HandHtmlBack_(pid) + "\n\n<br />\n\n";
        }
        s += RiverHtml_(pid);
        if (show_all_info) {
            s += "\n\n" + YamaHtmlForSpec_(pid);
        }
        return s;
    }

    std::string PlayerString_(const uint64_t pid) const
    {
        return PlayerNameString_(pid) + "\n" + HandStringBack_(pid) + "\n" + RiverString_(pid);
    }

    std::string TitleHtml_() const { return TitleHtml(option_.name_, round_); }

    std::string TitleString_() const
    {
        return option_.name_ + " - 第 " + std::to_string(round_) + " 巡";
    }

    std::string RiverHtml_(const uint64_t pid) const
    {
        const auto& river = players_[pid].river_;
        if (river.empty()) {
            return "";
        }
        html::Table table(1, 1);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        table.SetRowStyle(" align=\"left\" ");
        std::string s;
        s += Image_(river[0], TileStyle::LEFT);
        for (uint32_t i = 1; i < river.size(); ++i) {
            s += Image_(river[i], TileStyle::FORWARD);
            if (i == 5 || i == 11) {
                s += "&nbsp;&nbsp;";
            }
        }
        table.Get(0, 0).SetContent(s);
        return table.ToString();
    }

    std::string RiverString_(const uint64_t pid) const
    {
        std::string s = "- 舍牌：";
        const auto& river = players_[pid].river_;
        for (uint32_t i = 0; i < river.size(); ++i) {
            s += river[i].to_simple_string() + " ";
            if (i == 5 || i == 11) {
                s += "\n";
            }
        }
        return s;
    }

    std::string RonInfoHtml_(const uint64_t pid) const
    {
        const Player& player = players_[pid];
        std::string s;
        for (const auto& info : player.ron_infos_) {
            std::vector<std::string> texts = {LoserHtml(option_.player_descs_[info.loser_])};
            if (info.counter_.score1 < option_.ron_required_point_) {
                texts.emplace_back(HTML_COLOR_FONT_HEADER(red) " **点数没有达到起和点 " +
                    std::to_string(option_.ron_required_point_) + "，不计和牌** " HTML_FONT_TAIL);
            } else if (info.counter_.score2 == 0) {
                texts.emplace_back(HTML_COLOR_FONT_HEADER(red) " **荣和低目，不计和牌** " HTML_FONT_TAIL);
            } else if (info.counter_.score1 != info.counter_.score2) {
                texts.emplace_back(HTML_COLOR_FONT_HEADER(green) " **因多人放铳，放铳者仅需实际分担 " +
                        std::to_string(info.counter_.score2) + " 点** " HTML_FONT_TAIL);
            }
            s += YakusHtml_(info.tile_, info.counter_, std::move(texts)) + "\n\n";
        }
        return s;
    }

    std::string PlayerNameHtml_(const uint64_t pid) const
    {
        return PlayerNameHtml(option_.player_descs_[pid], players_[pid].point_);
    }

    std::string PlayerNameString_(const uint64_t pid) const
    {
        return "## " + wind2str(option_.player_descs_[pid].wind_) + "家：" + option_.player_descs_[pid].name_ +
            "（" + std::to_string(option_.player_descs_[pid].base_point_) + "）";
    }

    std::string DoraHtml_(const bool show_inner_dora) const
    {
        return DoraHtml(option_.image_path_, show_inner_dora, doras_, option_.with_inner_dora_);
    }

    std::string DoraString_() const
    {
        std::string s = "宝牌指示牌：";
        for (const auto& [dora, inner_dora] : doras_) {
            s += dora.to_simple_string();
            s += ' ';
        }
        return s;
    }

    std::string HandHtmlPrepare_(const uint64_t pid) const
    {
        const Player& player = players_[pid];
        std::string str = "<center>手牌 (" + std::to_string(player.hand_.size()) + " / 13)</center>\n\n";
        html::Table table(2, k_hand_tile_num);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        uint32_t i = 0;
        for (const auto& tile : player.hand_) {
            table.Get(0, i).SetContent(tile.to_simple_string());
            table.Get(1, i).SetContent(Image_(tile, TileStyle::HAND));
            ++i;
        }
        for (; i < k_hand_tile_num; ++i) {
            table.Get(1, i).SetContent("<p style=\"width:36px; height:60px;\">" HTML_SIZE_FONT_HEADER(6) "?" HTML_FONT_TAIL "</p>");
        }
        str += table.ToString();
        return str;
    }

    std::string HandStringPrepare_(const uint64_t pid) const
    {
        std::string s = "- 手牌 (" + std::to_string(players_[pid].hand_.size()) + " / 13)：";
        for (const auto& tile : players_[pid].hand_) {
            s += tile.to_simple_string();
            s += ' ';
        }
        return s;
    }

    std::string HandHtmlAll_(const uint64_t pid, const TileStyle style) const
    {
        return "<center>\n\n" + HandHtml(option_.image_path_, players_[pid].hand_, style) + "\n\n</center>\n\n";
    }

    std::string HandStringAll_(const uint64_t pid) const
    {
        std::string s = "- 手牌：";
        for (const auto& tile : players_[pid].hand_) {
            s += tile.to_simple_string();
            s += ' ';
        }
        return s;
    }

    std::string HandHtmlBack_(const uint64_t pid) const
    {
        return "<center>\n\n" + HandHtmlBack(option_.image_path_, players_[pid].hand_) + "\n\n</center>\n\n";
    }

    std::string HandStringBack_(const uint64_t pid) const
    {
        std::string s = "- 透明手牌：";
        for (const auto& tile : players_[pid].hand_) {
            if (tile.toumei) {
                s += tile.to_simple_string();
                s += ' ';
            }
        }
        return s;
    }

    enum class RonOccu { NORMAL, ROUND_1, ROUND_17 };
    static CounterResult UpdateCounterResult_(
            CounterResult counter, const bool with_inner_dora, const RonOccu ron_occu, const bool ron_red_dora)
    {
        if (!with_inner_dora) {
            counter.fan -= std::erase(counter.yakus, Yaku::里宝牌);
        }
        const bool has_yaku_man = std::ranges::any_of(counter.yakus, [](const Yaku yaku) { return yaku > Yaku::满贯; });
        if (ron_occu == RonOccu::ROUND_1 && !has_yaku_man &&
                std::ranges::none_of(counter.yakus, [](const Yaku yaku) { return yaku == Yaku::一发; })) {
            counter.yakus.emplace_back(Yaku::一发);
            ++counter.fan;
        } else if (ron_occu == RonOccu::ROUND_17 && !has_yaku_man &&
                std::ranges::none_of(counter.yakus, [](const Yaku yaku) { return yaku == Yaku::河底捞鱼; })) {
            counter.yakus.emplace_back(Yaku::河底捞鱼);
            ++counter.fan;
        }
        if (ron_red_dora) {
            counter.yakus.emplace_back(Yaku::赤宝牌);
            ++counter.fan;
        }
        counter.calculate_score(false, false);
        return counter;
    }

    std::string YakusHtml_(const Tile& tile, const CounterResult& counter, const std::vector<std::string>& texts = {}) const
    {
        const std::string score_info = std::string(
                counter.score1 == 32000 * 4  ? "四倍役满" :
                counter.score1 == 32000 * 3  ? "三倍役满" :
                counter.score1 == 32000 * 2  ? "两倍役满" :
                counter.score1 == 32000      ? "役满" :
                counter.score1 == 24000      ? "三倍满" :
                counter.score1 == 16000      ? "倍满" :
                counter.score1 == 12000      ? "跳满" :
                counter.score1 == 8000       ? "满贯" : "") +
            " " + (counter.fan > 0 ? std::to_string(counter.fu) + " 符 " + std::to_string(counter.fan) + " 番" : "") +
        " " + std::to_string(counter.score1) + " 点";
        return YakusHtml(option_.image_path_, tile, score_info, counter.yakus, texts);
    }

    std::string YamaHtml_(const uint64_t pid) const
    {
        static constexpr const uint32_t k_col_size = 13;
        const Player& player = players_[pid];
        html::Table table((k_yama_tile_num_ + k_col_size - 1) / k_col_size * 2, k_col_size);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        uint32_t i = 0;
        for (const auto& tile : player.yama_) {
            table.Get(i / k_col_size * 2, i % k_col_size).SetContent(tile.to_simple_string());
            table.Get(i / k_col_size * 2 + 1, i % k_col_size).SetContent(Image_(tile, TileStyle::HAND));
            ++i;
        }
        return "<center>\n\n **剩余牌山** </center>\n\n" + table.ToString();
    }

    std::string YamaString_(const uint64_t pid) const
    {
        std::string s = "- 剩余牌山：";
        for (const auto& tile : players_[pid].yama_) {
            s += tile.to_simple_string();
            s += ' ';
        }
        return s;
    }

    std::string YamaHtmlForSpec_(const uint64_t pid) const
    {
        static constexpr const uint32_t k_col_size = 13;
        const Player& player = players_[pid];
        html::Table table((k_yama_tile_num_ + k_col_size - 1) / k_col_size, std::min(static_cast<size_t>(k_col_size), player.yama_.size()));
        table.SetTableStyle(" align=\"center\" cellpadding=\"3\" cellspacing=\"0\" ");
        uint32_t i = 0;
        for (const auto& tile : player.yama_) {
            table.Get(i / k_col_size, i % k_col_size).SetContent(Image_(tile, TileStyle::SMALL_HAND));
            ++i;
        }
        return "<center>\n\n **剩余牌山** </center>\n\n" + table.ToString();
    }

    void InitTable_(Table& table, const uint64_t pid)
    {
        table.dora_spec = doras_.size();
        for (auto& [dora, inner_dora] : doras_) {
            table.宝牌指示牌.emplace_back(&dora);
            table.里宝牌指示牌.emplace_back(&inner_dora);
        }
        table.players[pid].first_round = false; // not count 天和
        table.players[pid].riichi = true;
        table.players[pid].亲家 = false;
        table.players[pid].一发 = false;
        for (auto& tile : players_[pid].hand_) {
            table.players[pid].hand.emplace_back(const_cast<Tile*>(&tile));
        }
    }

    std::map<BaseTile, CounterResult> GetListenInfo_(const uint64_t pid)
    {
        std::map<BaseTile, CounterResult> ret;
        Table table;
        InitTable_(table, pid);
        auto basetiles = convert_tiles_to_base_tiles(table.players[pid].hand);
        for (uint8_t basetile = 0; basetile < 9 * 3 + 7; ++basetile) {
            if (4 == std::count_if(table.players[pid].hand.begin(), table.players[pid].hand.end(),
                        [&basetile](const Tile* tile) { return tile->tile == basetile; })) {
                continue; // all same tiles are in hand
            }
            Tile correspond_tile = Tile{.tile = static_cast<BaseTile>(basetile), .red_dora = 0};
            basetiles.emplace_back(correspond_tile.tile);
            if (is和牌(basetiles)) {
                auto counter = yaku_counter(&table, pid, &correspond_tile, false /*枪杠*/, false /*枪暗杠*/,
                        option_.player_descs_[pid].wind_ /*自风*/, Wind::East /*场风*/);
                if (std::ranges::any_of(counter.yakus,
                            [](const Yaku yaku) { return yaku > Yaku::满贯 && yaku < Yaku::双倍役满; })) {
                    // convert double 役满 to single 役满 (without 大四喜)
                    for (auto& yaku : counter.yakus) {
                        if (yaku == Yaku::国士无双十三面) {
                            yaku = Yaku::国士无双;
                            counter.yakuman -= 1;
                        } else if (yaku == Yaku::纯正九莲宝灯) {
                            yaku = Yaku::九莲宝灯;
                            counter.yakuman -= 1;
                        } else if (yaku == Yaku::四暗刻单骑) {
                            yaku = Yaku::四暗刻;
                            counter.yakuman -= 1;
                        }
                    }
                    // remove non 役满 tiles
                    std::erase_if(counter.yakus, [](const Yaku yaku) { return yaku < Yaku::满贯; });
                }
                counter.calculate_score(false, false);
                ret.emplace(static_cast<BaseTile>(basetile), std::move(counter));
            }
            basetiles.pop_back();
        }
        return ret;
    }

    std::string Image_(const Tile& tile, const TileStyle style) const
    {
        return Image(option_.image_path_, tile, style);
    }
    std::string BackImage_(const TileStyle style) const
    {
        return BackImage(option_.image_path_, style);
    }

    const Mahjong17StepsOption option_;
    uint32_t round_;
    bool is_flow_;
    std::array<Player, 4> players_;
    std::vector<std::pair<Tile, Tile>> doras_; // inner dora are not currently used
    std::string errstr_;
};

} // namespace mahjong

} // namespace game_util

} // namespace lgtbot

#ifdef TEST_BOT
#undef private
#endif
