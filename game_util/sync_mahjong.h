// Copyright (c) 2024-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include "game_util/mahjong_util.h"

#include "Mahjong/Rule.h"

namespace lgtbot {

namespace game_util {

namespace mahjong {

static constexpr const uint32_t k_yama_tile_num_ = 19;

struct RiverTile
{
    uint32_t kiri_round_;
    uint32_t is_tsumo_kiri_;
    Tile tile_;
    bool richii_;
};

struct KiriTile
{
    Tile tile_;
    enum class Type : uint8_t { NORMAL, ADD_KAN, DARK_KAN, RIVER_BOTTOM } type_;
};

struct PlayerKiriInfo
{
    uint32_t player_id_;
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
    std::array<FuruTile, 4> tiles_;
};

enum class ActionState
{
    FIRST_ROUND_CAN_NAGASHI_BEGIN,
    ROUND_BEGIN, // nari, get tile
    AFTER_NARI, // kiri
    AFTER_GET_TILE, // kiri, kan, richii (if has no furu)
    AFTER_KAN, // kiri, kan, richii (if has no furu)
    AFTER_KIRI, // nari, over
    ROUND_OVER,
    RICHII_PREPARE,
    RICHII_FIRST_ROUND,
    RICHII,
    FU,
};

class Mahjong
{
  private:
    enum ActionType { GET_TILE, KIRI, FURU };
    // 摸牌
    //
    // 吃 35s 4s
    // 碰 5s (优先副露透明牌) // 碰 5t5s
    // 杠 5s
    //
    // 3s // 切 3s (可杠可切的时候) // 立直 3s
    //
    // 自摸
    // 荣
    //
    // 流局 (九种九牌)

    uint32_t richii_points_;
};

struct GamePlayer
{
    GamePlayer(const uint32_t player_id, const Wind wind, const bool can_richii, const std::array<Tile, k_yama_tile_num_>& yama, TileSet hand)
        : player_id_(player_id), wind_(wind), can_richii_(can_richii), yama_(yama), hand_(std::move(hand)) {}

    ActionState State() const { return state_; }

    const PlayerKiriInfo& CurrentRoundKiriInfo() const { return cur_round_my_kiri_info_; }

    void StartRound(const std::vector<PlayerKiriInfo>& kiri_infos)
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
        cur_round_my_kiri_info_.kiri_tiles_.clear();
        if (river_.empty() && CanNagashi_()) {
            state_ = ActionState::FIRST_ROUND_CAN_NAGASHI_BEGIN;
        } else if (state_ == ActionState::RICHII_PREPARE) {
            state_ = ActionState::RICHII_FIRST_ROUND;
        } else if (state_ == ActionState::RICHII_FIRST_ROUND) {
            state_ = ActionState::RICHII;
        } else if (state_ == ActionState::ROUND_OVER) {
            state_ = ActionState::ROUND_BEGIN;
        } else {
            assert(river_.empty());
        }
    }

    bool GetTile()
    {
        if (state_ != ActionState::ROUND_BEGIN || state_ != ActionState::FIRST_ROUND_CAN_NAGASHI_BEGIN) {
            // invalid action
            return false;
        }
        if (yama_idx_ == k_yama_tile_num_) {
            // no more tiles
            return false;
        }
        tsumo_ = yama_[yama_idx_++];
        state_ = ActionState::AFTER_GET_TILE;
        return true;
    }

    bool CanNagashi_() const
    {
        static constexpr const uint32_t k_tile_type_num = 34;
        std::array<bool, k_tile_type_num> basetile_bitset{false};
        for (const Tile& tile : hand_) {
            basetile_bitset[tile.tile] = true;
        }
        return std::ranges::count_if(std::array{_1m, _9m, _1s, _9s, _1p, _9p, east, south, west, north, 白, 发, 中},
                [&](const BaseTile basetile) { return basetile_bitset[basetile]; }) >= 9;
    }

    bool Nagashi() const
    {
        return state_ == ActionState::FIRST_ROUND_CAN_NAGASHI_BEGIN;
    }

    bool Kiri(const std::string_view tile, const bool richii, const int32_t round)
    {
        if (state_ != ActionState::AFTER_NARI && state_ != ActionState::AFTER_GET_TILE && state_ != ActionState::AFTER_KAN) {
            return false;
        }
        if (richii && !furus_.empty()) {
            return false;
        }
        if (richii && !can_richii_) {
            return false;
        }
        const auto kiri_type = yama_idx_ == k_yama_tile_num_ ? KiriTile::Type::RIVER_BOTTOM : KiriTile::Type::NORMAL;
        if (tile.empty()) {
            if (!tsumo_.has_value()) {
                // no tsumo tile
                return false;
            }
            river_.emplace_back(round, true, *tsumo_, richii);
            cur_round_my_kiri_info_.kiri_tiles_.emplace_back(KiriTile{*tsumo_, kiri_type});
            tsumo_ = std::nullopt;
            return true;
        }
        const auto tiles = GetTilesFrom(hand_, tile, errstr_);
        if (tiles.empty()) {
            return false;
        }
        if (tiles.size() != 1) {
            errstr_ = "您只能切出一张牌";
            hand_.insert(tiles.begin(), tiles.end());
            return false;
        }
        if (river_.empty() && furus_.empty() && richii) {
            is_double_richii_ = true;
        }
        river_.emplace_back(round, false, *tiles.begin(), richii);
        cur_round_my_kiri_info_.kiri_tiles_.emplace_back(KiriTile{*tiles.begin(), kiri_type});
        if (tsumo_.has_value()) {
            hand_.emplace(*tsumo_);
            tsumo_ = std::nullopt;
        }
        // TODO: 双立直/四风连打
        state_ = richii                            ? ActionState::RICHII_PREPARE :
                 state_ == ActionState::AFTER_NARI ? ActionState::AFTER_KIRI     : ActionState::ROUND_OVER;
        return true;
    }

    bool Chi(const std::string_view hand_tiles, const std::string_view others_kiri_tile)
    {
        if (state_ != ActionState::ROUND_BEGIN && state_ != ActionState::AFTER_KIRI) {
            return false;
        }
        const auto tiles = GetTilesFrom(hand_, hand_tiles, errstr_);
        const auto kiri_tile = GetTilesFrom(cur_round_kiri_info_.other_player_kiri_tiles_, others_kiri_tile, errstr_);
        if (tiles.empty() || kiri_tile.empty()) {
            return false;
        }
        const auto rollback =
            [&] {
                hand_.insert(tiles.begin(), tiles.end());
                cur_round_kiri_info_.other_player_kiri_tiles_.insert(kiri_tile.begin(), kiri_tile.end());
            };
        if (tiles.size() != 2) {
            errstr_ = std::string("您需要从手牌中指定 2 张牌，目前指定了 ") + hand_tiles.data() + " 共 " + std::to_string(tiles.size()) + " 张牌";
            rollback();
            return false;
        }
        if (kiri_tile.size() != 1) {
            errstr_ = std::string("您需要从前巡舍牌中指定 1 张牌，目前指定了 ") + others_kiri_tile.data() + " 共 " + std::to_string(kiri_tile.size()) + " 张牌";
            rollback();
            return false;
        }
        if (!is_顺子({tiles.begin()->tile, std::next(tiles.begin())->tile, kiri_tile.begin()->tile})) {
            errstr_ = "这三张牌无法形成顺子";
            rollback();
            return false;
        }
        furus_.emplace_back();
        auto& furu = furus_.back();
        furu.tiles_[0] = FuruTile{*tiles.begin(), FuruTile::Type::NORMAL};
        furu.tiles_[1] = FuruTile{*std::next(tiles.begin()), FuruTile::Type::NORMAL};
        furu.tiles_[2] = FuruTile{*kiri_tile.begin(), FuruTile::Type::NARI};
        std::sort(furu.tiles_.begin(), furu.tiles_.begin() + 3,
                [](const auto& _1, const auto& _2) { return _1.tile_.tile < _2.tile_.tile; });
        min_ron_yaku_++;
        state_ = ActionState::AFTER_NARI;
        return true;
    }

    bool Pon(const std::string_view hand_tiles)
    {
        if (state_ != ActionState::ROUND_BEGIN && state_ != ActionState::AFTER_KIRI) {
            return false;
        }
        // If player only specifies one tile, we should repeat the tile string to specify two tiles.
        const auto tiles = hand_tiles.size() > 3 ?
            GetTilesFrom(hand_, hand_tiles, errstr_) :
            GetTilesFrom(hand_, std::string(hand_tiles) + std::string(hand_tiles), errstr_);
        if (tiles.empty()) {
            return false;
        }
        const auto rollback = [&] { hand_.insert(tiles.begin(), tiles.end()); };
        if (tiles.size() != 2) {
            errstr_ = std::string("您需要从手牌中指定 2 张牌，目前指定了 ") + hand_tiles.data() + " 共 " + std::to_string(tiles.size()) + " 张牌";
            rollback();
            return false;
        }
        const auto kiri_tile = GetTilesFrom(cur_round_kiri_info_.other_player_kiri_tiles_, basetile_to_string_simple(tiles.begin()->tile), errstr_);
        if (tiles.empty()) {
            return false;
        }
        assert(kiri_tile.size() == 1);
        assert(is_刻子({tiles.begin()->tile, std::next(tiles.begin())->tile, kiri_tile.begin()->tile}));
        furus_.emplace_back();
        auto& furu = furus_.back();
        furu.tiles_[0] = FuruTile{*tiles.begin(), FuruTile::Type::NORMAL};
        furu.tiles_[1] = FuruTile{*std::next(tiles.begin()), FuruTile::Type::NORMAL};
        furu.tiles_[2] = FuruTile{*kiri_tile.begin(), FuruTile::Type::NARI};
        state_ = ActionState::AFTER_NARI;
        return true;
    }

    bool DirectKan_(const std::string_view tile_sv) {
        const auto kiri_tile = GetTilesFrom(cur_round_kiri_info_.other_player_kiri_tiles_, tile_sv, errstr_);
        if (kiri_tile.empty()) {
            return false;
        }
        const std::string tile_str_simple = basetile_to_string_simple(kiri_tile.begin()->tile);
        const auto tiles = GetTilesFrom(hand_, tile_str_simple + tile_str_simple + tile_str_simple, errstr_);
        if (tiles.empty()) {
            cur_round_kiri_info_.other_player_kiri_tiles_.insert(kiri_tile.begin(), kiri_tile.end());
            return false;
        }
        // kan
        assert(kiri_tile.size() == 1);
        assert(is_杠({tiles.begin()->tile, std::next(tiles.begin())->tile, std::next(tiles.begin(), 2)->tile, kiri_tile.begin()->tile}));
        furus_.emplace_back();
        auto& furu = furus_.back();
        furu.tiles_[0] = FuruTile{*tiles.begin(), FuruTile::Type::NORMAL};
        furu.tiles_[1] = FuruTile{*std::next(tiles.begin()), FuruTile::Type::NORMAL};
        furu.tiles_[2] = FuruTile{*std::next(tiles.begin(), 2), FuruTile::Type::NORMAL};
        furu.tiles_[3] = FuruTile{*kiri_tile.begin(), FuruTile::Type::NARI};
        return true;
    }

    std::set<BaseTile> GetListenTiles_(const TileSet& hand_tiles)
    {
        std::set<BaseTile> ret;
        std::vector<BaseTile> hand_basetiles;
        for (const Tile& tile : hand_tiles) {
            hand_basetiles.emplace_back(tile.tile);
        }
        for (uint8_t basetile = 0; basetile < 9 * 3 + 7; ++basetile) {
            if (4 == std::count(hand_basetiles.begin(), hand_basetiles.end(), basetile)) {
                continue; // all same tiles are in hand
            }
            hand_basetiles.emplace_back(static_cast<BaseTile>(basetile));
            if (is和牌(hand_basetiles)) {
                ret.emplace(static_cast<BaseTile>(basetile));
            }
            hand_basetiles.pop_back();
        }
        return ret;
    }

    bool DarkKanOrAddKan_(const std::string_view tile_sv) {
        const bool is_richii_state = state_ == ActionState::RICHII_FIRST_ROUND || state_ == ActionState::RICHII;
        const std::string decoded_tile_str = DecodeTilesString(tile_sv, errstr_);
        if (decoded_tile_str.size() != 2) {
            return false;
        }
        const bool kan_tsumo = static_cast<Tile>(TileIdent{decoded_tile_str[0], decoded_tile_str[1]}).tile == tsumo_->tile;
        if (is_richii_state && !kan_tsumo) {
            return false;
        }
        const auto handle_tsumo = [&]() {
            if (!kan_tsumo) {
                hand_.emplace(*tsumo_);
            }
            tsumo_ = std::nullopt;
        };
        std::set<BaseTile> listen_tiles_before_kan;
        if (is_richii_state) {
            listen_tiles_before_kan = GetListenTiles_(hand_);
            assert(!listen_tiles_before_kan.empty());
        }
        const std::string tile_str(tile_sv);
        const auto hand_tiles = kan_tsumo ? GetTilesFrom(hand_, tile_str + tile_str + tile_str, errstr_)
                                            : GetTilesFrom(hand_, tile_str + tile_str + tile_str + tile_str, errstr_);
        if (!hand_tiles.empty()) {
            // dark kan
            assert(is_杠({hand_tiles.begin()->tile, std::next(hand_tiles.begin())->tile,
                        std::next(hand_tiles.begin(), 2)->tile,
                        kan_tsumo ? tsumo_->tile : std::next(hand_tiles.begin(), 3)->tile}));
            if (is_richii_state && GetListenTiles_(hand_) != listen_tiles_before_kan) {
                // kan after richii cannot change tinpai
                hand_.insert(hand_tiles.begin(), hand_tiles.end());
                return false;
            }
            furus_.emplace_back();
            auto& furu = furus_.back();
            furu.tiles_[0] = FuruTile{*hand_tiles.begin(), FuruTile::Type::DARK};
            furu.tiles_[1] = FuruTile{*std::next(hand_tiles.begin()), FuruTile::Type::NORMAL};
            furu.tiles_[2] = FuruTile{*std::next(hand_tiles.begin(), 2), FuruTile::Type::NORMAL};
            furu.tiles_[3] = FuruTile{kan_tsumo ? *tsumo_ : *std::next(hand_tiles.begin(), 3), FuruTile::Type::DARK};
            handle_tsumo();
            return true;
        }
        const auto one_hand_tile = GetTilesFrom(hand_, tile_str, errstr_);
        if (one_hand_tile.empty()) {
            return false;
        }
        assert(one_hand_tile.size() == 1);
        bool found = false;
        for (auto& furu : furus_) {
            if (furu.tiles_[0].tile_.tile == one_hand_tile.begin()->tile && furu.tiles_[1].tile_.tile == one_hand_tile.begin()->tile) {
                // add kan
                assert(furu.tiles_[3].type_ == FuruTile::Type::EMPTY);
                furu.tiles_[3].type_ = FuruTile::Type::ADD_KAN;
                furu.tiles_[3].tile_ = *one_hand_tile.begin();
                handle_tsumo();
                found = true;
                break;
            }
        }
        if (!found) {
            hand_.insert(one_hand_tile.begin(), one_hand_tile.end());
        }
        return found;
    }

    bool Kan(const std::string_view tile_sv)
    {
        if (tile_sv.size() > 3) {
            errstr_ = "您需要且仅需要指定一张牌";
            return false;
        }
        if (yama_idx_ == k_yama_tile_num_) {
            // no more tiles
            return false;
        }
        bool succ = false;
        if (state_ == ActionState::ROUND_BEGIN || state_ == ActionState::AFTER_KIRI) {
            succ = DirectKan_(tile_sv);
        } else if (state_ == ActionState::AFTER_GET_TILE || state_ == ActionState::AFTER_KAN || state_ == ActionState::RICHII_FIRST_ROUND || state_ == ActionState::RICHII) {
            succ = DarkKanOrAddKan_(tile_sv);
        } else {
            // invalid state
            return false;
        }
        if (succ) {
            tsumo_ = yama_[yama_idx_++];
            state_ = ActionState::AFTER_KAN;
        }
        return false;
    }

    enum class FuType { TSUMO, ROB_KAN, RON };

    std::optional<CounterResult> GetCounter_(const Tile& tile, const FuType fu_type, const bool is_last_tile, const std::vector<std::pair<Tile, Tile>>& doras) const
    {
        Table table;
        InitTable_(table, doras);
        auto basetiles = convert_tiles_to_base_tiles(table.players[player_id_].hand);
        basetiles.emplace_back(tile.tile);
        if (!is和牌(basetiles)) {
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
            return std::nullopt;
        }
        auto fan = counter.fan;
        for (auto& yaku : counter.yakus) {
            if (yaku == Yaku::宝牌 || yaku == Yaku::里宝牌 || yaku == Yaku::赤宝牌 || yaku == Yaku::北宝牌) {
                --fan;
            }
            // TODO: do not show in html
            if (yaku == Yaku::场风_东 || yaku == Yaku::场风_北 || yaku == Yaku::场风_南 || yaku == Yaku::场风_西) {
                --counter.fan;
            }
        }
        if (fan < min_ron_yaku_) {
            return std::nullopt;
        }
        if (std::ranges::any_of(counter.yakus,
                    [](const Yaku yaku) { return yaku > Yaku::满贯 && yaku < Yaku::双倍役满; })) {
            std::erase_if(counter.yakus, [](const Yaku yaku) { return yaku < Yaku::满贯; });
        }
        return counter;
    }

    std::optional<CounterResult> Tsumo(const std::vector<std::pair<Tile, Tile>>& doras)
    {
        if (state_ != ActionState::AFTER_GET_TILE && state_ != ActionState::AFTER_KAN && state_ != ActionState::RICHII_FIRST_ROUND && state_ != ActionState::RICHII) {
            return std::nullopt;
        }
        assert(tsumo_.has_value());
        std::optional<CounterResult> counter = GetCounter_(*tsumo_, FuType::TSUMO, yama_idx_ == k_yama_tile_num_, doras);
        if (counter.has_value()) {
            counter->calculate_score(true, true);
        }
        if (counter.has_value()) {
            state_ = ActionState::FU;
        }
        return counter;
    }

    struct RonResult {
        uint32_t player_id_;
        CounterResult counter_;
        Tile tile_;
    };

    std::optional<CounterResult> RonCounter_(const KiriTile& kiri_tile, const std::vector<std::pair<Tile, Tile>>& doras) const
    {
        // `other_player_kiri_tiles_` does not contain add kan or dark kan tiles
        if (kiri_tile.type_ != KiriTile::Type::ADD_KAN && kiri_tile.type_ != KiriTile::Type::DARK_KAN &&
                cur_round_kiri_info_.other_player_kiri_tiles_.find(kiri_tile.tile_) == cur_round_kiri_info_.other_player_kiri_tiles_.end()) {
            // this tile has been used, skip
            return std::nullopt;
        }
        const FuType fu_type =
            kiri_tile.type_ == KiriTile::Type::ADD_KAN || kiri_tile.type_ == KiriTile::Type::DARK_KAN ? FuType::ROB_KAN : FuType::RON;
        const std::optional<CounterResult> counter = GetCounter_(kiri_tile.tile_, fu_type, kiri_tile.type_ == KiriTile::Type::RIVER_BOTTOM, doras);
        if (!counter.has_value()) {
            return std::nullopt;
        }
        if (kiri_tile.type_ == KiriTile::Type::DARK_KAN &&
                std::ranges::none_of(counter->yakus, [](const Yaku yaku) { return yaku == Yaku::国士无双; })) {
            return std::nullopt;
        }
        // TODO: check 振听
    }

    bool CanRon() const
    {
        for (const auto& player_info : cur_round_kiri_info_.other_players_) {
            for (const auto& kiri_tile : player_info.kiri_tiles_) {
                if (RonCounter_(kiri_tile, {}).has_value()) {
                    return true;
                }
            }
        }
        return false;
    }

    std::vector<RonResult> Ron(const std::vector<std::pair<Tile, Tile>>& doras)
    {
        std::vector<RonResult> result;
        for (const auto& player_info : cur_round_kiri_info_.other_players_) {
            std::optional<RonResult> player_result;
            for (const auto& kiri_tile : player_info.kiri_tiles_) {
                std::optional<CounterResult> counter = RonCounter_(kiri_tile, doras);
                if (!counter.has_value()) {
                    // cannot ron this tile, skip
                    continue;
                }
                counter->calculate_score(true, false);
                if (!player_result.has_value() || counter->score1 > player_result->counter_.score1) {
                    player_result = RonResult{player_info.player_id_, *counter, kiri_tile.tile_};
                }
            }
            if (player_result.has_value()) {
                result.emplace_back(std::move(*player_result));
            }
        }
        if (result.size() > 1) {
            // more than 1 player lose, we should update the scores
            assert(result.size() <= 3);
            for (auto& player_info : result) {
                player_info.counter_.score1 =
                    (player_info.counter_.fu * pow(2, player_info.counter_.fan + 2) * 6 / result.size() + 90) / 100 * 100; // upper alias to 100
            }
        }
        if (result.empty()) {
            state_ = ActionState::FU;
        }
        return result;
    }

    // return the score each player should pay
    struct NariResult
    {
        CounterResult counter_;
        Tile tile_;
    };

    std::optional<NariResult> NariRon(const std::vector<std::pair<Tile, Tile>>& doras)
    {
        if (state_ != ActionState::AFTER_KIRI) {
            return std::nullopt;
        }
        std::optional<NariResult> result;
        for (const auto& player_info : cur_round_kiri_info_.other_players_) {
            for (const auto& kiri_tile : player_info.kiri_tiles_) {
                std::optional<CounterResult> counter = RonCounter_(kiri_tile, doras);
                if (!counter.has_value()) {
                    // cannot ron this tile, skip
                    continue;
                }
                counter->calculate_score(true, true);
                if (!result.has_value() || counter->score1 > result->counter_.score1) {
                    result = NariResult{*counter, kiri_tile.tile_};
                }
            }
        }
        if (result.has_value()) {
            state_ = ActionState::FU;
        }
        return result;
    }

    void InitTable_(Table& table, const std::vector<std::pair<Tile, Tile>>& doras) const
    {
        table.dora_spec = doras.size();
        for (auto& [dora, inner_dora] : doras) {
            table.宝牌指示牌.emplace_back(const_cast<Tile*>(&dora));
            table.里宝牌指示牌.emplace_back(const_cast<Tile*>(&inner_dora));
        }
        table.庄家 = player_id_;
        table.last_action = state_ == ActionState::AFTER_KAN ? Action::杠 : Action::pass;
        table.players[player_id_].first_round = river_.empty() && furus_.empty();
        table.players[player_id_].riichi = state_ == ActionState::RICHII_FIRST_ROUND || state_ == ActionState::RICHII;
        table.players[player_id_].double_riichi = is_double_richii_;
        table.players[player_id_].亲家 = true;
        table.players[player_id_].一发 = state_ == ActionState::RICHII_FIRST_ROUND;
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
            } else {
                fulu.type = Fulu::Type::大明杠;
                for (const FuruTile& furu_tile : furu.tiles_) {
                    if (furu_tile.type_ == FuruTile::Type::DARK) {
                        fulu.type = Fulu::Type::暗杠;
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

    static std::string RiverHtml_(const std::string& image_path, const std::vector<RiverTile>& river)
    {
        if (river.empty()) {
            return "";
        }
        html::Table table(2, 0);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        table.SetRowStyle(" align=\"left\" ");
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

    static std::string FuruHtml_(const std::string& image_path, const Furu& furu)
    {
        html::Table table(2, 4);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        for (uint32_t i = 0; i < furu.tiles_.size(); ++i) {
            const FuruTile& furu_tile = furu.tiles_[i];
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
                    table.Get(0, i).SetContent(furu_tile.tile_.to_simple_string()); // TODO: use image
                    break;
                case FuruTile::Type::ADD_KAN:
                    table.Get(1, i - 1).SetContent(Image(image_path, furu_tile.tile_, TileStyle::FORWARD));
                    break;
                default:
                    assert(false);
            }
        }
        return table.ToString();
    }

    const uint32_t player_id_{UINT32_MAX};
    const Wind wind_{Wind::East};
    const bool can_richii_; // player cannot richii when the point is less than 1000
    const std::array<Tile, k_yama_tile_num_> yama_;
    uint32_t yama_idx_{0};
    TileSet hand_;
    std::optional<Tile> tsumo_;
    std::vector<RiverTile> river_;
    std::vector<Furu> furus_; // do not use array because there may be nuku pei
    uint32_t min_ron_yaku_{1};

    PlayerKiriInfo cur_round_my_kiri_info_;
    KiriInfo cur_round_kiri_info_;

    bool is_double_richii_{false};
    bool furutin_{false};
    ActionState state_{ActionState::ROUND_BEGIN};

    std::string errstr_;
};

inline bool CheckNagashi(const std::vector<GamePlayer>& players, const int32_t round) {
    assert(players.size() >= 2);
    static const auto is_richii = [](const GamePlayer& player) -> bool
        {
            return player.State() == ActionState::RICHII_FIRST_ROUND || player.State() == ActionState::RICHII;
        };
    static const auto is_nagashi = [](const GamePlayer& player) -> bool
        {
            return player.State() == ActionState::FIRST_ROUND_CAN_NAGASHI_BEGIN;
        };
    static const auto is_fu = [](const GamePlayer& player) -> bool
        {
            return player.State() == ActionState::FU;
        };
    const auto fu_count = std::ranges::count_if(players, is_fu);
    if (fu_count == 1 || fu_count == 2) {
        return false; // fu has higher priority
    }
    if (fu_count == 3) {
        return true; // 三家和了
    }
    if (players.size() == 4) {
        const auto kiri_basetile_equal = [&](const GamePlayer& player) -> bool
            {
                const auto& kiri_tiles = player.CurrentRoundKiriInfo().kiri_tiles_;
                assert(kiri_tiles.size() == 1);
                return kiri_tiles.begin()->tile_.tile == kiri_tiles.begin()->tile_.tile;
            };
        if (std::ranges::count_if(players, is_richii) == 4 ||
                (round == 1 && std::all_of(std::next(players.begin()), players.end(), kiri_basetile_equal))) {
            return true; // 四家立直 / 四风连打
        }
    }
    return round == 1 && std::ranges::any_of(players, is_nagashi); // 九种九牌
}

} // namespace mahjong

} // namespace game_util

} // namespace lgtbot
