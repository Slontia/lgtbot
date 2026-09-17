// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>

#include "utility/html.h"

namespace lgtbot {

namespace game_util {

namespace skating_chess {

// 棋盘边长
static constexpr int32_t k_board_size = 5;
// 每名玩家的棋子数量
static constexpr int32_t k_piece_num = 3;
// 玩家数量
static constexpr int32_t k_side_num = 2;
// 空格标记
static constexpr int32_t k_empty = -1;

// 双方阵营的名称与配色
static const char* const k_side_names[k_side_num] = {"红", "蓝"};
static const char* const k_side_colors[k_side_num] = {"#E8544F", "#3E8FD8"};
static const char* const k_side_dark_colors[k_side_num] = {"#A8322E", "#215F91"};

// 滑行方向，自正上方起顺时针排列
enum class Direct { UP = 0, UP_RIGHT = 1, RIGHT = 2, DOWN_RIGHT = 3, DOWN = 4, DOWN_LEFT = 5, LEFT = 6, UP_LEFT = 7 };

static constexpr int32_t k_direct_num = 8;

// 方向的中文名称，下标与 Direct 一致
static const char* const k_direct_names[k_direct_num] = {"上", "右上", "右", "右下", "下", "左下", "左", "左上"};

// 指令中可接受的方向写法
static const std::map<std::string, Direct> k_direction_map = {
    {"上", Direct::UP},           {"u", Direct::UP},          {"U", Direct::UP},
    {"右上", Direct::UP_RIGHT},   {"ur", Direct::UP_RIGHT},   {"UR", Direct::UP_RIGHT},
    {"右", Direct::RIGHT},        {"r", Direct::RIGHT},       {"R", Direct::RIGHT},
    {"右下", Direct::DOWN_RIGHT}, {"dr", Direct::DOWN_RIGHT}, {"DR", Direct::DOWN_RIGHT},
    {"下", Direct::DOWN},         {"d", Direct::DOWN},        {"D", Direct::DOWN},
    {"左下", Direct::DOWN_LEFT},  {"dl", Direct::DOWN_LEFT},  {"DL", Direct::DOWN_LEFT},
    {"左", Direct::LEFT},         {"l", Direct::LEFT},        {"L", Direct::LEFT},
    {"左上", Direct::UP_LEFT},    {"ul", Direct::UP_LEFT},    {"UL", Direct::UP_LEFT},
};

// 棋盘坐标，row_ 自上向下增长，col_ 自左向右增长
struct Coor
{
    int32_t row_;
    int32_t col_;

    bool operator==(const Coor& o) const { return row_ == o.row_ && col_ == o.col_; }
    bool operator!=(const Coor& o) const { return !(*this == o); }
};

// 八个方向对应的坐标增量，下标与 Direct 一致
static constexpr std::array<Coor, k_direct_num> k_direct_offsets {{
    {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}
}};

// 四条判定直线的方向（竖、横、主对角、副对角）
static constexpr std::array<Coor, 4> k_line_offsets {{
    {1, 0}, {0, 1}, {1, 1}, {1, -1}
}};

// 图片整体配色
inline constexpr const char* k_page_style = R"(
<style>
    html, body { background: #EEF4F9; color: #22303C; }
</style>
)";

// 图片宽度
inline constexpr uint32_t k_image_width = 580;

// 将坐标转为如 C4 的展示文本，列用字母、行用数字
inline std::string CoorToStr(const Coor& coor)
{
    return std::string(1, static_cast<char>('A' + coor.col_)) + std::to_string(coor.row_ + 1);
}

class Board
{
  public:
    // 摆放初始局面：红方位于顶行两侧与下部中央，蓝方与之中心对称
    Board()
    {
        for (auto& row : cells_) {
            row.fill(k_empty);
        }
        pieces_[0] = {Coor{0, 1}, Coor{0, 3}, Coor{3, 2}};
        pieces_[1] = {Coor{4, 3}, Coor{4, 1}, Coor{1, 2}};
        for (int32_t side = 0; side < k_side_num; ++side) {
            for (int32_t index = 0; index < k_piece_num; ++index) {
                const Coor& coor = pieces_[side][index];
                cells_[coor.row_][coor.col_] = side;
            }
        }
    }

    static bool IsValid(const Coor& coor)
    {
        return coor.row_ >= 0 && coor.row_ < k_board_size && coor.col_ >= 0 && coor.col_ < k_board_size;
    }

    // 格子上的棋子所属阵营，空格返回 k_empty
    int32_t GetCell(const Coor& coor) const
    {
        assert(IsValid(coor));
        return cells_[coor.row_][coor.col_];
    }

    // 某方第 index 枚棋子（index 从 0 开始）的位置
    const Coor& PiecePos(const int32_t side, const int32_t index) const
    {
        assert(side >= 0 && side < k_side_num && index >= 0 && index < k_piece_num);
        return pieces_[side][index];
    }

    // 计算棋子沿指定方向滑行的落点。棋子一直滑到碰到墙壁或其他棋子为止；
    // 若紧邻的格子就是墙壁或棋子，则本次滑行无法移动，返回空
    std::optional<Coor> TrySlide(const int32_t side, const int32_t index, const Direct direct) const
    {
        const Coor& offset = k_direct_offsets[static_cast<int32_t>(direct)];
        const Coor& from = PiecePos(side, index);
        Coor dst = from;
        while (true) {
            const Coor next{dst.row_ + offset.row_, dst.col_ + offset.col_};
            if (!IsValid(next) || cells_[next.row_][next.col_] != k_empty) {
                break;
            }
            dst = next;
        }
        if (dst == from) {
            return std::nullopt;
        }
        return dst;
    }

    // 执行滑行，返回落点。调用前必须确认 TrySlide 成功
    Coor Slide(const int32_t side, const int32_t index, const Direct direct)
    {
        const auto dst = TrySlide(side, index, direct);
        assert(dst.has_value());
        const Coor from = pieces_[side][index];
        cells_[from.row_][from.col_] = k_empty;
        cells_[dst->row_][dst->col_] = side;
        pieces_[side][index] = *dst;
        last_from_ = from;
        last_to_ = *dst;
        last_side_ = side;
        return *dst;
    }

    // 某方三枚棋子连成的相邻直线，未连成时返回空
    std::vector<Coor> LineCoors(const int32_t side) const
    {
        const auto& pieces = pieces_[side];
        for (int32_t mid = 0; mid < k_piece_num; ++mid) {
            const Coor& center = pieces[mid];
            const Coor& other1 = pieces[(mid + 1) % k_piece_num];
            const Coor& other2 = pieces[(mid + 2) % k_piece_num];
            for (const Coor& offset : k_line_offsets) {
                const Coor head{center.row_ - offset.row_, center.col_ - offset.col_};
                const Coor tail{center.row_ + offset.row_, center.col_ + offset.col_};
                if ((other1 == head && other2 == tail) || (other1 == tail && other2 == head)) {
                    return {head, center, tail};
                }
            }
        }
        return {};
    }

    // 某方是否已有三子连成相邻直线
    bool HasLine(const int32_t side) const { return !LineCoors(side).empty(); }

    // 某方全部合法行动，元素为（棋子下标，方向）
    std::vector<std::pair<int32_t, Direct>> LegalMoves(const int32_t side) const
    {
        std::vector<std::pair<int32_t, Direct>> moves;
        for (int32_t index = 0; index < k_piece_num; ++index) {
            for (int32_t d = 0; d < k_direct_num; ++d) {
                if (TrySlide(side, index, static_cast<Direct>(d)).has_value()) {
                    moves.emplace_back(index, static_cast<Direct>(d));
                }
            }
        }
        return moves;
    }

    // 某枚棋子可滑行的方向
    std::vector<Direct> LegalDirects(const int32_t side, const int32_t index) const
    {
        std::vector<Direct> directs;
        for (int32_t d = 0; d < k_direct_num; ++d) {
            if (TrySlide(side, index, static_cast<Direct>(d)).has_value()) {
                directs.emplace_back(static_cast<Direct>(d));
            }
        }
        return directs;
    }

    // 局面指纹，用于判定重复局面。忽略棋子编号，只比较双方棋子的位置集合与行动方
    std::string StateKey(const int32_t side_to_move) const
    {
        std::string key;
        key.reserve(k_side_num * k_piece_num + 1);
        for (int32_t side = 0; side < k_side_num; ++side) {
            std::array<int32_t, k_piece_num> codes;
            for (int32_t index = 0; index < k_piece_num; ++index) {
                codes[index] = pieces_[side][index].row_ * k_board_size + pieces_[side][index].col_;
            }
            std::sort(codes.begin(), codes.end());
            for (const int32_t code : codes) {
                key += static_cast<char>('A' + code);
            }
        }
        key += static_cast<char>('0' + side_to_move);
        return key;
    }

    // 绘制棋盘。side_to_move 为即将行动的一方，传 k_empty 表示对局已结束；win_coors 为需要高亮的获胜直线
    std::string ToHtml(const int32_t side_to_move, const std::vector<Coor>& win_coors = {}) const
    {
        const std::string canvas = std::to_string(k_canvas_px);
        std::string svg = "<svg width=\"" + canvas + "\" height=\"" + canvas + "\" viewBox=\"0 0 " + canvas + " " +
                          canvas + "\" xmlns=\"http://www.w3.org/2000/svg\">";
        // 外框
        svg += "<rect x=\"0\" y=\"0\" width=\"" + canvas + "\" height=\"" + canvas + "\" rx=\"18\" fill=\"#1B3247\"/>";
        // 冰面格子
        for (int32_t row = 0; row < k_board_size; ++row) {
            for (int32_t col = 0; col < k_board_size; ++col) {
                svg += Rect_(CellX_(col), CellY_(row), k_cell_px, k_cell_px,
                             (row + col) % 2 == 0 ? "#F3F9FE" : "#DCEAF6");
            }
        }
        // 网格线
        for (int32_t i = 1; i < k_board_size; ++i) {
            svg += Line_(k_pad_px, CellY_(i), k_pad_px + k_board_px, CellY_(i), "#A9C6DE", 1, 1.0);
            svg += Line_(CellX_(i), k_pad_px, CellX_(i), k_pad_px + k_board_px, "#A9C6DE", 1, 1.0);
        }
        // 棋盘边框
        svg += "<rect x=\"" + std::to_string(k_pad_px) + "\" y=\"" + std::to_string(k_pad_px) + "\" width=\"" +
               std::to_string(k_board_px) + "\" height=\"" + std::to_string(k_board_px) +
               "\" fill=\"none\" stroke=\"#6E92B0\" stroke-width=\"3\"/>";
        // 坐标标注
        for (int32_t i = 0; i < k_board_size; ++i) {
            const std::string col_label(1, static_cast<char>('A' + i));
            const std::string row_label = std::to_string(i + 1);
            svg += Text_(CellX_(i) + k_cell_px / 2, k_pad_px - 11, col_label, k_label_font, "#9FBED8", "600");
            svg += Text_(CellX_(i) + k_cell_px / 2, k_pad_px + k_board_px + 24, col_label, k_label_font, "#9FBED8", "600");
            svg += Text_(k_pad_px - 16, CellY_(i) + k_cell_px / 2 + 6, row_label, k_label_font, "#9FBED8", "600");
            svg += Text_(k_pad_px + k_board_px + 16, CellY_(i) + k_cell_px / 2 + 6, row_label, k_label_font, "#9FBED8", "600");
        }
        // 获胜直线高亮
        for (const Coor& coor : win_coors) {
            svg += "<rect x=\"" + std::to_string(CellX_(coor.col_)) + "\" y=\"" + std::to_string(CellY_(coor.row_)) +
                   "\" width=\"" + std::to_string(k_cell_px) + "\" height=\"" + std::to_string(k_cell_px) +
                   "\" fill=\"#FFD75E\" opacity=\"0.55\"/>";
        }
        if (win_coors.size() == static_cast<size_t>(k_piece_num)) {
            svg += Line_(CenterX_(win_coors.front()), CenterY_(win_coors.front()), CenterX_(win_coors.back()),
                         CenterY_(win_coors.back()), "#F2A81A", 9, 0.7);
        }
        // 上一手棋的滑行轨迹
        if (last_from_.has_value() && last_to_.has_value()) {
            const char* const trail_color = k_side_colors[last_side_];
            svg += Line_(CenterX_(*last_from_), CenterY_(*last_from_), CenterX_(*last_to_), CenterY_(*last_to_),
                         trail_color, 7, 0.3);
            svg += "<circle cx=\"" + std::to_string(CenterX_(*last_from_)) + "\" cy=\"" +
                   std::to_string(CenterY_(*last_from_)) + "\" r=\"10\" fill=\"none\" stroke=\"" + trail_color +
                   "\" stroke-width=\"3\" stroke-dasharray=\"4 3\" opacity=\"0.8\"/>";
        }
        // 棋子
        for (int32_t side = 0; side < k_side_num; ++side) {
            for (int32_t index = 0; index < k_piece_num; ++index) {
                const Coor& coor = pieces_[side][index];
                const int32_t cx = CenterX_(coor);
                const int32_t cy = CenterY_(coor);
                if (side == side_to_move) {
                    svg += Circle_(cx, cy, k_ring_r, "none", k_side_colors[side], 2, 0.55);
                }
                svg += Circle_(cx, cy + 2, k_piece_r, "#000000", nullptr, 0, 0.18);
                svg += Circle_(cx, cy, k_piece_r, k_side_dark_colors[side], nullptr, 0, 1.0);
                svg += Circle_(cx, cy, k_piece_inner_r, k_side_colors[side], nullptr, 0, 1.0);
                svg += Text_(cx, cy + 10, std::to_string(index + 1), k_number_font, "#FFFFFF", "bold");
            }
        }
        svg += "</svg>";
        return "<div align=\"center\" style=\"margin-top:8px;\">" + svg + "</div>";
    }

  private:
    // 画布尺寸
    static constexpr int32_t k_cell_px = 72;
    static constexpr int32_t k_pad_px = 32;
    static constexpr int32_t k_board_px = k_cell_px * k_board_size;
    static constexpr int32_t k_canvas_px = k_board_px + k_pad_px * 2;
    // 棋子与文字尺寸
    static constexpr int32_t k_piece_r = 27;
    static constexpr int32_t k_piece_inner_r = 23;
    static constexpr int32_t k_ring_r = 31;
    static constexpr int32_t k_number_font = 28;
    static constexpr int32_t k_label_font = 18;

    static int32_t CellX_(const int32_t col) { return k_pad_px + col * k_cell_px; }
    static int32_t CellY_(const int32_t row) { return k_pad_px + row * k_cell_px; }
    static int32_t CenterX_(const Coor& coor) { return CellX_(coor.col_) + k_cell_px / 2; }
    static int32_t CenterY_(const Coor& coor) { return CellY_(coor.row_) + k_cell_px / 2; }

    static std::string Rect_(const int32_t x, const int32_t y, const int32_t width, const int32_t height,
                             const char* const fill)
    {
        return "<rect x=\"" + std::to_string(x) + "\" y=\"" + std::to_string(y) + "\" width=\"" +
               std::to_string(width) + "\" height=\"" + std::to_string(height) + "\" fill=\"" + fill + "\"/>";
    }

    static std::string Line_(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2,
                             const char* const color, const int32_t width, const double opacity)
    {
        return "<line x1=\"" + std::to_string(x1) + "\" y1=\"" + std::to_string(y1) + "\" x2=\"" +
               std::to_string(x2) + "\" y2=\"" + std::to_string(y2) + "\" stroke=\"" + color + "\" stroke-width=\"" +
               std::to_string(width) + "\" stroke-linecap=\"round\" opacity=\"" + OpacityStr_(opacity) + "\"/>";
    }

    static std::string Circle_(const int32_t cx, const int32_t cy, const int32_t r, const char* const fill,
                              const char* const stroke, const int32_t stroke_width, const double opacity)
    {
        std::string str = "<circle cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) + "\" r=\"" +
                          std::to_string(r) + "\" fill=\"" + fill + "\"";
        if (stroke != nullptr) {
            str += std::string(" stroke=\"") + stroke + "\" stroke-width=\"" + std::to_string(stroke_width) + "\"";
        }
        str += " opacity=\"" + OpacityStr_(opacity) + "\"/>";
        return str;
    }

    static std::string Text_(const int32_t x, const int32_t y, const std::string& content, const int32_t font_size,
                             const char* const color, const char* const weight)
    {
        return "<text x=\"" + std::to_string(x) + "\" y=\"" + std::to_string(y) +
               "\" text-anchor=\"middle\" font-family=\"sans-serif\" font-size=\"" + std::to_string(font_size) +
               "\" font-weight=\"" + weight + "\" fill=\"" + color + "\">" + content + "</text>";
    }

    // 将透明度格式化为两位小数，避免不同地区的浮点输出差异
    static std::string OpacityStr_(const double opacity)
    {
        const int32_t percent = static_cast<int32_t>(opacity * 100 + 0.5);
        if (percent >= 100) {
            return "1";
        }
        return "0." + std::string(percent < 10 ? "0" : "") + std::to_string(percent);
    }

    // 棋盘各格所属阵营
    std::array<std::array<int32_t, k_board_size>, k_board_size> cells_;
    // 双方各三枚棋子的位置，下标即棋子编号减一
    std::array<std::array<Coor, k_piece_num>, k_side_num> pieces_;
    // 上一手棋的起点、落点与行动方
    std::optional<Coor> last_from_;
    std::optional<Coor> last_to_;
    int32_t last_side_{0};
};

} // namespace skating_chess

} // namespace game_util

} // namespace lgtbot
