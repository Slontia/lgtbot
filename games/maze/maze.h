
#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <queue>
#include <random>
#include <vector>

#include "constants.h"

// 墙壁位置引用
struct WallRef
{
    bool horizontal = true; // true：横向墙；false：纵向墙
    int c = 0;
    int r = 0;
};

// 指定格子的指定方向是否为地图边界
inline bool IsBorder(const Pos& pos, const Direct direct, const int size)
{
    switch (direct) {
        case Direct::UP:    return pos.r == 0;
        case Direct::DOWN:  return pos.r == size - 1;
        case Direct::LEFT:  return pos.c == 0;
        case Direct::RIGHT: return pos.c == size - 1;
    }
    return true;
}

// 获取指定格子指定方向对应的墙壁位置，边界返回 nullopt
inline std::optional<WallRef> WallAt(const Pos& pos, const Direct direct, const int size)
{
    if (IsBorder(pos, direct, size)) {
        return std::nullopt;
    }
    switch (direct) {
        case Direct::UP:    return WallRef{true, pos.c, pos.r - 1};
        case Direct::DOWN:  return WallRef{true, pos.c, pos.r};
        case Direct::LEFT:  return WallRef{false, pos.c - 1, pos.r};
        case Direct::RIGHT: return WallRef{false, pos.c, pos.r};
    }
    return std::nullopt;
}

inline Pos MovePos(const Pos& pos, const Direct direct)
{
    const int d = static_cast<int>(direct);
    return Pos{pos.c + k_DC_Direct[d], pos.r + k_DR_Direct[d]};
}

// 指定墙位是否紧贴 cell 四周
inline bool IsAdjacentWall(const WallRef& ref, const Pos& cell, const int size)
{
    for (int d = 0; d < 4; ++d) {
        const auto adjacent = WallAt(cell, static_cast<Direct>(d), size);
        if (adjacent.has_value() && adjacent->horizontal == ref.horizontal && adjacent->c == ref.c &&
                adjacent->r == ref.r) {
            return true;
        }
    }
    return false;
}

// 解析画墙指令的参数，每面墙形如「10左」「10 左」。解析失败时填入 err 并返回 false，成功时按输入顺序写入 out
inline bool ParseWallTokens(const std::vector<std::string>& tokens, const int size, std::vector<WallRef>& out,
        std::string& err)
{
    out.clear();
    const int max_id = size * size;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        size_t digit_len = 0;
        while (digit_len < token.size() && token[digit_len] >= '0' && token[digit_len] <= '9') {
            ++digit_len;
        }
        if (digit_len == 0) {
            err = "[错误] 无法识别的墙壁，格式为<编号><方向>，如 10左";
            return false;
        }
        std::string direct_text = token.substr(digit_len);
        if (direct_text.empty()) {
            // 编号与方向被空格分开时，尝试把下一个参数当作方向
            if (i + 1 < tokens.size() && direction_map.count(tokens[i + 1]) > 0) {
                direct_text = tokens[i + 1];
                ++i;
            } else {
                err = "[错误] 墙壁缺少方向，格式为<编号><方向>，如 10左";
                return false;
            }
        }
        Direct direct;
        if (!ParseDirect(direct_text, direct)) {
            err = "[错误] 无法识别的方向，可用方向：上下左右 / UDLR / sxzy";
            return false;
        }
        if (digit_len > 3) {
            err = "[错误] 编号超出范围，本局编号为 1 ~ " + std::to_string(max_id);
            return false;
        }
        const int id = std::stoi(token.substr(0, digit_len));
        if (id < 1 || id > max_id) {
            err = "[错误] 编号超出范围，本局编号为 1 ~ " + std::to_string(max_id);
            return false;
        }
        const Pos pos = IdToPos(id, size);
        const auto ref = WallAt(pos, direct, size);
        if (!ref.has_value()) {
            err = "[错误] " + std::to_string(id) + " 号格的" + std::string(dir_cn[static_cast<int>(direct)]) + "侧是地图边界，不允许绘制墙体";
            return false;
        }
        out.push_back(*ref);
    }
    if (out.empty()) {
        err = "[错误] 请至少给出一面墙，格式为<编号><方向>，如 10左";
        return false;
    }
    return true;
}


// 一张由某位玩家绘制、供其对手挑战的迷宫。
class Maze
{
  public:
    /* ========== 基础信息 ========== */
    int size = 5;

    void Reset(const int maze_size)
    {
        size = maze_size;
        for (int c = 0; c < MAX_MAZE_SIZE; ++c) {
            for (int r = 0; r + 1 < MAX_MAZE_SIZE; ++r) {
                wall_h_[c][r] = false;
                revealed_h_[c][r] = false;
                passed_h_[c][r] = false;
            }
        }
        for (int c = 0; c + 1 < MAX_MAZE_SIZE; ++c) {
            for (int r = 0; r < MAX_MAZE_SIZE; ++r) {
                wall_v_[c][r] = false;
                revealed_v_[c][r] = false;
                passed_v_[c][r] = false;
            }
        }
    }

    // 清空全部墙体（仅在绘制阶段使用）
    void ClearWalls() { Reset(size); }

    /* ========== 墙体读写 ========== */
    bool WallValue(const WallRef& ref) const
    {
        return ref.horizontal ? wall_h_[ref.c][ref.r] : wall_v_[ref.c][ref.r];
    }

    void SetWallValue(const WallRef& ref, const bool value)
    {
        (ref.horizontal ? wall_h_[ref.c][ref.r] : wall_v_[ref.c][ref.r]) = value;
    }

    // 翻转指定位置的墙体，返回翻转后是否存在墙体
    bool ToggleWall(const WallRef& ref)
    {
        const bool value = !WallValue(ref);
        SetWallValue(ref, value);
        return value;
    }

    int WallCount() const
    {
        int count = 0;
        for (const auto& ref : AllWallSlots()) {
            if (WallValue(ref)) {
                ++count;
            }
        }
        return count;
    }

    // 全部可放置墙体的位置，不含地图边界
    std::vector<WallRef> AllWallSlots() const
    {
        std::vector<WallRef> slots;
        slots.reserve(2 * size * (size - 1));
        for (int c = 0; c < size; ++c) {
            for (int r = 0; r + 1 < size; ++r) {
                slots.push_back(WallRef{true, c, r});
            }
        }
        for (int c = 0; c + 1 < size; ++c) {
            for (int r = 0; r < size; ++r) {
                slots.push_back(WallRef{false, c, r});
            }
        }
        return slots;
    }

    /* ========== 通行判定 ========== */
    // 真实的通行判定，地图边界视为墙体
    bool HasWall(const Pos& pos, const Direct direct) const
    {
        const auto ref = WallAt(pos, direct, size);
        return !ref.has_value() || WallValue(*ref);
    }

    /* ========== 公开信息 ========== */
    // 已被撞出、全场可见的墙体
    bool IsRevealed(const WallRef& ref) const
    {
        return ref.horizontal ? revealed_h_[ref.c][ref.r] : revealed_v_[ref.c][ref.r];
    }

    void Reveal(const WallRef& ref)
    {
        (ref.horizontal ? revealed_h_[ref.c][ref.r] : revealed_v_[ref.c][ref.r]) = true;
    }

    // 已被走通、确认无墙的位置，全场可见
    bool IsPassed(const WallRef& ref) const
    {
        return ref.horizontal ? passed_h_[ref.c][ref.r] : passed_v_[ref.c][ref.r];
    }

    void MarkPassed(const WallRef& ref)
    {
        (ref.horizontal ? passed_h_[ref.c][ref.r] : passed_v_[ref.c][ref.r]) = true;
    }

    // 该方向是否已经确定无法通行：地图边界，或已经被撞出来的墙体
    bool IsKnownBlocked(const Pos& pos, const Direct direct) const
    {
        const auto ref = WallAt(pos, direct, size);
        return !ref.has_value() || IsRevealed(*ref);
    }

    int RevealedCount() const
    {
        int count = 0;
        for (const auto& ref : AllWallSlots()) {
            if (IsRevealed(ref)) {
                ++count;
            }
        }
        return count;
    }

    /* ========== 连通性 ========== */
    int CellId(const Pos& pos) const { return pos.c * size + pos.r; }

    // 从 from 出发能否到达 to
    bool Reachable(const Pos& from, const Pos& to) const { return ShortestDistance(from, to) >= 0; }

    // 把 blocked 视为不可通行的格子，其余格子是否仍然彼此可达。终点自身不得充当通路
    bool IsConnectedExcept(const Pos& blocked) const
    {
        Pos origin{-1, -1};
        for (int c = 0; c < size && origin.c < 0; ++c) {
            for (int r = 0; r < size; ++r) {
                if (Pos{c, r} != blocked) {
                    origin = Pos{c, r};
                    break;
                }
            }
        }
        if (origin.c < 0) {
            return true;
        }
        std::array<bool, MAX_MAZE_SIZE * MAX_MAZE_SIZE> visited{};
        std::queue<Pos> que;
        que.push(origin);
        visited[CellId(origin)] = true;
        int count = 1;
        while (!que.empty()) {
            const Pos cur = que.front();
            que.pop();
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (HasWall(cur, direct)) {
                    continue;
                }
                const Pos next = MovePos(cur, direct);
                if (next == blocked || visited[CellId(next)]) {
                    continue;
                }
                visited[CellId(next)] = true;
                ++count;
                que.push(next);
            }
        }
        return count == size * size - 1;
    }

    // 从 from 到 to 的最短通路长度，不可达返回 -1
    int ShortestDistance(const Pos& from, const Pos& to) const
    {
        std::array<int, MAX_MAZE_SIZE * MAX_MAZE_SIZE> distance;
        distance.fill(-1);
        std::queue<Pos> que;
        que.push(from);
        distance[CellId(from)] = 0;
        while (!que.empty()) {
            const Pos cur = que.front();
            que.pop();
            if (cur == to) {
                return distance[CellId(cur)];
            }
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (HasWall(cur, direct)) {
                    continue;
                }
                const Pos next = MovePos(cur, direct);
                if (distance[CellId(next)] >= 0) {
                    continue;
                }
                distance[CellId(next)] = distance[CellId(cur)] + 1;
                que.push(next);
            }
        }
        return -1;
    }

    // 只依据公开信息寻路：绕开地图边界与已经撞出的墙，尚未探明的位置一律当作可以通行。
    // avoid_first 用于求次优路线：禁止以该方向作为第一步，增加电脑随机性
    std::vector<Direct> OptimisticPath(const Pos& from, const Pos& to, std::mt19937& g,
            const std::optional<Direct> avoid_first = std::nullopt) const
    {
        std::vector<int> prev_cell(MAX_MAZE_SIZE * MAX_MAZE_SIZE, -1);
        std::vector<int> prev_direct(MAX_MAZE_SIZE * MAX_MAZE_SIZE, -1);
        std::array<bool, MAX_MAZE_SIZE * MAX_MAZE_SIZE> visited{};
        std::queue<Pos> que;
        que.push(from);
        visited[CellId(from)] = true;
        while (!que.empty()) {
            const Pos cur = que.front();
            que.pop();
            if (cur == to) {
                break;
            }
            std::array<int, 4> order = {{0, 1, 2, 3}};
            std::shuffle(order.begin(), order.end(), g);
            for (const int d : order) {
                const Direct direct = static_cast<Direct>(d);
                if (IsKnownBlocked(cur, direct)) {
                    continue;
                }
                if (cur == from && avoid_first.has_value() && direct == *avoid_first) {
                    continue;
                }
                const Pos next = MovePos(cur, direct);
                if (visited[CellId(next)]) {
                    continue;
                }
                visited[CellId(next)] = true;
                prev_cell[CellId(next)] = CellId(cur);
                prev_direct[CellId(next)] = d;
                que.push(next);
            }
        }
        if (!visited[CellId(to)]) {
            return {};
        }
        std::vector<Direct> path;
        for (Pos cur = to; cur != from; ) {
            const int id = CellId(cur);
            path.push_back(static_cast<Direct>(prev_direct[id]));
            cur = Pos{prev_cell[id] / size, prev_cell[id] % size};
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    /* ========== 电脑玩家绘制 ========== */
    // 终点被围成只剩一个出口的死胡同，其余区域在终点不可通行的前提下依然全部连通。若干次尝试中取起点到终点最短通路最长的一张
    void GenerateForComputer(std::mt19937& g, const int wall_limit, const Pos& start, const Pos& goal)
    {
        Maze best;
        best.Reset(size);
        int best_distance = -1;
        for (int attempt = 0; attempt < GENERATE_ATTEMPT; ++attempt) {
            Maze trial;
            trial.Reset(size);
            int placed = 0;
            // 终点四周除随机保留的一个出口外全部砌墙，地图边界本身即为阻挡
            std::vector<Direct> goal_directs;
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (!IsBorder(goal, direct, size)) {
                    goal_directs.push_back(direct);
                }
            }
            std::shuffle(goal_directs.begin(), goal_directs.end(), g);
            for (size_t i = 1; i < goal_directs.size() && placed < wall_limit; ++i) {
                trial.SetWallValue(*WallAt(goal, goal_directs[i], size), true);
                ++placed;
            }
            // 终点四周的墙位不再参与随机布置，避免把唯一的出口也堵死
            std::vector<WallRef> slots;
            for (const auto& ref : trial.AllWallSlots()) {
                if (!IsAdjacentWall(ref, goal, size)) {
                    slots.push_back(ref);
                }
            }
            std::shuffle(slots.begin(), slots.end(), g);
            for (const auto& ref : slots) {
                if (placed >= wall_limit) {
                    break;
                }
                // 终点视为不可通行，其余格子必须始终彼此可达
                trial.SetWallValue(ref, true);
                if (trial.IsConnectedExcept(goal)) {
                    ++placed;
                } else {
                    trial.SetWallValue(ref, false);
                }
            }
            const int distance = trial.ShortestDistance(start, goal);
            if (distance > best_distance) {
                best_distance = distance;
                best = trial;
            }
        }
        *this = best;
    }

  private:
    // 电脑绘制迷宫时的尝试次数，取其中最短通路最长的一张
    static constexpr int GENERATE_ATTEMPT = 30;

    // wall_h_[c][r]：格子 (c, r) 与 (c, r+1) 之间的横向墙
    bool wall_h_[MAX_MAZE_SIZE][MAX_MAZE_SIZE - 1] = {};
    // wall_v_[c][r]：格子 (c, r) 与 (c+1, r) 之间的纵向墙
    bool wall_v_[MAX_MAZE_SIZE - 1][MAX_MAZE_SIZE] = {};

    bool revealed_h_[MAX_MAZE_SIZE][MAX_MAZE_SIZE - 1] = {};
    bool revealed_v_[MAX_MAZE_SIZE - 1][MAX_MAZE_SIZE] = {};

    bool passed_h_[MAX_MAZE_SIZE][MAX_MAZE_SIZE - 1] = {};
    bool passed_v_[MAX_MAZE_SIZE - 1][MAX_MAZE_SIZE] = {};
};
