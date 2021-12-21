#pragma once

#include <array>
#include <ranges>
#include <algorithm>
#include <bitset>
#include <set>
#include <memory>

#include "utility/html.h"

namespace renju {

enum class AreaType { EMPTY, FORBID, WHITE, BLACK };
enum class Result { CONTINUE_OK, CONTINUE_CRASH, CONTINUE_EXTEND, TIE_FULL_BOARD, TIE_DOUBLE_WIN, WIN_BLACK, WIN_WHITE };

class Piece
{
  public:

  private:
    AreaType type_; // should only be BLACK or WHITE
    std::set<std::pair<uint32_t, uint32_t>> liberties_;
    std::set<std::pair<uint32_t, uint32_t>> areas_;
    bool tmp_disappered_;
};

class GoBoard
{
  public:
    static constexpr const uint32_t k_size_ = 15;

    // should be valid
    bool CanBeSet(const uint32_t row, const uint32_t column, const AreaType type) const
    {
        
    }

    // should be valid
    bool Set(const uint32_t row, const uint32_t column, const AreaType type) const
    {
    }

  private:


    std::array<std::array<std::shared_ptr<Piece>, k_size_>, k_size_> pieces_;
};

class Board
{
  public:
    static constexpr const uint32_t k_size_ = 15;

    Board(const std::string& image_path)
        : image_path_(image_path)
        , expend_level_(2)
        , empty_count_((expend_level_ * 2 + 1) * (expend_level_ * 2 + 1))
    {
        for (auto& row : areas_) {
            for (auto& area : row) {
                area = AreaType::EMPTY;
            }
        }
    }

    bool CanBeSet(const uint32_t row, const uint32_t col) const { return Is_(row, col, AreaType::EMPTY); }

    // should satisfy CanBeSet
    Result Set(const uint32_t black_row, const uint32_t black_column, const uint32_t white_row, const uint32_t white_column)
    {
        ClearHighlight_();
        if (black_row == white_row && black_column == white_column) {
            areas_[black_row][black_column] = AreaType::FORBID;
            highlight_flag_[black_row][black_column] = true;
            return (empty_count_ -= 1) == 0 ? Result::TIE_FULL_BOARD : Result::CONTINUE_CRASH;
        }
        const auto set_chess = [this](const uint32_t row, const uint32_t col, const AreaType type)
            {
                areas_[row][col] = type;
                highlight_flag_[row][col] = true;
                return HasRenju_(row, col);
            };
        const bool black_renju = set_chess(black_row, black_column, AreaType::BLACK);
        const bool white_renju = set_chess(white_row, white_column, AreaType::WHITE);
        return (empty_count_ -= 2) == 0   ? Result::TIE_FULL_BOARD  :
               black_renju && white_renju ? Result::TIE_DOUBLE_WIN  :
               black_renju                ? Result::WIN_BLACK       :
               white_renju                ? Result::WIN_WHITE       :
               TryExpand_()               ? Result::CONTINUE_EXTEND :
                                            Result::CONTINUE_OK     ;
    }

    // should satisfy CanBeSet
    Result Set(const uint32_t row, const uint32_t col, const AreaType type)
    {
        ClearHighlight_();
        const bool renju = SetChess_(row, col, type);
        return (empty_count_ -= 1) == 0         ? Result::TIE_FULL_BOARD  :
               renju && type == AreaType::BLACK ? Result::WIN_BLACK       :
               renju && type == AreaType::WHITE ? Result::WIN_WHITE       :
               TryExpand_()                     ? Result::CONTINUE_EXTEND :
                                                  Result::CONTINUE_OK     ;
    }

    // should satisfy CanBeSet
    std::string SetAndToHtml(const uint32_t m, const uint32_t n, const AreaType type)
    {
        areas_[m][n] = type;
        highlight_flag_[m][n] = true;
        const std::string s = ToHtml();
        areas_[m][n] = AreaType::EMPTY;
        highlight_flag_[m][n] = false;
        return s;
    }

    std::string ToHtml() const
    {
        html::Table table(expend_level_ * 2 + 3, expend_level_ * 2 + 3);
        table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        const auto idx2c = [this](const uint32_t idx) { return idx == MinPos_() ? '0' : idx == MaxPos_() ? '2' : '1'; };
        for (uint32_t i = 0; i < expend_level_ * 2 + 1; ++i) {
            const bool highlight_row = highlight_flag_[MinPos_() + i].any();
            const bool highlight_column =
                std::ranges::any_of(highlight_flag_, [&](const auto& bitset) { return bitset.test(MinPos_() + i); });
            const std::string column_index =
                (highlight_column ? HTML_COLOR_FONT_HEADER(red) " **" : "") +
                std::to_string(MinPos_() + i) +
                (highlight_column ? "** " HTML_FONT_TAIL : "");
            const std::string row_index =
                (highlight_row ? HTML_COLOR_FONT_HEADER(red) " **" : "") +
                std::string(1, static_cast<char>('A' + MinPos_() + i)) +
                (highlight_row ? "** " HTML_FONT_TAIL : "");
            table.Get(0, i + 1).SetContent(column_index);
            table.GetLastRow(i + 1).SetContent(column_index);
            table.Get(i + 1, 0).SetContent(row_index);
            table.GetLastColumn(i + 1).SetContent(row_index);
            for (uint32_t j = 0; j < expend_level_ * 2 + 1; ++j) {
                std::string image_name;
                switch (areas_[MinPos_() + i][MinPos_() + j]) {
                case AreaType::WHITE:
                    image_name = "c_w";
                    break;
                case AreaType::BLACK:
                    image_name = "c_b";
                    break;
                case AreaType::FORBID:
                    image_name = "c_c";
                    break;
                case AreaType::EMPTY:
                    image_name = "b_";
                    image_name += idx2c(MinPos_() + i);
                    image_name += idx2c(MinPos_() + j);
                    break;
                }
                if (highlight_flag_[MinPos_() + i][MinPos_() + j]) {
                    image_name += "_l";
                }
                table.Get(i + 1, j + 1).SetContent(Image_(std::move(image_name)));
            }
        }
        return "<style>html,body{color:#6b421d; background:#d8bf81;}</style>\n" + table.ToString();
    }

  private:
    bool SetChess_(const uint32_t row, const uint32_t col, const AreaType type)
    {
        areas_[row][col] = type;
        highlight_flag_[row][col] = true;
        return HasRenju_(row, col);
    }

    bool Is_(const uint32_t row, const uint32_t col, const AreaType type) const
    {
        return MinPos_() <= row && row <= MaxPos_() && MinPos_() <= col && col <= MaxPos_() && areas_[row][col] == type;
    }

    bool HasRenju_(const uint32_t row, const uint32_t col)
    {
        return HasRenju_(row, col, -1, 1) + HasRenju_(row, col, 1, 1) + HasRenju_(row, col, 0, 1) + HasRenju_(row, col, 1, 0) > 0;
    }

    bool HasRenju_(const uint32_t row, const uint32_t col, const int32_t direct_x, const int32_t direct_y)
    {
        using pair = std::pair<uint32_t, uint32_t>;
        const auto type = areas_[row][col];
        std::vector<pair> renjus = { pair{row, col} };
        const auto extend_range = [&](const int32_t d)
            {
                for (uint32_t line_x = row + direct_x * d, line_y = col + direct_y * d;
                        Is_(line_x, line_y, type);
                        line_x += direct_x * d, line_y += direct_y * d) {
                    renjus.emplace_back(line_x, line_y);
                }
            };
        extend_range(1);
        extend_range(-1);
        if (renjus.size() >= 5) {
            for (const auto& [m, n] : renjus) {
                highlight_flag_[m].set(n);
            }
            return true;
        }
        return false;
    }

    bool TryExpand_()
    {
        if (expend_level_ < k_size_ / 2 && EdgeHas_(AreaType::BLACK) && EdgeHas_(AreaType::WHITE)) {
            ++expend_level_;
            empty_count_ += expend_level_ * 2 * 4;
            return true;
        }
        return false;
    }

    uint32_t MinPos_() const { return k_size_ / 2 - expend_level_; }

    uint32_t MaxPos_() const { return k_size_ / 2 + expend_level_; }

    bool EdgeHas_(const AreaType type) const
    {
        return std::ranges::any_of(std::views::iota(MinPos_(), MaxPos_() + 1), [this, type](const uint32_t pos)
                {
                    return areas_[MinPos_()][pos] == type || areas_[pos][MinPos_()] == type ||
                           areas_[MaxPos_()][pos] == type || areas_[pos][MaxPos_()] == type;
                });
    }

    std::string Image_(std::string name) const { return "![](file://" + image_path_ + "/" + std::move(name) + ".bmp)"; }

    void ClearHighlight_() { std::ranges::for_each(highlight_flag_, [](auto& flags) { flags.reset(); }); }

    const std::string image_path_;
    std::array<std::array<AreaType, k_size_>, k_size_> areas_;
    std::array<std::bitset<k_size_>, k_size_> highlight_flag_;
    uint32_t expend_level_; // [ k_size_ / 2 - expend_level_, k_size_ / 2 + expend_level_] is available
    uint32_t empty_count_;
};

}
