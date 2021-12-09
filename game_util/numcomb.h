#pragma once

#include <cassert>

#include <vector>
#include <optional>
#include <array>
#include <filesystem>
#include <numeric>
#include <iostream>
#include <map>

#include "../utility/html.h"

namespace comb {

static constexpr const uint32_t k_direct_max = 3;
enum class Direct { TOP_LEFT = 0, VERT = 1, TOP_RIGHT = 2};

struct Coordinate
{
    Coordinate& operator+=(const Coordinate& c)
    {
        x_ += c.x_;
        y_ += c.y_;
        return *this;
    }

    friend Coordinate operator+(const Coordinate& _1, const Coordinate& _2)
    {
        Coordinate tmp(_1);
        return tmp += _2;
    }

    Coordinate operator-() const { return Coordinate{-x_, -y_}; }

    auto operator<=>(const Coordinate&) const = default;

    int32_t x_;
    int32_t y_;
};

template <Direct direct> const Coordinate k_direct_step;
template <> const Coordinate k_direct_step<Direct::TOP_LEFT>{1, 1};
template <> const Coordinate k_direct_step<Direct::VERT>{0, 2};
template <> const Coordinate k_direct_step<Direct::TOP_RIGHT>{-1, 1};

class AreaCard
{
  public:
    AreaCard() {} // wild card

    AreaCard(const int32_t a, const int32_t b, const int32_t c) : points_(std::in_place, std::array<int32_t, k_direct_max>{a, b, c}) {}

    template <Direct direct>
    bool IsMatch(const int32_t point) const { return !points_.has_value() || points_->at(static_cast<uint32_t>(direct)) == point; }

    int32_t PointSum() const
    {
        if (points_.has_value()) {
            return std::accumulate(points_->begin(), points_->end(), 0);
        } else {
            return k_wild_score_sum;
        }
    }

    template <Direct direct>
    std::optional<int32_t> Point() const
    {
        if (points_.has_value()) {
            return points_->at(static_cast<uint32_t>(direct));
        } else {
            return std::nullopt;
        }
    }

    std::string ImageName() const
    {
        std::string str = "card_";
        if (points_.has_value()) {
            for (const int32_t point : *points_) {
                str += std::to_string(point);
            }
        } else {
            str += "any";
        }
        return str;
    }

    bool IsWild() const { return !points_.has_value(); }

  private:
    static constexpr int32_t k_wild_score_sum = 30;
    std::optional<std::array<int32_t, k_direct_max>> points_;
};

class Wall
{
  public:
    friend class Comb;

    Wall(html::Box& box) : box_(box), has_line_{false, false, false} {}

    template <Direct direct>
    void SetLine() { has_line_.at(static_cast<uint32_t>(direct)) = true; }

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

class Area
{
  public:
    friend class Comb;

    Area(html::Box& box, const Coordinate coordinate) : box_(box), coordinate_(coordinate) {}

  private:
    html::Box& box_;
    Coordinate coordinate_;
    std::optional<AreaCard> card_;
};

class Comb
{
  public:
    Comb(std::string image_path) : image_path_(std::move(image_path)), table_(k_max_row, k_max_column)
    {
        table_.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");

        static constexpr int32_t k_zero_row = 0;
        static constexpr int32_t k_zero_col = k_size + 2;
        static constexpr int32_t k_mid_row = k_size * 2;
        static constexpr int32_t k_mid_col = k_size;

        static const Coordinate zero_coor{k_zero_col - k_mid_col, k_zero_row - k_mid_row};
        html::Box& zero_box = table_.Get(k_zero_row, k_zero_col);
        areas_.emplace_back(zero_box, zero_coor);

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

        const auto num = walls_.erase(zero_coor);
        assert(num == 1);
        zero_box.SetContent(Image_("num_0"));
    }

    Comb(const Comb& comb) = delete;
    Comb(Comb&& comb) = delete;

    int32_t Fill(const uint32_t idx, const AreaCard& card)
    {
        auto& area = areas_[idx];
        area.card_ = card;
        const auto img_str = Image_(card.ImageName());
        area.box_.SetContent(img_str);
        if (idx == 0) {
            return card.PointSum();
        } else {
            return Check_(area.coordinate_);
        }
    }

    void Clear(const uint32_t idx);

    bool IsFilled(const uint32_t idx) const { return areas_[idx].card_.has_value(); }

    std::string ToHtml() const { return table_.ToString(); } 

    // -2 -1  0  1  2       value of coordinate.x
    //              x
    //           x
    //        .     x
    //     .     .          (. + x) is the initial count (left of coordinate.x)
    //  .     .     .
    //     .     .          x is to-remove count
    //  .     .     .
    //     .     .          . is the real count
    //  .     .     .
    //     .     .
    //        .     x
    //           x          then idx the real count + (coordinate.y offset value)
    //              x
    static uint32_t ToIndex(const Coordinate coordinate)
    {
        const int32_t level = coordinate.x_ + k_size - 1;
        uint32_t idx = (k_size + k_size + level - 1) * level / 2;
        if (coordinate.x_ > 1) { // x == 2
            idx -= (coordinate.x_ - 1) * coordinate.x_; // idx -= 2
        }
        idx += (coordinate.y_ + k_size * 2 - std::abs(coordinate.x_)) / 2;
        return idx;
    }

  private:
    static constexpr uint32_t k_size = 3;
    static constexpr uint32_t k_max_row = k_size * 4 + 2;
    static constexpr uint32_t k_max_column = k_size * 2 + 1;

    std::string Image_(std::string name) { return "![](file://" + image_path_ + std::move(name) + ".png)"; }

    template <Direct direct>
    int32_t CheckOneDirect_(const Coordinate coordinate)
    {
        assert(Get_(coordinate).card_.has_value());
        auto point = Get_(coordinate).card_->Point<direct>();
        uint32_t count = 1;
        const auto check = [&](Coordinate& coor, const Coordinate& step)
            {
                for (; IsValid_(coor); coor += step) {
                    ++count;
                    const auto& area = Get_(coor);
                    if (!area.card_.has_value()) { // happens when the area is empty
                        return false;
                    }
                    if (!point.has_value()) { // happens when previous card are wild
                        point = area.card_->Point<direct>();
                        continue;
                    }
                    if (!area.card_->IsMatch<direct>(*point)) {
                        return false;
                    }
                }
                return true;
            };

        const auto wall_line = [&](Coordinate& coor, const Coordinate& step)
            {
                for (auto it = walls_.find(coor); it != walls_.end(); it = walls_.find(coor += step)) {
                    Wall& wall = it->second;
                    wall.SetLine<direct>();
                    wall.box_.SetContent(Image_(wall.ImageName()));
                }
            };

        auto head_coor = coordinate + k_direct_step<direct>;
        auto tail_coor = coordinate + -k_direct_step<direct>;
        if (!check(head_coor, k_direct_step<direct>) || !check(tail_coor, -k_direct_step<direct>)) {
            return 0;
        }

        wall_line(head_coor, k_direct_step<direct>);
        wall_line(tail_coor, -k_direct_step<direct>);

        // TODO: set around success line
        if (point.has_value()) {
            return *point * count;
        } else {
            return 10000; // TODO: is an impossible case, we should limit number of wild card less than k_size
        }
    }

    int32_t Check_(const Coordinate coordinate)
    {
        return CheckOneDirect_<Direct::VERT>(coordinate) +
               CheckOneDirect_<Direct::TOP_LEFT>(coordinate) +
               CheckOneDirect_<Direct::TOP_RIGHT>(coordinate);
    }

    Area& Get_(const Coordinate coordinate) { return areas_[ToIndex(coordinate)]; }

    bool IsValid_(const Coordinate coordinate)
    {
        return std::abs(coordinate.x_) + std::abs(coordinate.y_) < k_size * 2 && std::abs(coordinate.x_) < k_size;
    }

    const std::string image_path_;
    std::vector<Area> areas_;
    std::map<Coordinate, Wall> walls_;
    html::Table table_;
};


}
