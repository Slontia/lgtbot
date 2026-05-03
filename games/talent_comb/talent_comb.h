// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).
//
// Aggregator header for the talent_comb game. Splits:
//   - talent.h : SpecialEvent, Talent enum, TalentInfo table, grade helpers
//   - This file: board primitives (Direct/Coordinate/AreaCard/Wall/Area/LineDefinition/ScoreResult),
//                TalentComb board class, GetStyle()
//   - player.h : Player struct (uses types defined above in this file)

#pragma once

#include "utility/random.h"
#include "talent.h"
// Note: "player.h" is included at the bottom of this file because
// Player uses AreaCard / TalentComb / k_all_lines which are defined in this file.

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class TalentComb;  // forward-declare (Wall/Area befriend it)

// ==================== Direction System ====================

static constexpr uint32_t k_direct_max = 3;
enum class Direct { 左上 = 0, 垂直 = 1, 右上 = 2 };

struct Coordinate
{
    Coordinate& operator+=(const Coordinate& c) { x_ += c.x_; y_ += c.y_; return *this; }
    friend Coordinate operator+(const Coordinate& _1, const Coordinate& _2) { Coordinate tmp(_1); return tmp += _2; }
    Coordinate operator-() const { return Coordinate{-x_, -y_}; }
    auto operator<=>(const Coordinate&) const = default;
    int32_t x_;
    int32_t y_;
};

template <Direct direct> extern const Coordinate k_direct_step;

template <> inline const Coordinate k_direct_step<Direct::左上>{1, 1};
template <> inline const Coordinate k_direct_step<Direct::垂直>{0, 2};
template <> inline const Coordinate k_direct_step<Direct::右上>{-1, 1};

// ==================== AreaCard ====================

class AreaCard
{
  public:
    AreaCard() : points_{10, 10, 10} {} // wild card (癞子/万能牌) = 10,10,10

    AreaCard(const int32_t a, const int32_t b, const int32_t c)
        : points_{a, b, c} {}

    template <Direct direct>
    bool IsMatch(const int32_t point) const { return points_[static_cast<uint32_t>(direct)] == 10 || points_[static_cast<uint32_t>(direct)] == point; }

    template <Direct direct>
    std::optional<int32_t> Point() const
    {
        if (IsWild()) return std::nullopt;
        return points_[static_cast<uint32_t>(direct)];
    }

    int32_t PointAt(const uint32_t dir) const { return points_[dir]; }

    int32_t PointSum() const { return std::accumulate(points_.begin(), points_.end(), 0); }

    bool IsWild() const { return points_[0] == 10 && points_[1] == 10 && points_[2] == 10; }
    bool IsZero() const { return points_[0] == 0 && points_[1] == 0 && points_[2] == 0; }

    void ApplyRetreatingAdvance()
    {
        for (auto& point : points_) {
            if (point == 4) point = 3;
            else if (point == 7) point = 6;
        }
    }

    void ApplyPerfectBlock()
    {
        if (IsWild()) return;
        if ((points_[0] == 3 || points_[0] == 10) &&
            (points_[1] == 1 || points_[1] == 10) &&
            (points_[2] == 2 || points_[2] == 10)) {
            points_ = {10, 10, 10};
        }
    }

    void ApplyDigitReverse()
    {
        for (auto& point : points_) {
            if (point == 1) point = 9;
            else if (point == 9) point = 1;
        }
    }

    void ApplyNineAsWild()
    {
        for (auto& point : points_) {
            if (point == 9) point = 10;
        }
    }

    void ApplyThreeToFour()
    {
        for (auto& point : points_) {
            if (point == 3) point = 4;
        }
    }

    void ApplyThreeSixToFourSeven()
    {
        for (auto& point : points_) {
            if (point == 3) point = 4;
            else if (point == 6) point = 7;
        }
    }

    void SetDirectionWild(const int32_t dir)
    {
        if (dir >= 0 && dir < static_cast<int32_t>(k_direct_max)) {
            points_[dir] = 10;
        }
    }

    // HTML rendering: card.png base + 3 directional number overlays
    std::string ToHtml(const std::string& image_path) const
    {
        std::string div = "<div class=\"brick\"><img src=\"file:///" + image_path + "card.png\">";
        div += "<img src=\"file:///" + image_path + ImageNameForDirect_(Direct::垂直) + ".png\">";
        div += "<img src=\"file:///" + image_path + ImageNameForDirect_(Direct::右上) + ".png\">";
        div += "<img src=\"file:///" + image_path + ImageNameForDirect_(Direct::左上) + ".png\">";
        div += "</div>";
        return div;
    }

    // Display name for broadcast messages
    std::string CardName() const
    {
        std::string name = "card_";
        for (int i = 0; i < static_cast<int>(k_direct_max); ++i) {
            if (points_[i] == 10) {
                name += "X";
            } else {
                name += std::to_string(points_[i]);
            }
        }
        return name;
    }

    bool operator==(const AreaCard& other) const { return points_ == other.points_; }

  private:
    std::string ImageNameForDirect_(Direct direct) const
    {
        int32_t val = points_[static_cast<uint32_t>(direct)];
        if (val == 10) {
            switch (direct) {
                case Direct::垂直: return "Xv";
                case Direct::左上: return "Xl";
                case Direct::右上: return "Xr";
            }
            return "";
        }
        if (val == 0) {
            switch (direct) {
                case Direct::垂直: return "0v";
                case Direct::左上: return "0l";
                case Direct::右上: return "0r";
            }
            return "";
        }
        return std::to_string(val);
    }

    std::array<int32_t, k_direct_max> points_;
};

// ==================== Wall ====================

class Wall
{
  public:
    friend class TalentComb;

    Wall(html::Box& box) : box_(box), has_line_{false, false, false} {}

    template <Direct direct>
    void SetLine() { has_line_.at(static_cast<uint32_t>(direct)) = true; }

    template <Direct direct>
    void ClearLine() { has_line_.at(static_cast<uint32_t>(direct)) = false; }

    void ClearAllLines() { has_line_ = {false, false, false}; }

    std::string ImageName() const
    {
        std::string str = "wall_";
        for (const bool has_line : has_line_) {
            str += std::to_string(has_line);
        }
        return str;
    }

  private:
    html::Box& box_;
    std::array<bool, k_direct_max> has_line_;
};

// ==================== Area ====================

class Area
{
  public:
    friend class TalentComb;

    Area(html::Box& box, const Coordinate coordinate) : box_(box), coordinate_(coordinate) {}

  private:
    html::Box& box_;
    Coordinate coordinate_;
    std::optional<AreaCard> card_;
};

// ==================== Line Definition ====================

struct LineDefinition
{
    Direct direction;
    std::vector<uint32_t> positions; // area indices (1-19)
};

// Precomputed lines for the 19-cell hex board (positions 1-19)
// Direction 0 (TOP_LEFT): diagonals grouped by (y-x)
// Direction 1 (VERT): columns grouped by x
// Direction 2 (TOP_RIGHT): diagonals grouped by (x+y)
static const std::vector<LineDefinition> k_all_lines = {
    // TOP_LEFT lines
    {Direct::左上, {8, 13, 17}},
    {Direct::左上, {4, 9, 14, 18}},
    {Direct::左上, {1, 5, 10, 15, 19}},
    {Direct::左上, {2, 6, 11, 16}},
    {Direct::左上, {3, 7, 12}},
    // VERT lines
    {Direct::垂直, {1, 2, 3}},
    {Direct::垂直, {4, 5, 6, 7}},
    {Direct::垂直, {8, 9, 10, 11, 12}},
    {Direct::垂直, {13, 14, 15, 16}},
    {Direct::垂直, {17, 18, 19}},
    // TOP_RIGHT lines
    {Direct::右上, {1, 4, 8}},
    {Direct::右上, {2, 5, 9, 13}},
    {Direct::右上, {3, 6, 10, 14, 17}},
    {Direct::右上, {7, 11, 15, 18}},
    {Direct::右上, {12, 16, 19}},
};

// ==================== Score Result ====================

struct ScoreResult
{
    int32_t base_score;       // Total base score from completed lines
    uint32_t line_count;      // Number of completed lines
    int32_t score_delta;      // Change in base score from previous
};

// ==================== TalentComb Board ====================

class TalentComb
{
  public:
    TalentComb(std::string image_path) : image_path_(std::move(image_path)), table_(k_max_row, k_max_column)
    {
        table_.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");

        static constexpr int32_t k_zero_row = 0;
        static constexpr int32_t k_zero_col = k_size + 2;
        static constexpr int32_t k_mid_row = k_size * 2;
        static constexpr int32_t k_mid_col = k_size;

        static const Coordinate zero_coor{k_zero_col - k_mid_col, k_zero_row - k_mid_row};

        // Do NOT add the zero area as playable - it stays as a wall
        // Add a placeholder at index 0 (never used for gameplay)
        html::Box& zero_box = table_.Get(k_zero_row, k_zero_col);
        areas_.emplace_back(zero_box, zero_coor); // placeholder at index 0

        for (int32_t col = 0; col < table_.Column(); ++col) {
            for (int32_t row = 0; row < table_.Row(); ++row) {
                const Coordinate coor{col - k_mid_col, row - k_mid_row};
                const bool is_full_box = (coor.x_ + coor.y_) % 2 == 0 && row != table_.Row() - 1;
                if (is_full_box) {
                    table_.MergeDown(row, col, 2);
                }
                html::Box& box = table_.Get(row, col);
                if (IsValid_(coor) && is_full_box) {
                    box.SetContent(Image_("num_" + std::to_string(areas_.size())));
                    areas_.emplace_back(box, coor);
                } else if (is_full_box) {
                    const auto [it, succ] = walls_.emplace(coor, Wall(box));
                    assert(succ);
                    box.SetContent(Image_(it->second.ImageName()));
                } else if (box.IsVisable()) {
                    box.SetContent(Image_("wall_half"));
                }
            }
        }

        // Keep the wall at zero_coor (do NOT erase it)
        // The zero_box content stays as the wall image set during the loop
    }

    TalentComb(const TalentComb&) = delete;
    TalentComb(TalentComb&&) = delete;

    // Place a card at position idx (1-19). Supports overwriting.
    // Returns the score result after full board rescore.
    ScoreResult Fill(const uint32_t idx, const AreaCard& card)
    {
        assert(idx >= 1 && idx <= 19);
        auto& area = areas_[idx];
        area.card_ = card;
        area.box_.SetContent(CardHtml_(card));
        return Rescore_();
    }

    // Auto-fill first empty position. Returns {position, ScoreResult}.
    std::pair<uint32_t, ScoreResult> SeqFill(const AreaCard& card)
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (!areas_[i].card_.has_value()) {
                areas_[i].card_ = card;
                areas_[i].box_.SetContent(CardHtml_(card));
                return {i, Rescore_()};
            }
        }
        // Board is full - should not happen if caller checks HasEmptyPosition first
        return {0, ScoreResult{base_score_, line_count_, 0}};
    }

    bool IsFilled(const uint32_t idx) const
    {
        assert(idx >= 1 && idx <= 19);
        return areas_[idx].card_.has_value();
    }

    bool HasEmptyPosition() const
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (!areas_[i].card_.has_value()) return true;
        }
        return false;
    }

    // Get the card at a position (for talents like 劫掠)
    const std::optional<AreaCard>& GetCard(const uint32_t idx) const
    {
        assert(idx >= 1 && idx <= 19);
        return areas_[idx].card_;
    }

    // Get filled positions (for talents like 劫掠)
    std::vector<uint32_t> GetFilledPositions() const
    {
        std::vector<uint32_t> positions;
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (areas_[i].card_.has_value()) {
                positions.push_back(i);
            }
        }
        return positions;
    }

    // Apply 以退为进 talent: convert all 7→6, 4→3 on the board
    // Then rescore
    ScoreResult ApplyRetreatingAdvance()
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (areas_[i].card_.has_value()) {
                auto& card = *areas_[i].card_;
                int32_t a = card.PointAt(0);
                int32_t b = card.PointAt(1);
                int32_t c = card.PointAt(2);
                bool changed = false;
                // Only transform non-wild directions (value != 10)
                if (a != 10 && a == 4) { a = 3; changed = true; }
                if (a != 10 && a == 7) { a = 6; changed = true; }
                if (b != 10 && b == 4) { b = 3; changed = true; }
                if (b != 10 && b == 7) { b = 6; changed = true; }
                if (c != 10 && c == 4) { c = 3; changed = true; }
                if (c != 10 && c == 7) { c = 6; changed = true; }
                if (changed) {
                    areas_[i].card_ = AreaCard(a, b, c);
                    areas_[i].box_.SetContent(CardHtml_(*areas_[i].card_));
                }
            }
        }
        return Rescore_();
    }

    // Apply 完美块 talent: convert all (3,1,2) cards to wild (only check non-wild directions)
    ScoreResult ApplyPerfectBlock()
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (areas_[i].card_.has_value()) {
                auto& card = *areas_[i].card_;
                if (card.IsWild()) continue;
                // Check non-wild directions for 3,1,2 pattern
                int32_t a = card.PointAt(0), b = card.PointAt(1), c = card.PointAt(2);
                if ((a == 3 || a == 10) && (b == 1 || b == 10) && (c == 2 || c == 10)) {
                    areas_[i].card_ = AreaCard(); // full wild
                    areas_[i].box_.SetContent(CardHtml_(*areas_[i].card_));
                }
            }
        }
        return Rescore_();
    }

    // Apply 两级反转 talent: swap all 1↔9 on the board, then rescore
    ScoreResult ApplyDigitReverse()
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (areas_[i].card_.has_value()) {
                auto& card = *areas_[i].card_;
                int32_t a = card.PointAt(0), b = card.PointAt(1), c = card.PointAt(2);
                bool changed = false;
                auto swap19 = [&](int32_t& v) {
                    if (v != 10) {
                        if (v == 1) { v = 9; changed = true; }
                        else if (v == 9) { v = 1; changed = true; }
                    }
                };
                swap19(a); swap19(b); swap19(c);
                if (changed) {
                    areas_[i].card_ = AreaCard(a, b, c);
                    areas_[i].box_.SetContent(CardHtml_(*areas_[i].card_));
                }
            }
        }
        return Rescore_();
    }

    ScoreResult ApplyNineAsWild()
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (areas_[i].card_.has_value()) {
                AreaCard card = *areas_[i].card_;
                card.ApplyNineAsWild();
                if (!(card == *areas_[i].card_)) {
                    areas_[i].card_ = card;
                    areas_[i].box_.SetContent(CardHtml_(*areas_[i].card_));
                }
            }
        }
        return Rescore_();
    }

    ScoreResult SetWildAt(const uint32_t idx)
    {
        assert(idx >= 1 && idx <= 19);
        areas_[idx].card_ = AreaCard();
        areas_[idx].box_.SetContent(CardHtml_(*areas_[idx].card_));
        return Rescore_();
    }

    // Apply 星河流转 talent: positions 2,4,7,13,16,18 get a single-direction wild
    // 2&18 → VERT (dir 1), 4&16 → TOP_RIGHT (dir 2), 7&13 → TOP_LEFT (dir 0)
    ScoreResult ApplyGalaxyFlow()
    {
        static const std::array<std::pair<uint32_t, uint32_t>, 6> k_galaxy_positions = {{
            {2, 1}, {4, 2}, {7, 0}, {13, 0}, {16, 2}, {18, 1}
        }};
        for (auto [pos, dir] : k_galaxy_positions) {
            if (areas_[pos].card_.has_value()) {
                auto& card = *areas_[pos].card_;
                int32_t a = card.PointAt(0), b = card.PointAt(1), c = card.PointAt(2);
                if (dir == 0) a = 10;
                else if (dir == 1) b = 10;
                else c = 10;
                areas_[pos].card_ = AreaCard(a, b, c);
                areas_[pos].box_.SetContent(CardHtml_(*areas_[pos].card_));
            }
        }
        return Rescore_();
    }

    // Apply 星河流转 to a single position (idx). If idx is in the galaxy set and
    // the card's corresponding direction is not already wild, mutate it to wild
    // and rescore. Returns nullopt when no change occurred (so callers can skip
    // the broadcast / score-delta handling).
    // Direction mapping matches ApplyGalaxyFlow: 2&18→VERT, 4&16→TOP_RIGHT, 7&13→TOP_LEFT.
    std::optional<std::pair<uint32_t, ScoreResult>> ApplyGalaxyFlowAt(const uint32_t idx)
    {
        uint32_t dir = 0;
        switch (idx) {
            case 2: case 18: dir = 1; break;
            case 4: case 16: dir = 2; break;
            case 7: case 13: dir = 0; break;
            default: return std::nullopt;
        }
        if (!areas_[idx].card_.has_value()) return std::nullopt;
        auto& card = *areas_[idx].card_;
        int32_t a = card.PointAt(0), b = card.PointAt(1), c = card.PointAt(2);
        int32_t& target = (dir == 0) ? a : (dir == 1) ? b : c;
        if (target == 10) return std::nullopt; // already wild in that direction
        target = 10;
        areas_[idx].card_ = AreaCard(a, b, c);
        areas_[idx].box_.SetContent(CardHtml_(*areas_[idx].card_));
        return std::make_pair(dir, Rescore_());
    }

    // Remove a card from position idx (for 临时用品 expiry)
    ScoreResult RemoveCard(const uint32_t idx)
    {
        assert(idx >= 1 && idx <= 19);
        areas_[idx].card_ = std::nullopt;
        areas_[idx].box_.SetContent(Image_("num_" + std::to_string(idx)));
        return Rescore_();
    }

    ScoreResult SwapCards(const uint32_t lhs, const uint32_t rhs)
    {
        assert(lhs >= 1 && lhs <= 19);
        assert(rhs >= 1 && rhs <= 19);
        std::swap(areas_[lhs].card_, areas_[rhs].card_);
        areas_[lhs].box_.SetContent(areas_[lhs].card_.has_value() ? CardHtml_(*areas_[lhs].card_) : Image_("num_" + std::to_string(lhs)));
        areas_[rhs].box_.SetContent(areas_[rhs].card_.has_value() ? CardHtml_(*areas_[rhs].card_) : Image_("num_" + std::to_string(rhs)));
        return Rescore_();
    }

    int32_t BaseScore() const { return base_score_; }
    uint32_t LineCount() const { return line_count_; }
    std::string ToHtml() const { return table_.ToString(); }

    std::string Image_(std::string name) const { return "![](file:///" + image_path_ + std::move(name) + ".png)"; }

  private:
    static constexpr uint32_t k_size = 3;
    static constexpr uint32_t k_max_row = k_size * 4 + 2;
    static constexpr uint32_t k_max_column = k_size * 2 + 1;

    std::string CardHtml_(const AreaCard& card) const
    {
        return card.ToHtml(image_path_);
    }

    bool IsValid_(const Coordinate coordinate) const
    {
        return std::abs(coordinate.x_) + std::abs(coordinate.y_) < static_cast<int32_t>(k_size) * 2
            && std::abs(coordinate.x_) < static_cast<int32_t>(k_size);
    }

    // Full board rescore: check all 15 lines, update walls, return result
    ScoreResult Rescore_()
    {
        int32_t old_score = base_score_;

        // Reset all wall line indicators
        for (auto& [coor, wall] : walls_) {
            wall.ClearAllLines();
        }

        base_score_ = 0;
        line_count_ = 0;

        for (const auto& line_def : k_all_lines) {
            CheckLine_(line_def);
        }

        // Update all wall images
        for (auto& [coor, wall] : walls_) {
            wall.box_.SetContent(Image_(wall.ImageName()));
        }

        return ScoreResult{base_score_, line_count_, base_score_ - old_score};
    }

    void CheckLine_(const LineDefinition& line_def)
    {
        const auto& positions = line_def.positions;
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);

        // Check if all positions are filled
        for (uint32_t pos : positions) {
            if (!areas_[pos].card_.has_value()) {
                return; // Not all filled
            }
        }

        // Find the matching value (considering per-direction wilds: value 10 = wild)
        std::optional<int32_t> matched_value;
        bool all_wild = true;

        for (uint32_t pos : positions) {
            const auto& card = *areas_[pos].card_;
            int32_t val = card.PointAt(dir_idx);
            if (val != 10) {
                all_wild = false;
                if (!matched_value.has_value()) {
                    matched_value = val;
                } else if (val != *matched_value) {
                    return; // Mismatch - not a completed line
                }
            }
        }

        // Line is completed!
        line_count_++;

        if (all_wild) {
            base_score_ += 10 * static_cast<int32_t>(positions.size());
        } else {
            base_score_ += *matched_value * static_cast<int32_t>(positions.size());
        }

        // Update walls along this line's direction
        UpdateWallsForLine_(line_def);
    }

    void UpdateWallsForLine_(const LineDefinition& line_def)
    {
        if (line_def.positions.size() < 2) return;

        // Compute actual step from position coordinates (avoids direction/ordering mismatch)
        const Coordinate& first_coord = areas_[line_def.positions.front()].coordinate_;
        const Coordinate& second_coord = areas_[line_def.positions[1]].coordinate_;
        const Coordinate& last_coord = areas_[line_def.positions.back()].coordinate_;
        const Coordinate step{second_coord.x_ - first_coord.x_, second_coord.y_ - first_coord.y_};

        // Walk from first position backwards (opposite direction)
        WalkAndSetWalls_(first_coord + (-step), -step, line_def.direction);
        // Walk from last position forwards
        WalkAndSetWalls_(last_coord + step, step, line_def.direction);
    }

    void WalkAndSetWalls_(Coordinate coor, const Coordinate& step, Direct direction)
    {
        for (auto it = walls_.find(coor); it != walls_.end(); it = walls_.find(coor += step)) {
            switch (direction) {
                case Direct::左上: it->second.SetLine<Direct::左上>(); break;
                case Direct::垂直: it->second.SetLine<Direct::垂直>(); break;
                case Direct::右上: it->second.SetLine<Direct::右上>(); break;
            }
        }
    }

    static const Coordinate& GetDirectStep_(Direct direction)
    {
        switch (direction) {
            case Direct::左上: return k_direct_step<Direct::左上>;
            case Direct::垂直: return k_direct_step<Direct::垂直>;
            case Direct::右上: return k_direct_step<Direct::右上>;
        }
        static const Coordinate dummy{0, 0};
        return dummy;
    }

    const std::string image_path_;
    std::vector<Area> areas_;
    std::map<Coordinate, Wall> walls_;
    html::Table table_;
    int32_t base_score_ = 0;
    uint32_t line_count_ = 0;
};

// ==================== CSS Style (adapted from opencomb) ====================

static std::string GetStyle(const std::string& resource_path)
{
    return R"(
<style>
    .brick {
        position: relative;
        width: 64px;
        height: 64px;
        display: flex;
        justify-content: center;
        align-items: center;
    }
    .brick img {
        position: absolute;
        width: 100%;
        height: 100%;
        left: 0;
        top: 0;
        z-index: 1;
    }
</style>)";
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

// Player depends on AreaCard / TalentComb / k_all_lines defined above, so
// player.h must be included at the bottom of this file (not the top).
#include "player.h"
