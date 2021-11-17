#pragma once

#include <array>
#include <ranges>
#include <algorithm>

#include "../utility/html.h"

namespace quixo {

enum class ErrCode { OK, INVALID_SRC, INVALID_DST };

struct Coor
{
    uint32_t x_;
    uint32_t y_;
    template <bool IS_X>
    uint32_t Get() const;
    Coor& operator=(const Coor&) = default;
};

template <>
uint32_t Coor::Get<true>() const { return x_; }
template <>
uint32_t Coor::Get<false>() const { return y_; }

static constexpr const uint32_t k_edge_num = 16;

static Coor idx2coor(const uint32_t idx)
{
    assert(k_edge_num > idx);
    static const std::array<Coor, k_edge_num> k_edge_coors = {
        Coor{0, 0}, Coor{0, 1}, Coor{0, 2}, Coor{0, 3},
        Coor{0, 4}, Coor{1, 4}, Coor{2, 4}, Coor{3, 4},
        Coor{4, 4}, Coor{4, 3}, Coor{4, 2}, Coor{4, 1},
        Coor{4, 0}, Coor{3, 0}, Coor{2, 0}, Coor{1, 0},
    };
    return k_edge_coors[idx];
}

static uint32_t coor2idx(const Coor& coor)
{
    return coor.x_ <= coor.y_ ? coor.x_ + coor.y_ : k_edge_num - coor.x_ - coor.y_;
}

enum class Type { O = 'o', X = 'x', _ = '_' };

class Board
{
  public:
    Board(const std::string& image_path) : image_path_(image_path)
    {
        for (auto& arr : areas_) {
            for (auto& type : arr) {
                type = Type::_;
            }
        }
    }

    std::string ToHtml() const
    {
        Table table(7, 7);
        table.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        const auto set_image = [&](const uint32_t row, const uint32_t col, std::string name)
            {
                table.Get(row, col).SetContent("![](file://" + image_path_ + "/" + std::move(name) + ".png)");
            };
        for (uint32_t i = 0; i < 5; ++i) {
            set_image(0, 1 + i, "num_" + std::to_string(0 + i));
            set_image(1 + i, 6, "num_" + std::to_string(4 + i));
            set_image(6, 5 - i, "num_" + std::to_string(8 + i));
            set_image(5 - i, 0, "num_" + std::to_string((12 + i) % 16));
        }
        const auto set_box_image = [&](const uint32_t x, const uint32_t y, const char* const prefix)
            {
                set_image(x + 1, y + 1, std::string(prefix) + static_cast<char>(areas_[x][y]));
            };
        for (uint32_t x = 0; x < 5; ++x) {
            for (uint32_t y = 0; y < 5; ++y) {
                set_box_image(x, y, "box_");
            }
        }
        if (last_move_coor_.has_value()) {
            set_box_image(last_move_coor_->x_, last_move_coor_->y_, "light_");
        }
        ForAllSuccLine_([&](const auto& coors, const Type type)
                {
                    std::ranges::for_each(coors, [&](const Coor& coor) { set_box_image(coor.x_, coor.y_, "light_"); });
                });

        return table.ToString();
    }

    ErrCode Push(const uint32_t src, const uint32_t dst, const Type type)
    {
        assert(src < k_edge_num && dst < k_edge_num);
        assert(type != Type::_);
        const auto src_coor = idx2coor(src);
        const auto dst_coor = idx2coor(dst);
        const Type src_type = areas_[src_coor.x_][src_coor.y_];
        if (src_type != Type::_ && src_type != type) {
            return ErrCode::INVALID_SRC;
        }
        if (!TryPush_<true>(src_coor, dst_coor, type) && !TryPush_<false>(src_coor, dst_coor, type)) {
            return ErrCode::INVALID_DST;
        }
        areas_[dst_coor.x_][dst_coor.y_] = type;
        last_move_coor_.emplace(dst_coor);
        return ErrCode::OK;
    }

    Type CheckWin() const
    {
        bool x_win = false;
        bool o_win = false;
        ForAllSuccLine_([&](const auto& coors, const Type type) { (type == Type::X ? x_win : o_win) = true; });
        return !x_win ^ o_win ? Type::_ :
                        x_win ? Type::X : Type::O;
    }

    std::vector<uint32_t> ValidDsts(const uint32_t src)
    {
        std::vector<uint32_t> dsts;
        const auto src_coor = idx2coor(src);
        if (src_coor.x_ != 0) {
            dsts.emplace_back(coor2idx(Coor{0, src_coor.y_}));
        }
        if (src_coor.x_ != 4) {
            dsts.emplace_back(coor2idx(Coor{4, src_coor.y_}));
        }
        if (src_coor.y_ != 0) {
            dsts.emplace_back(coor2idx(Coor{src_coor.x_, 0}));
        }
        if (src_coor.y_ != 4) {
            dsts.emplace_back(coor2idx(Coor{src_coor.x_, 4}));
        }
        return dsts;
    }

  private:
    template <typename Fn>
    void ForAllSuccLine_(const Fn& fn) const
    {
        const auto check_all_same = [&](const auto& coors, const Type type)
            {
                if (std::ranges::all_of(coors, [&](const Coor& coor) { return areas_[coor.x_][coor.y_] == type; })) {
                    fn(coors, type);
                    return true;
                }
                return false;
            };
        const auto check_line = [&](const auto& transform)
            {
                const auto coors = std::views::iota(0) | std::views::take(5) | std::views::transform(transform);
                check_all_same(coors, Type::X) || check_all_same(coors, Type::O);
            };
        for (uint32_t i = 0; i < 5; ++i) {
            check_line([&](const uint32_t x) { return Coor{x, i}; });
            check_line([&](const uint32_t x) { return Coor{i, x}; });
        }
        check_line([&](const uint32_t x) { return Coor{x, x}; });
        check_line([&](const uint32_t x) { return Coor{x, 4 - x}; });
    }

    template <bool A_IS_X>
    bool TryPush_(const Coor& src_coor, const Coor& dst_coor, const Type type)
    {
        const auto src_a = src_coor.Get<A_IS_X>();
        const auto dst_a = dst_coor.Get<A_IS_X>();
        const auto b = src_coor.Get<!A_IS_X>();
        if (src_a == dst_a || b != dst_coor.Get<!A_IS_X>() || (dst_a != 0 && dst_a != 4)) {
            return false;
        }
        const int direct = src_a > dst_a ? -1 : 1;
        for (uint32_t a = src_a; a != dst_a; a += direct) {
            GetArea_<A_IS_X>(a, b) = GetArea_<A_IS_X>(a + direct, b);
        }
        return true;
    }

    template <bool IS_X>
    Type& GetArea_(const uint32_t a, const uint32_t b)
    {
        return IS_X ? areas_[a][b] : areas_[b][a];
    }

    const std::string image_path_;
    std::array<std::array<Type, 5>, 5> areas_;
    std::optional<Coor> last_move_coor_;
};

}
