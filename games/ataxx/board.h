// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <cassert>

#include "utility/html.h"

namespace lgtbot {

namespace game_util {

namespace ataxx {

static constexpr int32_t k_max_size = 10;
static constexpr int32_t k_max_players = 4;

// Player IDs: 0~3 for players, -1 for empty
static constexpr int32_t k_empty = -1;

// Colors for up to 4 players
static const char* k_player_color_names[] = {"黑", "白", "红", "蓝"};
static const char* k_player_colors[] = {"#222222", "#F5F5F5", "#E74C3C", "#3498DB"};

// SVG piece (filled circle, 28x28 for use in 40x40 cells)
inline std::string SvgPiece(const char* color)
{
    return std::string("<svg width=\"28\" height=\"28\" viewBox=\"0 0 28 28\">")
         + "<circle cx=\"14\" cy=\"14\" r=\"12\" fill=\"" + color + "\"/></svg>";
}

// SVG small dot for clone target hint (12x12)
inline std::string SvgCloneDot()
{
    return "<svg width=\"12\" height=\"12\" viewBox=\"0 0 12 12\">"
           "<circle cx=\"6\" cy=\"6\" r=\"5\" fill=\"#9ECDA0\"/></svg>";
}

// SVG triangle for jump target hint (12x12, upward pointing)
inline std::string SvgJumpTriangle()
{
    return "<svg width=\"12\" height=\"12\" viewBox=\"0 0 12 12\">"
           "<polygon points=\"6,1 11,11 1,11\" fill=\"#9ECDA0\"/></svg>";
}

// SVG small piece for turn order display (16x16)
inline std::string SvgSmallPiece(const char* color)
{
    return std::string("<svg width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">")
         + "<circle cx=\"8\" cy=\"8\" r=\"7\" fill=\"" + color + "\"/></svg>";
}

// SVG arrow for turn order display (20x16, right-pointing, vertically centered)
inline std::string SvgArrow()
{
    return "<svg width=\"20\" height=\"16\" viewBox=\"0 0 20 16\">"
           "<path d=\"M2,8 L14,8 M10,3 L16,8 L10,13\" stroke=\"#FFFFFF\" stroke-width=\"2\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/></svg>";
}

struct Coor
{
    int32_t row_;
    int32_t col_;

    bool operator==(const Coor& o) const { return row_ == o.row_ && col_ == o.col_; }
    bool operator!=(const Coor& o) const { return !(*this == o); }
};

// 8 directions for adjacency
static constexpr std::array<Coor, 8> k_directions {{
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}
}};

// 8 directions for jump (straight line distance 2, including diagonals)
static constexpr std::array<Coor, 8> k_jump_offsets {{
    {-2, 0}, {2, 0}, {0, -2}, {0, 2}, {-2, -2}, {-2, 2}, {2, -2}, {2, 2}
}};

class Board
{
  public:
    Board(const int32_t size, const int32_t player_num)
        : size_(size), player_num_(player_num)
    {
        assert(size >= 5 && size <= k_max_size);
        assert(player_num >= 2 && player_num <= k_max_players);

        // Initialize all cells as empty
        for (auto& row : cells_) {
            row.fill(k_empty);
        }

        // Place initial pieces at corners
        if (player_num == 2) {
            // 2 players: each player on a diagonal pair
            cells_[0][0] = 0;
            cells_[size_ - 1][size_ - 1] = 0;
            cells_[0][size_ - 1] = 1;
            cells_[size_ - 1][0] = 1;
        } else {
            // 3-4 players: 黑=top-right, 白=bottom-left, 红=top-left, 蓝=bottom-right
            cells_[0][size_ - 1] = 0;       // 黑 top-right
            cells_[size_ - 1][0] = 1;       // 白 bottom-left
            cells_[0][0] = 2;               // 红 top-left
            if (player_num >= 4) {
                cells_[size_ - 1][size_ - 1] = 3; // 蓝 bottom-right
            }
        }

        piece_counts_.fill(0);
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                if (cells_[r][c] != k_empty) {
                    ++piece_counts_[cells_[r][c]];
                }
            }
        }
    }

    int32_t Size() const { return size_; }
    int32_t PlayerNum() const { return player_num_; }
    int32_t PieceCount(int32_t pid) const { return piece_counts_[pid]; }

    bool IsValid(const Coor& c) const
    {
        return c.row_ >= 0 && c.row_ < size_ && c.col_ >= 0 && c.col_ < size_;
    }

    int32_t GetCell(const Coor& c) const { return cells_[c.row_][c.col_]; }

    // Check if a player has any legal move
    bool HasLegalMove(int32_t pid) const
    {
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                if (cells_[r][c] == pid) {
                    // Check clone moves (adjacent empty)
                    for (const auto& d : k_directions) {
                        Coor target{r + d.row_, c + d.col_};
                        if (IsValid(target) && cells_[target.row_][target.col_] == k_empty) {
                            return true;
                        }
                    }
                    // Check jump moves (distance 2)
                    for (const auto& d : k_jump_offsets) {
                        Coor target{r + d.row_, c + d.col_};
                        if (IsValid(target) && cells_[target.row_][target.col_] == k_empty) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    // Get all legal clone targets for a player (all adjacent empty cells of all player's pieces)
    std::vector<Coor> GetCloneTargets(int32_t pid) const
    {
        std::vector<Coor> targets;
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                if (cells_[r][c] == pid) {
                    for (const auto& d : k_directions) {
                        Coor target{r + d.row_, c + d.col_};
                        if (IsValid(target) && cells_[target.row_][target.col_] == k_empty) {
                            if (std::find(targets.begin(), targets.end(), target) == targets.end()) {
                                targets.push_back(target);
                            }
                        }
                    }
                }
            }
        }
        return targets;
    }

    // Get all legal jump targets for a player
    std::vector<std::pair<Coor, Coor>> GetJumpMoves(int32_t pid) const
    {
        std::vector<std::pair<Coor, Coor>> moves;
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                if (cells_[r][c] == pid) {
                    Coor from{r, c};
                    for (const auto& d : k_jump_offsets) {
                        Coor target{r + d.row_, c + d.col_};
                        if (IsValid(target) && cells_[target.row_][target.col_] == k_empty) {
                            moves.push_back({from, target});
                        }
                    }
                }
            }
        }
        return moves;
    }

    // Get jump-only targets (reachable by jump but NOT by clone)
    std::vector<Coor> GetJumpOnlyTargets(int32_t pid) const
    {
        auto clone_targets = GetCloneTargets(pid);
        std::vector<Coor> jump_only;
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                if (cells_[r][c] == pid) {
                    for (const auto& d : k_jump_offsets) {
                        Coor target{r + d.row_, c + d.col_};
                        if (IsValid(target) && cells_[target.row_][target.col_] == k_empty) {
                            // Only include if not already a clone target
                            if (std::find(clone_targets.begin(), clone_targets.end(), target) == clone_targets.end() &&
                                std::find(jump_only.begin(), jump_only.end(), target) == jump_only.end()) {
                                jump_only.push_back(target);
                            }
                        }
                    }
                }
            }
        }
        return jump_only;
    }

    // Check if a target cell is a valid clone destination for a given player
    bool IsCloneTarget(int32_t pid, const Coor& target) const
    {
        if (!IsValid(target) || cells_[target.row_][target.col_] != k_empty) return false;
        for (const auto& d : k_directions) {
            Coor adj{target.row_ + d.row_, target.col_ + d.col_};
            if (IsValid(adj) && cells_[adj.row_][adj.col_] == pid) {
                return true;
            }
        }
        return false;
    }

    // Check if (from -> to) is a valid jump for the player (straight line distance 2)
    bool IsJumpMove(int32_t pid, const Coor& from, const Coor& to) const
    {
        if (!IsValid(from) || !IsValid(to)) return false;
        if (cells_[from.row_][from.col_] != pid) return false;
        if (cells_[to.row_][to.col_] != k_empty) return false;
        Coor offset{to.row_ - from.row_, to.col_ - from.col_};
        for (const auto& d : k_jump_offsets) {
            if (d == offset) return true;
        }
        return false;
    }

    // Execute a clone move: place a new piece at target, then convert neighbors
    int32_t DoClone(int32_t pid, const Coor& target)
    {
        assert(IsCloneTarget(pid, target));
        cells_[target.row_][target.col_] = pid;
        ++piece_counts_[pid];
        return ConvertNeighbors_(pid, target);
    }

    // Execute a jump move: move piece from `from` to `to`, then convert neighbors
    int32_t DoJump(int32_t pid, const Coor& from, const Coor& to)
    {
        assert(IsJumpMove(pid, from, to));
        cells_[from.row_][from.col_] = k_empty;
        cells_[to.row_][to.col_] = pid;
        return ConvertNeighbors_(pid, to);
    }

    // Check if the game is over (no player can move)
    bool IsGameOver() const
    {
        for (int p = 0; p < player_num_; ++p) {
            if (piece_counts_[p] > 0 && HasLegalMove(p)) {
                return false;
            }
        }
        return true;
    }

    // Check if all empty cells are gone
    bool IsBoardFull() const
    {
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                if (cells_[r][c] == k_empty) return false;
            }
        }
        return true;
    }

    // Generate HTML for the board
    // clone_highlights: cells reachable by clone move (shown as small circle)
    // jump_highlights:  cells reachable only by jump move (shown as diamond)
    std::string ToHtml(int32_t current_player,
                       const std::vector<Coor>& clone_highlights = {},
                       const std::vector<Coor>& jump_highlights = {}) const
    {
        const int32_t table_size = size_ + 2; // +2 for column/row labels on both sides
        html::Table table(table_size, table_size);
        table.SetTableStyle(" align=\"center\" cellpadding=\"2\" cellspacing=\"0\"");

        // Fill labels (columns: A,B,C...; rows: 1,2,3...)
        for (int32_t i = 0; i < size_; ++i) {
            // Column labels (letters A, B, C...)
            table.Get(0, i + 1).SetContent(std::string(1, 'A' + i));
            table.Get(size_ + 1, i + 1).SetContent(std::string(1, 'A' + i));
            // Row labels (numbers starting from 1)
            table.Get(i + 1, 0).SetContent(std::to_string(i + 1));
            table.Get(i + 1, size_ + 1).SetContent(std::to_string(i + 1));
        }

        // Fill board cells
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c < size_; ++c) {
                auto& box = table.Get(r + 1, c + 1);
                Coor pos{r, c};

                // Determine highlight type
                bool is_clone_hl = false, is_jump_hl = false;
                for (const auto& h : clone_highlights) {
                    if (h == pos) { is_clone_hl = true; break; }
                }
                if (!is_clone_hl) {
                    for (const auto& h : jump_highlights) {
                        if (h == pos) { is_jump_hl = true; break; }
                    }
                }

                // Checkerboard pattern
                box.SetColor((r + c) % 2 == 0 ? "#1A5C32" : "#2E8B57");

                // Force square cells
                box.SetStyle("style=\"width:40px; height:40px; padding:0;\"");

                // Wrap content in flexbox div for precise centering
                // (html.cc injects \n\n around content, so td-level centering is unreliable)
                const std::string flex_open = "<div style=\"display:flex;align-items:center;justify-content:center;width:40px;height:40px;\">";
                const std::string flex_close = "</div>";

                int32_t cell = cells_[r][c];
                if (cell != k_empty) {
                    // Player piece: SVG circle (pixel-perfect centering, no font metric issues)
                    box.SetContent(flex_open + SvgPiece(k_player_colors[cell]) + flex_close);
                } else if (is_clone_hl) {
                    // Clone target: small circle dot
                    box.SetContent(flex_open + SvgCloneDot() + flex_close);
                } else if (is_jump_hl) {
                    // Jump-only target: triangle
                    box.SetContent(flex_open + SvgJumpTriangle() + flex_close);
                } else {
                    box.SetContent(flex_open + "&#8203;" + flex_close); // zero-width space
                }
            }
        }

        return "<style>html,body{color:#8fd59c; background:#217844;}</style>\n" + table.ToString();
    }

  private:
    // Convert all opponent pieces adjacent to `target` to `pid`'s pieces. Returns number converted.
    int32_t ConvertNeighbors_(int32_t pid, const Coor& target)
    {
        int32_t converted = 0;
        for (const auto& d : k_directions) {
            Coor adj{target.row_ + d.row_, target.col_ + d.col_};
            if (IsValid(adj)) {
                int32_t& cell = cells_[adj.row_][adj.col_];
                if (cell != k_empty && cell != pid) {
                    --piece_counts_[cell];
                    cell = pid;
                    ++piece_counts_[pid];
                    ++converted;
                }
            }
        }
        return converted;
    }

    int32_t size_;
    int32_t player_num_;
    std::array<std::array<int32_t, k_max_size>, k_max_size> cells_;
    std::array<int32_t, k_max_players> piece_counts_;
};

} // namespace ataxx

} // namespace game_util

} // namespace lgtbot
