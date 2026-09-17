// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include "utility/html.h"
#include "utility/random.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// ==================== Special Event ====================

enum class SpecialEvent
{
    无 = 0,           // 无
    大的要来了,        // 卡池中没有1
    两极分化,          // 卡池中没有5
    大的没了,          // 卡池中没有9
    天降恩泽,          // 第一轮每人发一个癞子
    调色盘,            // 卡池中添加大量癞子
    有1吗,             // 每行1额外加12分
    小透不算挂,        // 提前公布下一轮的卡
    愚人节,            // ？？？：天降恩泽+调色盘+有1吗+小透不算挂
    COUNT
};

static std::string SpecialEventShortName(SpecialEvent event)
{
    switch (event) {
        case SpecialEvent::无: return "无";
        case SpecialEvent::大的要来了: return "大的要来了";
        case SpecialEvent::两极分化: return "两极分化";
        case SpecialEvent::大的没了: return "大的没了";
        case SpecialEvent::天降恩泽: return "天降恩泽";
        case SpecialEvent::调色盘: return "调色盘";
        case SpecialEvent::有1吗: return "有1吗";
        case SpecialEvent::小透不算挂: return "小透不算挂";
        case SpecialEvent::愚人节: return "？？？";
        default: return "未知";
    }
}

static std::string SpecialEventName(SpecialEvent event)
{
    switch (event) {
        case SpecialEvent::无: return "无";
        case SpecialEvent::大的要来了: return "大的要来了——卡池中没有1";
        case SpecialEvent::两极分化: return "两极分化——卡池中没有5";
        case SpecialEvent::大的没了: return "大的没了——卡池中没有9";
        case SpecialEvent::天降恩泽: return "天降恩泽——第一轮每人发一个癞子";
        case SpecialEvent::调色盘: return "调色盘——卡池中添加大量癞子";
        case SpecialEvent::有1吗: return "有1吗——每行1额外加12分";
        case SpecialEvent::小透不算挂: return "小透不算挂——提前公布下一轮的卡";
        case SpecialEvent::愚人节: return "？？？\n\n2023年愚人节纪念事件：\n天降恩泽、调色盘、有1吗、小透不算挂";
        default: return "未知";
    }
}

static bool HasWindfall(SpecialEvent event) { return event == SpecialEvent::天降恩泽 || event == SpecialEvent::愚人节; }
static bool HasColorful(SpecialEvent event) { return event == SpecialEvent::调色盘 || event == SpecialEvent::愚人节; }
static bool HasValuableOne(SpecialEvent event) { return event == SpecialEvent::有1吗 || event == SpecialEvent::愚人节; }
static bool HasForesee(SpecialEvent event) { return event == SpecialEvent::小透不算挂 || event == SpecialEvent::愚人节; }

// 事件选项：保留"随机"=0 的特殊值，其余 = enum 值 + 1。
// 解析端（mygame.cc）用 `static_cast<SpecialEvent>(event_opt - 1)` 反查。
inline std::map<std::string, int> MakeSpecialEventOptionMap()
{
    std::map<std::string, int> m;
    m.emplace("随机", 0);
    for (int i = 0; i < static_cast<int>(SpecialEvent::COUNT); ++i) {
        m.emplace(SpecialEventShortName(static_cast<SpecialEvent>(i)), i + 1);
    }
    return m;
}

// ==================== Score Mode ====================

// 终局计分方式：排名 = 按淘汰名次结算；分数 = 直接按玩家最终总分结算。
enum class ScoreMode { 排名 = 0, 分数 = 1 };

inline std::map<std::string, ScoreMode> MakeScoreModeOptionMap()
{
    return {{"排名", ScoreMode::排名}, {"分数", ScoreMode::分数}};
}

// ==================== Select Order ====================

// 选牌轮中血量与分数都相同时的先后顺序：
//   随机 = 由随机种子决定；顺位 = 按开局分配的玩家编号升序。
enum class SelectOrder { 随机 = 0, 顺位 = 1 };

inline std::map<std::string, SelectOrder> MakeSelectOrderOptionMap()
{
    return {{"随机", SelectOrder::随机}, {"顺位", SelectOrder::顺位}};
}

class CloudComb;  // forward-declare (Wall/Area befriend it)

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

// ==================== AreaCard ====================

class AreaCard
{
  public:
    AreaCard() : points_{10, 10, 10} {} // wild card (癞子/万能牌) = 10,10,10

    AreaCard(const int32_t a, const int32_t b, const int32_t c)
        : points_{a, b, c} {}

    int32_t PointAt(const uint32_t dir) const { return points_[dir]; }

    int32_t PointSum() const { return std::accumulate(points_.begin(), points_.end(), 0); }

    bool IsWild() const { return points_[0] == 10 && points_[1] == 10 && points_[2] == 10; }

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
        return std::to_string(val);
    }

    std::array<int32_t, k_direct_max> points_;
};

// ==================== Wall ====================

class Wall
{
  public:
    friend class CloudComb;

    Wall(html::Box& box) : box_(box), has_line_{false, false, false} {}

    template <Direct direct>
    void SetLine() { has_line_.at(static_cast<uint32_t>(direct)) = true; }

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
    friend class CloudComb;

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

// ==================== CloudComb Board ====================

class CloudComb
{
  public:
    CloudComb(std::string image_path) : image_path_(std::move(image_path)), table_(k_max_row, k_max_column)
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

    CloudComb(const CloudComb&) = delete;
    CloudComb(CloudComb&&) = delete;

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

    bool HasEmptyPosition() const
    {
        for (uint32_t i = 1; i < areas_.size(); ++i) {
            if (!areas_[i].card_.has_value()) return true;
        }
        return false;
    }

    const std::optional<AreaCard>& GetCard(const uint32_t idx) const
    {
        assert(idx >= 1 && idx <= 19);
        return areas_[idx].card_;
    }

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

    const std::string image_path_;
    std::vector<Area> areas_;
    std::map<Coordinate, Wall> walls_;
    html::Table table_;
    int32_t base_score_ = 0;
    uint32_t line_count_ = 0;
};

// ==================== Player ====================

struct Player
{
    Player(std::string resource_path, int32_t init_hp = 150)
        : comb_(new CloudComb(std::move(resource_path)))
        , hp_(init_hp)
        , base_score_(0)
        , valuable_one_bonus_(0)
    {
    }

    Player(Player&&) = default;

    // --- Core state ---
    std::unique_ptr<CloudComb> comb_;
    int32_t hp_;
    int32_t base_score_;
    int32_t valuable_one_bonus_; // "有1吗" special event bonus (stored separately for display)

    // Battle tracking (for achievements)
    bool never_lost_ = true;     // True if player never lost a battle (draws OK)

    // 总分 = 盘面连线分 + 「有1吗」事件加成
    int32_t TotalScore() const { return base_score_ + valuable_one_bonus_; }

    // Calculate the "有1吗" special event bonus
    int32_t ValuableOneBonus() const
    {
        int32_t bonus = 0;
        for (const auto& line_def : k_all_lines) {
            if (IsLineCompleted_(line_def)) {
                int32_t matched = GetLineMatchedValue_(line_def);
                if (matched == 1) bonus += 12;
            }
        }
        return bonus;
    }

    // Check if all filled cells share the same number in any direction (wilds count as any)
    bool AllCellsSameNumber() const
    {
        auto filled = comb_->GetFilledPositions();
        if (filled.size() < 19) return false; // Must have all cells filled

        for (uint32_t dir = 0; dir < k_direct_max; ++dir) {
            // Try each possible digit 1-9
            for (int32_t digit = 1; digit <= 9; ++digit) {
                bool all_match = true;
                for (uint32_t pos : filled) {
                    const auto& card = comb_->GetCard(pos);
                    if (!card.has_value()) { all_match = false; break; }
                    int32_t val = card->PointAt(dir);
                    if (val != 10 && val != digit) {  // 10 = wild in this direction
                        all_match = false;
                        break;
                    }
                }
                if (all_match) return true;
            }
        }
        return false;
    }

    // Check if any line (of any length) is entirely wild in its direction
    bool HasFullWildLine() const
    {
        for (const auto& line_def : k_all_lines) {
            const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
            bool all_wild = true;
            for (uint32_t pos : line_def.positions) {
                const auto& card = comb_->GetCard(pos);
                if (!card.has_value() || card->PointAt(dir_idx) != 10) {
                    all_wild = false;
                    break;
                }
            }
            if (all_wild) return true;
        }
        return false;
    }

    // Update base score from board rescore result
    void UpdateScore(const ScoreResult& result, bool has_valuable_one)
    {
        base_score_ = result.base_score;
        valuable_one_bonus_ = has_valuable_one ? ValuableOneBonus() : 0;
    }

  private:
    bool IsLineCompleted_(const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        std::optional<int32_t> matched_value;
        for (uint32_t pos : line_def.positions) {
            const auto& card = comb_->GetCard(pos);
            if (!card.has_value()) return false;
            int32_t val = card->PointAt(dir_idx);
            if (val != 10) {  // 10 = wild in this direction
                if (!matched_value.has_value()) {
                    matched_value = val;
                } else if (val != *matched_value) {
                    return false;
                }
            }
        }
        return true;
    }

    int32_t GetLineMatchedValue_(const LineDefinition& line_def) const
    {
        const uint32_t dir_idx = static_cast<uint32_t>(line_def.direction);
        for (uint32_t pos : line_def.positions) {
            const auto& card = comb_->GetCard(pos);
            if (card.has_value() && card->PointAt(dir_idx) != 10) {
                return card->PointAt(dir_idx);
            }
        }
        return 0; // all wild
    }
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
