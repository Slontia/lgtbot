// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <span>

#include "game_util/mahjong_util.h"
#include "Mahjong/Rule.h"

using namespace std::string_literals;

namespace lgtbot {

namespace game_util {

namespace mahjong {

static constexpr const uint32_t k_yama_tile_num = 19;
static constexpr const std::array<BaseTile, 13> k_one_nine_tiles{_1m, _9m, _1s, _9s, _1p, _9p, east, south, west, north, 白, 发, 中};
static constexpr const uint32_t k_max_dora_num = 4;

struct RiverTile
{
    uint32_t kiri_round_{0};
    uint32_t is_tsumo_kiri_{0};
    Tile tile_;
    bool richii_{false};
};

struct KiriTile
{
    Tile tile_;
    enum class Type : uint8_t { NORMAL, ADD_KAN, DARK_KAN, RIVER_BOTTOM } type_;
};

struct PlayerKiriInfo
{
    uint32_t player_id_{UINT32_MAX};
    std::vector<KiriTile> kiri_tiles_;
};

struct KiriInfo
{
    std::vector<PlayerKiriInfo> other_players_;
    TileSet other_player_kiri_tiles_; // these tiles can used to nari
};

struct FuruTile
{
    Tile tile_;
    enum class Type : uint8_t { EMPTY, NORMAL, NARI, ADD_KAN, DARK } type_{Type::EMPTY};
};

struct Furu
{
    uint32_t nari_round_{0};
    std::array<FuruTile, 4> tiles_;
};

enum class ActionState
{
    ROUND_BEGIN, // nari, get tile
    AFTER_CHI_PON, // kiri
    AFTER_GET_TILE, // kiri, kan, richii (if has no furu)
    AFTER_KAN, // kiri, kan, richii (if has no furu)
    AFTER_KAN_CAN_NARI, // kiri, kan
    AFTER_KIRI, // nari, over
    ROUND_OVER,
    NOTIFIED_RON,
};

struct FuResult {
    static constexpr const uint32_t k_tsumo_player_id_ = UINT32_MAX;
    uint32_t player_id_;
    CounterResult counter_;
    Tile tile_;
};

struct SyncMahjongOption
{
#ifdef TEST_BOT
    std::variant<TilesOption, std::string> tiles_option_;
#else
    TilesOption tiles_option_;
#endif
    std::string image_path_;
    int32_t benchang_{0};
    int32_t richii_points_{0};
    std::vector<PlayerDesc> player_descs_;
};

class DorasManager
{
  public:
    DorasManager(const std::array<std::pair<Tile, Tile>, 4>& doras) : doras_(doras) {}

    void TryOpenNewDora(const int32_t round)
    {
        if (round > last_open_round_ && dora_num_ < k_max_dora_num) {
            last_open_round_ = round;
            ++dora_num_;
        }
    }

    bool HasOpenedThisRound(const int32_t round) const { return last_open_round_ == round; }

    std::span<const std::pair<Tile, Tile>> Doras() const
    {
        return std::span<const std::pair<Tile, Tile>>(doras_.begin(), doras_.begin() + dora_num_);
    }

  private:
    std::array<std::pair<Tile, Tile>, 4> doras_;
    int32_t last_open_round_{0};
    uint32_t dora_num_{1};
};

class SyncMahjongGamePlayer
{
    friend class SyncMajong;

  public:
    SyncMahjongGamePlayer(const SyncMahjongOption& option, const uint32_t player_id, const std::array<Tile, k_yama_tile_num>& yama,
            TileSet hand, const std::array<std::pair<Tile, Tile>, 4>& doras)
        : option_(option)
        , player_id_(player_id)
        , wind_(option.player_descs_[player_id].wind_)
        , yama_(yama)
        , hand_(std::move(hand))
        , cur_round_my_kiri_info_{.player_id_ = player_id}
        , doras_manager_(doras)
    {}

    std::string PublicDoraHtml() const { return DoraHtml(option_.image_path_, false, doras_manager_.Doras(), false); }

    enum class HtmlMode { OPEN, PRIVATE, PUBLIC };
    std::string Html(const HtmlMode html_mode) const
    {
        std::string s;
        s += PlayerNameHtml(option_.player_descs_[player_id_], point_variation_) + "\n\n";
        s += "<center>\n\n**剩余牌山：" HTML_COLOR_FONT_HEADER(blue) + std::to_string(k_yama_tile_num - yama_idx_) + HTML_FONT_TAIL + "**\n\n</center>\n\n";
        if (!fu_results_.empty()) {
            s += "<center> <font size=\"6\">\n\n " HTML_COLOR_FONT_HEADER(blue) " **和&nbsp;&nbsp;了** "
                HTML_FONT_TAIL "\n\n</font> </center>\n\n";
            s += DoraHtml(option_.image_path_, IsRiichi_(), doras_manager_.Doras(), true);
            s += "\n\n";
            s += HandHtml_(TileStyle::FORWARD);
            s += "\n\n";
            s += RonInfoHtml_(option_, fu_results_);
            s += "\n\n<br />\n\n";
        } else if (html_mode == HtmlMode::OPEN) {
            s += HandHtml_(TileStyle::FORWARD) + "\n\n<br />\n\n";
        } else if (html_mode == HtmlMode::PRIVATE) {
            s += HandHtml_(TileStyle::HAND) + "\n\n<br />\n\n";
            if (IsFurutin_()) {
                s += "<center>\n\n" HTML_COLOR_FONT_HEADER(red) " **振听中，无法荣和** " HTML_FONT_TAIL "\n\n</center>";
            }
        } else {
            s += HandHtmlBack_() + "\n\n<br />\n\n";
        }
        if (IsRiichi_()) {
            s += "<div align=\"center\"><img src=\"file:///" + option_.image_path_ + "/riichi.png\"/></div>\n\n<br />\n\n";
        }
        s += RiverHtml_(option_.image_path_, river_);
        return s;
    }

    void PerformDefault()
    {
        switch (state_) {
            case ActionState::ROUND_BEGIN:
                assert(yama_idx_ < k_yama_tile_num);
                tsumo_ = yama_[yama_idx_++];
            case ActionState::AFTER_GET_TILE:
            case ActionState::AFTER_KAN:
            case ActionState::AFTER_KAN_CAN_NARI:
                KiriInternal_(true /*is_tsumo*/, false /*richii*/, *tsumo_);
                tsumo_ = std::nullopt;
                break;
            case ActionState::AFTER_CHI_PON:
                KiriInternal_(false /*is_tsumo*/, false /*richii*/, *hand_.begin());
                hand_.erase(hand_.begin());
                break;
        }
        state_ = ActionState::ROUND_OVER;
    }

    bool GetTile()
    {
        if (state_ != ActionState::ROUND_BEGIN) {
            errstr_ = "当前状态不允许摸牌";
            return false;
        }
        GetTileInternal_();
        state_ = ActionState::AFTER_GET_TILE;
        return true;
    }

    bool Nagashi()
    {
        if (state_ != ActionState::AFTER_GET_TILE) {
            errstr_ = "当前状态不允许宣告九种九牌流局";
            return false;
        }
        if (round_ != 1) {
            errstr_ = "仅第一巡允许宣告九种九牌流局";
            return false;
        }
        if (!CanNagashi_()) {
            errstr_ = "手牌中幺九牌小于九种，无法宣告九种九牌流局";
            return false;
        }
        state_ = ActionState::ROUND_OVER;
        return true;
    }

    bool Kiri(const std::string_view tile, const bool richii)
    {
        if (state_ != ActionState::AFTER_CHI_PON && state_ != ActionState::AFTER_GET_TILE && state_ != ActionState::AFTER_KAN && state_ != ActionState::AFTER_KAN_CAN_NARI) {
            errstr_ = "当前状态不允许切牌";
            return false;
        }
        if (richii && IsRiichi_()) {
            errstr_ = "您已经立直";
            return false;
        }
        if (!tile.empty() && IsRiichi_()) {
            errstr_ = "立直状态下只能选择摸切";
            return false;
        }
        if (richii && !std::ranges::all_of(furus_, [](const Furu& furu) { return furu.tiles_[3].type_ == FuruTile::Type::DARK; })) {
            errstr_ = "在有副露的情况下不允许立直";
            return false;
        }
        if (richii && option_.player_descs_[player_id_].base_point_ < 1000) {
            errstr_ = "您的点数不足 1000，无法立直";
            return false;
        }
        const bool succ = tile.empty() ? KiriTsumo_(richii) : Kiri_(tile, richii);
        if (succ) {
            state_ = state_ == ActionState::AFTER_CHI_PON || state_ == ActionState::AFTER_KAN_CAN_NARI ? ActionState::AFTER_KIRI :
                ActionState::ROUND_OVER;
        }
        return succ;
    }

    bool Over()
    {
        if (state_ != ActionState::AFTER_KIRI && state_ != ActionState::NOTIFIED_RON) {
            errstr_ = "当前状态不允许结束行动";
            return false;
        }
        if (IsRiichi_()) {
            is_richii_furutin_ = true;
        }
        state_ = ActionState::ROUND_OVER;
        return true;
    }

    bool Chi(const std::string_view hand_tiles, const std::string_view others_kiri_tile)
    {
        if (state_ != ActionState::ROUND_BEGIN && state_ != ActionState::AFTER_KIRI) {
            errstr_ = "当前状态不允许吃牌";
            return false;
        }
        const auto tiles = GetTilesFrom(hand_, hand_tiles, errstr_);
        const auto kiri_tile = GetTilesFrom(cur_round_kiri_info_.other_player_kiri_tiles_, others_kiri_tile, errstr_);
        const auto rollback =
            [&] {
                hand_.insert(tiles.begin(), tiles.end());
                cur_round_kiri_info_.other_player_kiri_tiles_.insert(kiri_tile.begin(), kiri_tile.end());
            };
        if (tiles.empty()) {
            errstr_ = "您的手牌中不存在「"s + hand_tiles.data() + "」";
            rollback();
            return false;
        }
        if (kiri_tile.empty()) {
            errstr_ = "前巡舍牌中不存在「"s + others_kiri_tile.data() + "」";
            rollback();
            return false;
        }
        if (tiles.size() != 2) {
            errstr_ = "您需要从手牌中指定 2 张牌，目前指定了「"s + hand_tiles.data() + "」共 " + std::to_string(tiles.size()) + " 张牌";
            rollback();
            return false;
        }
        if (kiri_tile.size() != 1) {
            errstr_ = "您需要从前巡舍牌中指定 1 张牌，目前指定了「"s + others_kiri_tile.data() + "」共 " + std::to_string(kiri_tile.size()) + " 张牌";
            rollback();
            return false;
        }
        if (!is_顺子({tiles.begin()->tile, std::next(tiles.begin())->tile, kiri_tile.begin()->tile})) {
            errstr_ = "「"s + hand_tiles.data() + others_kiri_tile.data() + "」无法形成顺子";
            rollback();
            return false;
        }
        furus_.emplace_back();
        auto& furu = furus_.back();
        furu.nari_round_ = round_;
        furu.tiles_[0] = FuruTile{*tiles.begin(), FuruTile::Type::NORMAL};
        furu.tiles_[1] = FuruTile{*std::next(tiles.begin()), FuruTile::Type::NORMAL};
        furu.tiles_[2] = FuruTile{*kiri_tile.begin(), FuruTile::Type::NARI};
        std::sort(furu.tiles_.begin(), furu.tiles_.begin() + 3,
                [](const auto& _1, const auto& _2) { return _1.tile_.tile < _2.tile_.tile; });
        has_chi_ = true;
        state_ = ActionState::AFTER_CHI_PON;
        return true;
    }

    bool Pon(const std::string_view hand_tiles)
    {
        if (state_ != ActionState::ROUND_BEGIN && state_ != ActionState::AFTER_KIRI) {
            errstr_ = "当前状态不允许碰牌";
            return false;
        }
        // If player only specifies one tile, we should repeat the tile string to specify two tiles.
        const auto tiles = hand_tiles.size() > 3 ?
            GetTilesFrom(hand_, hand_tiles, errstr_) :
            GetTilesFrom(hand_, std::string(hand_tiles) + std::string(hand_tiles), errstr_);
        if (tiles.empty()) {
            errstr_ = "您的手牌中不存在足够的「"s + hand_tiles.data() + "」";
            return false;
        }
        const auto rollback = [&] { hand_.insert(tiles.begin(), tiles.end()); };
        if (tiles.size() != 2) {
            errstr_ = "您需要从手牌中指定 2 张牌，目前指定了「"s + hand_tiles.data() + "」共 " + std::to_string(tiles.size()) + " 张牌";
            rollback();
            return false;
        }
        const std::string& tile_str = basetile_to_string_simple(tiles.begin()->tile);
        const auto kiri_tile = GetTilesFrom(cur_round_kiri_info_.other_player_kiri_tiles_, tile_str, errstr_);
        if (kiri_tile.empty()) {
            errstr_ = "前巡舍牌中不存在「"s + tile_str + "」";
            rollback();
            return false;
        }
        assert(kiri_tile.size() == 1);
        assert(is_刻子({tiles.begin()->tile, std::next(tiles.begin())->tile, kiri_tile.begin()->tile}));
        furus_.emplace_back();
        auto& furu = furus_.back();
        furu.nari_round_ = round_;
        furu.tiles_[0] = FuruTile{*tiles.begin(), FuruTile::Type::NORMAL};
        furu.tiles_[1] = FuruTile{*std::next(tiles.begin()), FuruTile::Type::NORMAL};
        furu.tiles_[2] = FuruTile{*kiri_tile.begin(), FuruTile::Type::NARI};
        state_ = ActionState::AFTER_CHI_PON;
        return true;
    }

    bool Kan(const std::string_view tile_sv)
    {
        if (tile_sv.size() <= 1) {
            errstr_ = "非法的牌型「"s + tile_sv.data() + "」";
            return false;
        }
        if (tile_sv.size() > 3) {
            errstr_ = "您需要且仅需要指定一张牌";
            return false;
        }
        if (yama_idx_ == k_yama_tile_num) {
            // no more tiles
            errstr_ = "您当前无可摸牌，无法杠牌";
            return false;
        }
        bool succ = false;
        if (state_ == ActionState::ROUND_BEGIN || state_ == ActionState::AFTER_KIRI) {
            succ = DirectKan_(tile_sv);
        } else if (state_ == ActionState::AFTER_GET_TILE || state_ == ActionState::AFTER_KAN || state_ == ActionState::AFTER_KAN_CAN_NARI) {
            succ = DarkKanOrAddKan_(tile_sv);
        } else {
            errstr_ = "当前状态不允许杠牌";
            return false;
        }
        if (succ) {
            doras_manager_.TryOpenNewDora(round_);
            tsumo_ = yama_[yama_idx_++];
            state_ = (state_ == ActionState::ROUND_BEGIN || state_ == ActionState::AFTER_KIRI || state_ == ActionState::AFTER_KAN_CAN_NARI)
                ? ActionState::AFTER_KAN_CAN_NARI : ActionState::AFTER_KAN;
        }
        return succ;
    }

    bool Tsumo()
    {
        if (state_ != ActionState::AFTER_GET_TILE && state_ != ActionState::AFTER_KAN && state_ != ActionState::AFTER_KAN_CAN_NARI) {
            errstr_ = "当前状态不允许自摸";
            return false;
        }
        assert(tsumo_.has_value());
        std::optional<CounterResult> counter = GetCounter_(*tsumo_, FuType::TSUMO, yama_idx_ == k_yama_tile_num, &errstr_);
        if (counter.has_value()) {
            counter->calculate_score(true, true);
        }
        if (counter.has_value()) {
            fu_results_.emplace_back(FuResult::k_tsumo_player_id_, std::move(*counter), *tsumo_);
        }
        if (fu_results_.empty()) {
            // `errstr_` is filled by `GetCounter_`
            return false;
        }
        state_ = ActionState::ROUND_OVER;
        return true;
    }

    bool Ron()
    {
        if (IsFurutin_()) {
            errstr_ = "当前处于振听状态";
            return false;
        }
        if (has_chi_) {
            errstr_ = "吃过牌后只能通过自摸和牌";
            return false;
        }
        if (state_ == ActionState::NOTIFIED_RON) {
            fu_results_ = GetFuResultsForRon_();
        } else if (state_ == ActionState::AFTER_KIRI) {
            fu_results_ = GetFuResultsForNariRon_();
        } else {
            errstr_ = "当前状态不允许荣和";
            return false;
        }
        if (fu_results_.empty()) {
            errstr_ = "其他玩家舍牌无法使手牌构成合法和牌型";
            return false;
        }
        state_ = ActionState::ROUND_OVER;
        return true;
    }

    ActionState State() const { return state_; }

    int64_t PointVariation() const { return point_variation_; }

    uint32_t PlayerID() const { return player_id_; }

    const std::string& ErrorString() const { return errstr_; }

  private:
    enum class FuType { TSUMO, ROB_KAN, RON };

    static std::string RiverHtml_(const std::string& image_path, const std::vector<RiverTile>& river)
    {
        if (river.empty()) {
            return "";
        }
        html::Table table(2, 0);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        table.SetRowStyle(" align=\"center\" ");
        for (uint32_t i = 0; i < river.size(); ++i) {
            table.AppendColumn();
            if (river[i].is_tsumo_kiri_) {
                table.GetLastColumn(0).SetContent(HTML_COLOR_FONT_HEADER(red) + std::to_string(river[i].kiri_round_) + HTML_FONT_TAIL);
            } else {
                table.GetLastColumn(0).SetContent(std::to_string(river[i].kiri_round_));
            }
            table.GetLastColumn(1).SetContent(Image(image_path, river[i].tile_, river[i].richii_ ? TileStyle::LEFT : TileStyle::FORWARD));
            if ((i + 1) % 6 == 0) {
                table.AppendColumn();
                table.GetLastColumn(1).SetContent("&nbsp;&nbsp;");
            }
        }
        return table.ToString();
    }

    static std::string FuruTilesHtml_(const std::string& image_path, const std::array<FuruTile, 4>& furu_tiles)
    {
        html::Table table(2, 4);
        table.SetTableStyle(" align=\"center\" valign=\"bottom\" cellpadding=\"0\" cellspacing=\"0\" width=\"1\"");
        table.SetRowStyle(" valign=\"bottom\" ");
        for (uint32_t i = 0; i < furu_tiles.size(); ++i) {
            const FuruTile& furu_tile = furu_tiles[i];
            switch (furu_tile.type_) {
                case FuruTile::Type::EMPTY:
                    break;
                case FuruTile::Type::NORMAL:
                    table.MergeDown(0, i, 2);
                    table.Get(0, i).SetContent(Image(image_path, furu_tile.tile_, TileStyle::FORWARD));
                    break;
                case FuruTile::Type::DARK:
                    table.MergeDown(0, i, 2);
                    if (furu_tile.tile_.toumei) {
                        table.Get(0, i).SetContent(Image(image_path, furu_tile.tile_, TileStyle::FORWARD));
                    } else {
                        table.Get(0, i).SetContent(BackImage(image_path, TileStyle::FORWARD));
                    }
                    break;
                case FuruTile::Type::NARI:
                    table.Get(1, i).SetContent("<img src=\"file:///" + image_path + "/2_n" + Tile{.tile = furu_tile.tile_.tile}.to_simple_string() + ".png\"/>");
                    break;
                case FuruTile::Type::ADD_KAN:
                    table.Get(0, i - 1).SetContent(Image(image_path, furu_tile.tile_, TileStyle::LEFT));
                    break;
                default:
                    assert(false);
            }
        }
        return table.ToString();
    }

    static std::string YakusHtml_(const SyncMahjongOption& option, const Tile& tile, const CounterResult& counter, const bool as_tsumo, const std::vector<std::string>& texts = {})
    {
        const auto score = as_tsumo ? counter.score1 * (option.player_descs_.size() - 1) : counter.score1;
        const std::string score_info = std::string(
                score == 48000 * 6  ? "六倍役满" :
                score == 48000 * 5  ? "五倍役满" :
                score == 48000 * 4  ? "四倍役满" :
                score == 48000 * 3  ? "三倍役满" :
                score == 48000 * 2  ? "两倍役满" :
                score == 48000      ? "役满" :
                score == 36000      ? "三倍满" :
                score == 24000      ? "倍满" :
                score == 18000      ? "跳满" :
                score == 12000      ? "满贯" : "") +
            " " + (counter.fan > 0 ? std::to_string(counter.fu) + " 符 " + std::to_string(counter.fan) + " 番" : "") +
            " " + std::to_string(score) + " 点";
        return YakusHtml(option.image_path_, tile, score_info, counter.yakus, texts,
                std::vector<Yaku>{Yaku::场风_东, Yaku::场风_北, Yaku::场风_南, Yaku::场风_西});
    }

    static std::string RonInfoHtml_(const SyncMahjongOption& option, const std::vector<FuResult>& fu_results)
    {
        std::string s;
        for (const auto& result : fu_results) {
            std::vector<std::string> texts;
            if (result.player_id_ != FuResult::k_tsumo_player_id_) {
                texts.emplace_back(LoserHtml(option.player_descs_[result.player_id_]));
            }
            s += YakusHtml_(option, result.tile_, result.counter_, result.player_id_ == FuResult::k_tsumo_player_id_, texts) + "\n\n";
        }
        return s;
    }

    std::string AppendFuruHtmls_(std::string hand_html) const {
        html::Table table(2, 1 + (!furus_.empty()) + furus_.size());
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        table.SetRowStyle(" align=\"center\" valign=\"bottom\" ");
        table.MergeDown(0, 0, 2);
        table.Get(0, 0).SetContent(std::move(hand_html));
        if (furus_.empty()) {
            return table.ToString();
        }
        table.MergeDown(0, 1, 2);
        table.Get(0, 1).SetContent(HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE);
        for (int64_t i = furus_.size() - 1; i >= 0; --i) {
            table.Get(0, 2 + furus_.size() - 1 - i).SetContent(std::to_string(furus_[i].nari_round_));
            table.Get(1, 2 + furus_.size() - 1 - i).SetContent(FuruTilesHtml_(option_.image_path_, furus_[i].tiles_));
        }
        return table.ToString();
    }

    std::string HandHtml_(const TileStyle tile_style) const {
        const auto hand_html_func = tile_style == TileStyle::HAND ? &HandHtmlWithName : &HandHtml;
        return AppendFuruHtmls_(hand_html_func(option_.image_path_, hand_, tile_style, tsumo_));
    }

    std::string HandHtmlBack_() const {
        return AppendFuruHtmls_(HandHtmlBack(option_.image_path_, hand_));
    }

    bool IsRiichi_() const { return !richii_listen_tiles_.empty() && richii_round_ != round_; }

    void StartNormalStage_(const uint32_t round)
    {
        assert(state_ == ActionState::ROUND_OVER);
        round_ = round;
        cur_round_my_kiri_info_.kiri_tiles_.clear();
        if (!IsRiichi_() && (CanChi_() || CanPon_())) {
            state_ = ActionState::ROUND_BEGIN;
            return;
        }
        GetTileInternal_();
        if (!IsRiichi_() || CanTsumo_() || CanDarkKanInRichiiState_()) {
            state_ = ActionState::AFTER_GET_TILE;
            return;
        }
        KiriInternal_(true /*is_tsumo*/, false /*richii*/, *tsumo_);
        tsumo_ = std::nullopt;
    }

    void SetKiriInfos_(const std::vector<PlayerKiriInfo>& kiri_infos)
    {
        cur_round_kiri_info_.other_players_.clear();
        cur_round_kiri_info_.other_player_kiri_tiles_.clear();
        for (uint32_t player_id = 0; player_id < kiri_infos.size(); ++player_id) {
            if (player_id == player_id_) {
                continue;
            }
            cur_round_kiri_info_.other_players_.emplace_back(kiri_infos[player_id]);
            for (const auto& kiri_tile : kiri_infos[player_id].kiri_tiles_) {
                if (kiri_tile.type_ == KiriTile::Type::NORMAL || kiri_tile.type_ == KiriTile::Type::RIVER_BOTTOM) {
                    cur_round_kiri_info_.other_player_kiri_tiles_.emplace(kiri_tile.tile_);
                }
            }
        }
    }

    void StartRonStage_(const std::vector<PlayerKiriInfo>& kiri_infos)
    {
        SetKiriInfos_(kiri_infos);
        if (CanRon_()) {
            state_ = ActionState::NOTIFIED_RON;
        }
    }

    void GetTileInternal_()
    {
        assert(yama_idx_ < k_yama_tile_num);
        tsumo_ = yama_[yama_idx_++];
    }

    bool Richii_()
    {
        std::vector<BaseTile> listen_tiles = GetListenTiles_();
        if (listen_tiles.empty()) {
            errstr_ = "切这张牌无法构成听牌牌型";
            return false;
        }
        assert(richii_listen_tiles_.empty());
        richii_listen_tiles_ = std::move(listen_tiles);
        is_richii_furutin_ = IsSelfFurutin_(richii_listen_tiles_, river_);
        is_double_richii_ = river_.empty() && furus_.empty();
        richii_round_ = round_;
        return true;
    }

    bool KiriInternal_(const bool is_tsumo, const bool richii, const Tile& tile)
    {
        if (richii && !Richii_()) {
            return false;
        }
        river_.emplace_back(round_, is_tsumo, tile, richii);
        const auto kiri_type = yama_idx_ == k_yama_tile_num ? KiriTile::Type::RIVER_BOTTOM : KiriTile::Type::NORMAL;
        cur_round_my_kiri_info_.kiri_tiles_.emplace_back(KiriTile{tile, kiri_type});
        return true;
    }

    bool IsFurutin_() const
    {
        return (richii_listen_tiles_.empty() && IsSelfFurutin_(GetListenTiles_(), river_)) || is_richii_furutin_;
    }

    static bool IsSelfFurutin_(const std::vector<BaseTile>& listen_tiles, const std::vector<RiverTile>& river)
    {
        return std::ranges::any_of(river,
                [&listen_tiles](const RiverTile& river_tile) { return std::ranges::find(listen_tiles, river_tile.tile_.tile) != listen_tiles.end(); });
    }

    bool KiriTsumo_(const bool richii)
    {
        if (!tsumo_.has_value()) {
            errstr_ = "您当前没有自摸牌，无法摸切";
            return false;
        }
        assert(tsumo_.has_value());
        const auto tsumo = *tsumo_;
        tsumo_ = std::nullopt;
        if (!KiriInternal_(true, richii, *tsumo_)) {
            tsumo_ = tsumo;
            return false;
        }
        return true;
    }

    bool MatchTsumo_(const std::string_view tile_sv)
    {
        assert(tile_sv.size() == 2 || tile_sv.size() == 3);
        if (!tsumo_.has_value()) {
            return false;
        }
        const std::string tsumo_str = tsumo_->to_simple_string();
        const auto match_not_toumei_tile = [](const std::string_view tsumo_sv, const std::string_view tile_sv)
            {
                assert(tsumo_sv.size() == 2 && tile_sv.size() == 2);
                return tsumo_sv == tile_sv || (tsumo_sv[0] == '0' && tile_sv[0] == '5' && tsumo_sv[1] == tile_sv[1]);
            };
        const auto to_not_toumei_tile = [](const std::string_view toumei_tile_sv) -> std::string_view
            {
                assert(toumei_tile_sv.size() == 3);
                return toumei_tile_sv.substr(1);
            };
        return (tsumo_str.size() == 2 && tile_sv.size() == 2 && match_not_toumei_tile(tsumo_str, tile_sv)) ||
            (tsumo_str.size() == 3 && tile_sv.size() == 2 && match_not_toumei_tile(to_not_toumei_tile(tsumo_str), tile_sv)) ||
            (tsumo_str.size() == 3 && tile_sv.size() == 3 && match_not_toumei_tile(to_not_toumei_tile(tsumo_str), to_not_toumei_tile(tile_sv)));
    }

    bool Kiri_(const std::string_view tile_sv, const bool richii)
    {
        if (tile_sv.size() <= 1) {
            errstr_ = "非法的牌型「"s + tile_sv.data() + "」";
            return false;
        }
        if (tile_sv.size() > 3) {
            errstr_ = "您需要且仅需要指定一张牌";
            return false;
        }
        const auto tiles = GetTilesFrom(hand_, tile_sv, errstr_);
        if (tiles.empty()) {
            if (!MatchTsumo_(tile_sv)) {
                errstr_ = "您的手牌中不存在「"s + tile_sv.data() + "」";
                return false;
            }
            return KiriTsumo_(richii);
        }
        assert(tiles.size() == 1);
        if (!KiriInternal_(false, richii, *tiles.begin())) {
            hand_.insert(tiles.begin(), tiles.end()); // rollback
            return false;
        }
        if (tsumo_.has_value()) {
            hand_.emplace(*tsumo_);
            tsumo_ = std::nullopt;
        }
        return true;
    }

    bool DarkKanOrAddKan_(const std::string_view tile_sv) {
        assert(tsumo_.has_value());
        assert(tile_sv.size() <= 3);
        const bool kan_tsumo = MatchTsumo_(tile_sv);
        if (IsRiichi_() && !kan_tsumo) {
            errstr_ = "立直状态下只能选择暗杠或加杠自摸牌";
            return false;
        }
        const auto handle_tsumo = [&]() {
            if (!kan_tsumo) {
                hand_.emplace(*tsumo_);
            }
            tsumo_ = std::nullopt;
        };
        const std::string tile_str(tile_sv);
        const auto hand_tiles = kan_tsumo ? GetTilesFrom(hand_, tile_str + tile_str + tile_str, errstr_)
                                          : GetTilesFrom(hand_, tile_str + tile_str + tile_str + tile_str, errstr_);
        if (!hand_tiles.empty()) {
            // dark kan
            const Tile kan_tile = kan_tsumo ? *tsumo_ : *std::next(hand_tiles.begin(), 3);
            assert(is_杠({hand_tiles.begin()->tile, std::next(hand_tiles.begin())->tile,
                        std::next(hand_tiles.begin(), 2)->tile, kan_tile.tile}));
            if (!richii_listen_tiles_.empty() && GetListenTiles_() != richii_listen_tiles_) {
                // kan after richii cannot change tinpai
                hand_.insert(hand_tiles.begin(), hand_tiles.end());
                errstr_ = "立直状态下暗杠或加杠不允许改变听牌种类";
                return false;
            }
            furus_.emplace_back();
            auto& furu = furus_.back();
            furu.nari_round_ = round_;
            furu.tiles_[0] = FuruTile{*hand_tiles.begin(), FuruTile::Type::DARK};
            furu.tiles_[1] = FuruTile{*std::next(hand_tiles.begin()), FuruTile::Type::NORMAL};
            furu.tiles_[2] = FuruTile{*std::next(hand_tiles.begin(), 2), FuruTile::Type::NORMAL};
            furu.tiles_[3] = FuruTile{kan_tile, FuruTile::Type::DARK};
            cur_round_my_kiri_info_.kiri_tiles_.emplace_back(KiriTile{kan_tile, KiriTile::Type::DARK_KAN});
            handle_tsumo();
            return true;
        }
        const auto one_hand_tile = GetTilesFrom(hand_, tile_str, errstr_);
        if (!kan_tsumo && one_hand_tile.empty()) {
            errstr_ = "您的手牌中不存在「"s + tile_sv.data() + "」";
            return false;
        }
        const Tile& kan_tile = kan_tsumo ? *tsumo_ : *one_hand_tile.begin();
        bool found = false;
        for (auto& furu : furus_) {
            if (furu.tiles_[0].tile_.tile == kan_tile.tile && furu.tiles_[1].tile_.tile == kan_tile.tile) {
                // add kan
                assert(furu.tiles_[3].type_ == FuruTile::Type::EMPTY);
                furu.nari_round_ = round_;
                furu.tiles_[3].type_ = FuruTile::Type::ADD_KAN;
                furu.tiles_[3].tile_ = kan_tile;
                cur_round_my_kiri_info_.kiri_tiles_.emplace_back(KiriTile{kan_tile, KiriTile::Type::ADD_KAN});
                handle_tsumo();
                found = true;
                break;
            }
        }
        if (!found) {
            hand_.insert(one_hand_tile.begin(), one_hand_tile.end());
            errstr_ = "您的副露中不存在「"s + tile_sv.data() + "」的刻子";
        }
        return found;
    }

    bool CanNagashi_() const
    {
        std::array<bool, k_tile_type_num> basetile_bitset{false};
        for (const Tile& tile : hand_) {
            basetile_bitset[tile.tile] = true;
        }
        if (tsumo_.has_value()) {
            basetile_bitset[tsumo_->tile] = true;
        }
        return std::ranges::count_if(k_one_nine_tiles, [&](const BaseTile basetile) { return basetile_bitset[basetile]; }) >= 9;
    }

    bool CanChi_() const
    {
        // TODO
        return round_ != 1;
    }

    bool CanPon_() const
    {
        // TODO
        return round_ != 1;
    }

    bool CanDarkKanInRichiiState_() const
    {
        // TODO
        assert(IsRiichi_());
        return true;
    }

    // If `CanRon_()` returns true, `Ron()` must return true.
    bool CanRon_() const
    {
        if (IsFurutin_() || has_chi_) {
            return false;
        }
        for (const auto& player_info : cur_round_kiri_info_.other_players_) {
            for (const auto& kiri_tile : player_info.kiri_tiles_) {
                if (RonCounter_(kiri_tile).has_value()) {
                    return true;
                }
            }
        }
        return false;
    }

    bool CanTsumo_() const
    {
        assert(tsumo_.has_value());
        return GetCounter_(*tsumo_, FuType::TSUMO, yama_idx_ == k_yama_tile_num, nullptr).has_value();
    }

    std::optional<CounterResult> GetCounter_(const Tile& tile, const FuType fu_type, const bool is_last_tile, std::string* const errstr = nullptr) const
    {
        Table table;
        InitTable_(table);
        auto basetiles = convert_tiles_to_base_tiles(table.players[player_id_].hand);
        basetiles.emplace_back(tile.tile);
        if (!is和牌(basetiles)) {
            if (errstr) {
                *errstr = "不满足和牌牌型";
            }
            return std::nullopt;
        }
        if (fu_type == FuType::TSUMO) {
            table.players[player_id_].hand.emplace_back(const_cast<Tile*>(&tile));
        }
        if (is_last_tile) {
            table.牌山.resize(14);
        }
        auto counter = yaku_counter(&table, player_id_, fu_type == FuType::TSUMO ? nullptr : const_cast<Tile*>(&tile),
                fu_type == FuType::ROB_KAN /*枪杠*/, false /*枪暗杠*/, wind_ /*自风*/, wind_ /*场风*/);
        if (counter.yakus.empty() || counter.yakus[0] == Yaku::None) {
            if (errstr) {
                *errstr = "无役";
            }
            return std::nullopt;
        }
        counter.fan -= std::ranges::count_if(counter.yakus, [](const Yaku yaku)
                {
                    return yaku == Yaku::场风_东 || yaku == Yaku::场风_北 || yaku == Yaku::场风_南 || yaku == Yaku::场风_西;
                });
        if (std::ranges::any_of(counter.yakus,
                    [](const Yaku yaku) { return yaku > Yaku::满贯 && yaku < Yaku::双倍役满; })) {
            std::erase_if(counter.yakus, [](const Yaku yaku) { return yaku < Yaku::满贯; });
        }
        return counter;
    }

    std::optional<CounterResult> RonCounter_(const KiriTile& kiri_tile) const
    {
        // `other_player_kiri_tiles_` does not contain add kan or dark kan tiles
        if (kiri_tile.type_ != KiriTile::Type::ADD_KAN && kiri_tile.type_ != KiriTile::Type::DARK_KAN &&
                cur_round_kiri_info_.other_player_kiri_tiles_.find(kiri_tile.tile_) == cur_round_kiri_info_.other_player_kiri_tiles_.end()) {
            // this tile has been used, skip
            return std::nullopt;
        }
        const FuType fu_type =
            kiri_tile.type_ == KiriTile::Type::ADD_KAN || kiri_tile.type_ == KiriTile::Type::DARK_KAN ? FuType::ROB_KAN : FuType::RON;
        const std::optional<CounterResult> counter = GetCounter_(kiri_tile.tile_, fu_type, kiri_tile.type_ == KiriTile::Type::RIVER_BOTTOM, nullptr);
        if (!counter.has_value()) {
            return std::nullopt;
        }
        if (kiri_tile.type_ == KiriTile::Type::DARK_KAN &&
                std::ranges::none_of(counter->yakus, [](const Yaku yaku) { return yaku == Yaku::国士无双; })) {
            return std::nullopt;
        }
        return counter;
    }

    std::vector<FuResult> GetFuResultsForRon_() const
    {
        assert(state_ == ActionState::NOTIFIED_RON);
        std::vector<FuResult> fu_results;
        for (const auto& player_info : cur_round_kiri_info_.other_players_) {
            std::optional<FuResult> player_result;
            for (const auto& kiri_tile : player_info.kiri_tiles_) {
                std::optional<CounterResult> counter = RonCounter_(kiri_tile);
                if (!counter.has_value()) {
                    // cannot ron this tile, skip
                    continue;
                }
                counter->calculate_score(true, false);
                if (!player_result.has_value() || counter->score1 > player_result->counter_.score1) {
                    player_result = FuResult{player_info.player_id_, *counter, kiri_tile.tile_};
                }
            }
            if (player_result.has_value()) {
                fu_results.emplace_back(std::move(*player_result));
            }
        }
        if (fu_results.size() > 1) {
            // more than 1 player lose, we should update the scores
            assert(fu_results.size() <= 3);
            for (auto& player_info : fu_results) {
                if (player_info.counter_.score1 >= 12000) {
                    player_info.counter_.score1 /= fu_results.size();
                } else {
                    player_info.counter_.score1 =
                        (player_info.counter_.fu * pow(2, player_info.counter_.fan + 2) * 6 / fu_results.size() + 90) / 100 * 100; // upper alias to 100
                }
            }
        }
        return fu_results;
    }

    std::vector<FuResult> GetFuResultsForNariRon_() const
    {
        assert(state_ == ActionState::AFTER_KIRI);
        std::vector<FuResult> fu_results;
        for (const auto& player_info : cur_round_kiri_info_.other_players_) {
            for (const auto& kiri_tile : player_info.kiri_tiles_) {
                std::optional<CounterResult> counter = RonCounter_(kiri_tile);
                if (!counter.has_value()) {
                    // cannot ron this tile, skip
                    continue;
                }
                counter->calculate_score(true, true);
                if (fu_results.empty() || counter->score1 > fu_results.begin()->counter_.score1) {
                    fu_results.emplace_back(FuResult::k_tsumo_player_id_, *counter, kiri_tile.tile_);
                }
            }
        }
        return fu_results;
    }

    void InitTable_(Table& table) const
    {
        table.dora_spec = doras_manager_.Doras().size();
        for (auto& [dora, inner_dora] : doras_manager_.Doras()) {
            table.宝牌指示牌.emplace_back(const_cast<Tile*>(&dora));
            table.里宝牌指示牌.emplace_back(const_cast<Tile*>(&inner_dora));
        }
        table.庄家 = player_id_;
        table.last_action = state_ == ActionState::AFTER_KAN || state_ == ActionState::AFTER_KAN_CAN_NARI ? Action::杠 : Action::pass;
        table.players[player_id_].first_round = river_.empty() && furus_.empty();
        table.players[player_id_].riichi = IsRiichi_();
        table.players[player_id_].double_riichi = table.players[player_id_].riichi && is_double_richii_;
        table.players[player_id_].亲家 = true;
        table.players[player_id_].一发 = table.players[player_id_].riichi && richii_round_ + 1 == round_;
        table.players[player_id_].门清 = true;
        for (auto& tile : hand_) {
            table.players[player_id_].hand.emplace_back(const_cast<Tile*>(&tile));
        }
        for (const auto& furu : furus_) {
            Fulu fulu;
            for (int i = 0; i < furu.tiles_.size(); i++) {
                const FuruTile& furu_tile = furu.tiles_[i];
                if (furu_tile.type_ != FuruTile::Type::EMPTY) {
                    fulu.tiles.emplace_back(const_cast<Tile*>(&furu_tile.tile_));
                }
                if (furu_tile.type_ == FuruTile::Type::NARI) {
                    fulu.take = i;
                }
            }
            if (furu.tiles_[3].type_ == FuruTile::Type::EMPTY) {
                if (furu.tiles_[0].tile_.tile == furu.tiles_[1].tile_.tile) {
                    fulu.type = Fulu::Type::Pon;
                } else {
                    fulu.type = Fulu::Type::Chi;

                }
                table.players[player_id_].门清 = false;
            } else {
                fulu.type = Fulu::Type::大明杠;
                table.players[player_id_].门清 = false;
                for (const FuruTile& furu_tile : furu.tiles_) {
                    if (furu_tile.type_ == FuruTile::Type::DARK) {
                        fulu.type = Fulu::Type::暗杠;
                        table.players[player_id_].门清 = true;
                        break;
                    } else if (furu_tile.type_ == FuruTile::Type::ADD_KAN) {
                        fulu.type = Fulu::Type::加杠;
                        break;
                    }
                }
            }
            table.players[player_id_].副露s.emplace_back(std::move(fulu));
        }
    }

    bool DirectKan_(const std::string_view tile_sv) {
        assert(tile_sv.size() <= 3);
        const auto kiri_tile = GetTilesFrom(cur_round_kiri_info_.other_player_kiri_tiles_, tile_sv, errstr_);
        if (kiri_tile.empty()) {
            errstr_ = "前巡舍牌中不存在「"s + tile_sv.data() + "」";
            return false;
        }
        const std::string tile_str_simple = basetile_to_string_simple(kiri_tile.begin()->tile);
        const bool kan_tsumo = MatchTsumo_(tile_sv);
        const auto tiles = kan_tsumo ? GetTilesFrom(hand_, tile_str_simple + tile_str_simple, errstr_)
                                     : GetTilesFrom(hand_, tile_str_simple + tile_str_simple + tile_str_simple, errstr_);
        if (tiles.empty()) {
            cur_round_kiri_info_.other_player_kiri_tiles_.insert(kiri_tile.begin(), kiri_tile.end());
            errstr_ = "您的手牌中不存在足够的「"s + tile_sv.data() + "」";
            return false;
        }
        // kan
        assert(kiri_tile.size() == 1);
        assert(is_杠({tiles.begin()->tile, std::next(tiles.begin())->tile, std::next(tiles.begin(), 2)->tile, kiri_tile.begin()->tile}));
        furus_.emplace_back();
        auto& furu = furus_.back();
        furu.nari_round_ = round_;
        furu.tiles_[0] = FuruTile{*tiles.begin(), FuruTile::Type::NORMAL};
        furu.tiles_[1] = FuruTile{*std::next(tiles.begin()), FuruTile::Type::NORMAL};
        furu.tiles_[2] = FuruTile{kan_tsumo ? *tsumo_ : *std::next(tiles.begin(), 2), FuruTile::Type::NORMAL};
        furu.tiles_[3] = FuruTile{*kiri_tile.begin(), FuruTile::Type::NARI};
        return true;
    }

    std::vector<BaseTile> GetListenTiles_() const
    {
        std::vector<BaseTile> ret;
        std::vector<BaseTile> hand_basetiles;
        for (const Tile& tile : hand_) {
            hand_basetiles.emplace_back(tile.tile);
        }
        if (tsumo_.has_value()) {
            hand_basetiles.emplace_back(tsumo_->tile);
        }
        for (uint8_t basetile = 0; basetile < 9 * 3 + 7; ++basetile) {
            if (4 == std::count(hand_basetiles.begin(), hand_basetiles.end(), basetile)) {
                continue; // all same tiles are in hand
            }
            hand_basetiles.emplace_back(static_cast<BaseTile>(basetile));
            if (is和牌(hand_basetiles)) {
                ret.emplace_back(static_cast<BaseTile>(basetile));
            }
            hand_basetiles.pop_back();
        }
        return ret;
    }

    const SyncMahjongOption& option_;
    const uint32_t player_id_{UINT32_MAX};
    const Wind wind_{Wind::East};
    const std::array<Tile, k_yama_tile_num> yama_;
    int32_t point_variation_{0};

    uint32_t yama_idx_{0};
    TileSet hand_;
    std::optional<Tile> tsumo_;
    std::vector<RiverTile> river_;
    std::vector<Furu> furus_; // do not use array because there may be nuku pei
    bool has_chi_{false};
    ActionState state_{ActionState::ROUND_OVER};

    int32_t round_{0};
    PlayerKiriInfo cur_round_my_kiri_info_;
    KiriInfo cur_round_kiri_info_;

    bool is_double_richii_{false};
    bool is_richii_furutin_{false};
    int32_t richii_round_{0};
    std::vector<BaseTile> richii_listen_tiles_;

    std::vector<FuResult> fu_results_;

    DorasManager doras_manager_;

    std::string errstr_;
};

class SyncMajong
{
  public:
    SyncMajong(const SyncMahjongOption& option) : option_(option), richii_points_(option_.richii_points_)
    {
        std::array<Tile, k_tile_type_num * 4> tiles{BaseTile::_1m};
#ifdef TEST_BOT
        if (const TilesOption* const tiles_option = std::get_if<TilesOption>(&option.tiles_option_)) {
            ShuffleTiles(*tiles_option, tiles);
        } else {
            GenerateTilesFromDecodedString(std::get<std::string>(option.tiles_option_), tiles);
        }
#else
        ShuffleTiles(option.tiles_option_, tiles);
#endif
        AssignTiles_(option, tiles);
        ++round_;
        StartNormalStage_();
    }

    enum class RoundOverResult { CHUTO_NAGASHI_三家和了, CHUTO_NAGASHI_九种九牌, CHUTO_NAGASHI_四风连打, CHUTO_NAGASHI_四家立直, NYANPAI_NAGASHI, RON_ROUND, NORMAL_ROUND, FU };
    RoundOverResult RoundOver() {
        for (auto& player : players_) {
            player.PerformDefault();
        }
        UpdateDora_();
        if (Is_三家和了(players_)) {
            return RoundOverResult::CHUTO_NAGASHI_三家和了;
        }
        if (HandleFuResults_()) {
            return RoundOverResult::FU;
        }
        if (ron_stage_ = !ron_stage_ && StartRonStage_()) {
            return RoundOverResult::RON_ROUND;
        }
        if (round_ == 1 && Is_九种九牌(players_)) {
            return RoundOverResult::CHUTO_NAGASHI_九种九牌;
        }
        for (auto& player : players_) {
            if (player.richii_round_ == round_) {
                player.point_variation_ -= 1000;
                richii_points_ += 1000;
            }
        }
        if (round_ == 1 && Is_四风连打(players_)) {
            return RoundOverResult::CHUTO_NAGASHI_四风连打;
        }
        if (HandleNyanapaiNagashi_(players_)) {
            return RoundOverResult::NYANPAI_NAGASHI;
        }
        ++round_;
        if (Is_四家立直(players_)) {
            return RoundOverResult::CHUTO_NAGASHI_四家立直;
        }
        StartNormalStage_();
        return RoundOverResult::NORMAL_ROUND;
    }

    auto& Players() { return players_; }
    const auto& Players() const { return players_; }

    int32_t Round() const { return round_; }

    int32_t RichiiPoints() const { return richii_points_; }

  private:
    void StartNormalStage_()
    {
        for (auto& player : players_) {
            player.StartNormalStage_(round_);
        }
    }

    [[nodiscard]] bool StartRonStage_()
    {
        std::vector<game_util::mahjong::PlayerKiriInfo> kiri_infos;
        kiri_infos.reserve(players_.size());
        for (const auto& player : players_) {
            kiri_infos.emplace_back(player.cur_round_my_kiri_info_);
        }
        for (auto& player : players_) {
            player.StartRonStage_(kiri_infos);
        }
        return std::ranges::any_of(players_, [](const auto& player) { return player.State() == ActionState::NOTIFIED_RON; });
    }

    void HandleOneFuResult_(const lgtbot::game_util::mahjong::FuResult& fu_result, SyncMahjongGamePlayer& player)
    {
        if (fu_result.player_id_ == game_util::mahjong::FuResult::k_tsumo_player_id_) {
            // tsumo
            const int score_each_player = fu_result.counter_.score1 + option_.benchang_ * 100;
            std::ranges::for_each(players_, [score_each_player](auto& player) { player.point_variation_ -= score_each_player; });
            player.point_variation_ += score_each_player * players_.size();
        } else {
            // ron
            const int score = fu_result.counter_.score1 + option_.benchang_ * (players_.size() - 1) * 100 / player.fu_results_.size();
            players_[fu_result.player_id_].point_variation_ -= score;
            player.point_variation_ += score;
        }
    }

    [[nodiscard]] bool HandleFuResults_()
    {
        bool found_fu = false;
        const uint32_t fu_players_num = std::ranges::count_if(players_, [](const auto& player) { return !player.fu_results_.empty(); });
        for (auto& player : players_) {
            for (const auto& fu_result : player.fu_results_) {
                found_fu = true;
                HandleOneFuResult_(fu_result, player);
                player.point_variation_ += richii_points_ / fu_players_num;
            }
        }
        if (found_fu) {
            richii_points_ = 0;
        }
        return found_fu;
    }

    void UpdateDora_()
    {
        if (std::ranges::any_of(players_, [round = round_](const auto& player) { return player.doras_manager_.HasOpenedThisRound(round); })) {
            std::ranges::for_each(players_, [round = round_](auto& player) { player.doras_manager_.TryOpenNewDora(round); });
        }
    }

    void AssignTiles_(const SyncMahjongOption& option, const std::array<Tile, k_tile_type_num * 4>& tiles)
    {
        size_t tile_idx = 0;
        std::array<std::pair<Tile, Tile>, 4> doras;
        for (auto& [dora, inner_dora] : doras) {
            dora = tiles[tile_idx++];
            inner_dora = tiles[tile_idx++];
        }
        for (uint32_t player_id = 0; player_id < option.player_descs_.size(); ++player_id) {
            std::array<Tile, k_yama_tile_num> yama_tiles;
            TileSet hand;
            for (uint32_t i = 0; i < k_yama_tile_num; ++i) {
                yama_tiles[i] = tiles[tile_idx++];
            }
            for (uint32_t i = 0; i < k_hand_tile_num; ++i) {
                hand.emplace(tiles[tile_idx++]);
            }
            players_.emplace_back(option, player_id, yama_tiles, std::move(hand), doras);
        }
        assert(tile_idx == tiles.size());
    }

    static bool Is_三家和了(const std::vector<SyncMahjongGamePlayer>& players)
    {
        return std::ranges::count_if(players, [](const SyncMahjongGamePlayer& player) { return !player.fu_results_.empty(); }) >= 3;
    }

    static bool Is_四家立直(const std::vector<SyncMahjongGamePlayer>& players)
    {
        return std::ranges::count_if(players, [](const SyncMahjongGamePlayer& player) -> bool { return !player.richii_listen_tiles_.empty(); }) == 4;
    }

    // should only check for the first round
    static bool Is_九种九牌(const std::vector<SyncMahjongGamePlayer>& players)
    {
        return std::ranges::any_of(players, [](const SyncMahjongGamePlayer& player) { return player.river_.empty() && player.fu_results_.empty(); });
    }

    // should only check for the first round
    static bool Is_四风连打(const std::vector<SyncMahjongGamePlayer>& players)
    {
        if (players.size() != 4) {
            return false;
        }
        const BaseTile first_player_kiri_basetile = players.begin()->cur_round_my_kiri_info_.kiri_tiles_.begin()->tile_.tile;
        const auto kiri_basetile_equal = [&](const SyncMahjongGamePlayer& player) -> bool
            {
                const auto& kiri_tiles = player.cur_round_my_kiri_info_.kiri_tiles_;
                return kiri_tiles.size() == 1 && kiri_tiles.begin()->tile_.tile == first_player_kiri_basetile;
            };
        static constexpr const std::array<BaseTile, 4> k_wind_tiles{east, south, west, north};
        return std::ranges::find(k_wind_tiles, first_player_kiri_basetile) != k_wind_tiles.end() &&
            std::all_of(std::next(players.begin()), players.end(), kiri_basetile_equal);
    }

    static bool HandleNagashiManganForNyanpaiNagashi_(std::vector<SyncMahjongGamePlayer>& players)
    {
        uint32_t mangan_player_num = 0;
        for (auto& player : players) {
            if (std::ranges::all_of(player.river_, [](const RiverTile& river_tile)
                        {
                            return std::find(k_one_nine_tiles.begin(), k_one_nine_tiles.end(), river_tile.tile_.tile) != k_one_nine_tiles.end();
                        })) {
                ++mangan_player_num;
                player.point_variation_ += 4000 * players.size();
            }
        }
        if (mangan_player_num == 0) {
            return false;
        }
        for (auto& player : players) {
            player.point_variation_ -= 4000 * mangan_player_num;
        }
        return true;
    }

    static void HandleTinpaiForNyanpaiNagashi_(std::vector<SyncMahjongGamePlayer>& players)
    {
        const int32_t nyanpai_tinpai_points = 1000 * (players.size() - 1);
        const uint32_t tinpai_player_num = std::ranges::count_if(players, [](const auto& player) { return !player.GetListenTiles_().empty(); });
        if (tinpai_player_num == 0 || tinpai_player_num == players.size()) {
            return;
        }
        for (auto& player : players) {
            if (player.GetListenTiles_().empty()) {
                player.point_variation_ -= nyanpai_tinpai_points / (players.size() - tinpai_player_num);
            } else {
                player.point_variation_ += nyanpai_tinpai_points / tinpai_player_num;
            }
        }
    }

    static bool HandleNyanapaiNagashi_(std::vector<SyncMahjongGamePlayer>& players)
    {
        if (std::ranges::none_of(players, [](const SyncMahjongGamePlayer& player) { return player.yama_idx_ == k_yama_tile_num; })) {
            return false;
        }
        if (!HandleNagashiManganForNyanpaiNagashi_(players)) {
            HandleTinpaiForNyanpaiNagashi_(players);
        }
        return true;
    }

    const SyncMahjongOption& option_;
    int32_t round_{0};
    bool ron_stage_{false};
    std::vector<SyncMahjongGamePlayer> players_;
    uint32_t dora_num_{1};
    int32_t richii_points_{0};

};

} // namespace mahjong

} // namespace game_util

} // namespace lgtbot
  // TODO:
  // 显示可用舍牌
  // 显示具体可执行操作
  // 立直后自动摸切
  // 三麻
  //
  // 测试各种振听
  // 立直之后杠
  // 多人放铳,其中一人无役
  // 最后一巡禁止立直
  // 测试岭上
  // 测试海底
  // 测试河底
