
#pragma once

#include <algorithm>
#include <array>
#include <queue>
#include <random>
#include <vector>

#include "constants.h"

// 墙壁位置引用
struct WallRef
{
    bool horizontal = true; // true：横向墙，分隔上下两格；false：纵向墙，分隔左右两格
    int c = 0;
    int r = 0;
};

// 墙壁所属的统计区域
enum class Region { ABC, COL_D, EFG };

// 墙壁连通分组的统计结果
struct WallGroupResult
{
    int isolated = 0;   // 单独的墙
    int pair = 0;       // 恰好两面相连的墙组
    int large = 0;      // 三面及以上相连的墙组
};


class Maze
{
  public:
    // wall_h[c][r]：格子 (c, r) 与 (c, r+1) 之间的横向墙
    bool wall_h[MAZE_SIZE][MAZE_SIZE - 1] = {};
    // wall_v[c][r]：格子 (c, r) 与 (c+1, r) 之间的纵向墙
    bool wall_v[MAZE_SIZE - 1][MAZE_SIZE] = {};

    /* ========== 基础查询 ========== */

    static int CellId(const Pos& pos) { return pos.c * MAZE_SIZE + pos.r; }

    static Pos Move(const Pos& pos, const Direct direct)
    {
        const int d = static_cast<int>(direct);
        return Pos{pos.c + k_DC_Direct[d], pos.r + k_DR_Direct[d]};
    }

    // 指定格子的指定方向是否为地图边框
    static bool IsBorder(const Pos& pos, const Direct direct)
    {
        switch (direct) {
            case Direct::UP:    return pos.r == 0;
            case Direct::DOWN:  return pos.r == MAZE_SIZE - 1;
            case Direct::LEFT:  return pos.c == 0;
            case Direct::RIGHT: return pos.c == MAZE_SIZE - 1;
        }
        return true;
    }

    // 获取指定格子指定方向对应的墙壁位置，边框返回 nullopt
    static std::optional<WallRef> WallAt(const Pos& pos, const Direct direct)
    {
        if (IsBorder(pos, direct)) {
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

    bool WallValue(const WallRef& ref) const
    {
        return ref.horizontal ? wall_h[ref.c][ref.r] : wall_v[ref.c][ref.r];
    }

    void SetWallValue(const WallRef& ref, const bool value)
    {
        (ref.horizontal ? wall_h[ref.c][ref.r] : wall_v[ref.c][ref.r]) = value;
    }

    // 指定格子的指定方向是否不可通过（边框视为墙）
    bool HasWall(const Pos& pos, const Direct direct) const
    {
        const auto ref = WallAt(pos, direct);
        return !ref.has_value() || WallValue(*ref);
    }

    // 墙壁位置所属区域
    static Region RegionOf(const WallRef& ref)
    {
        if (ref.horizontal) {
            if (ref.c < k_center.c) return Region::ABC;
            if (ref.c > k_center.c) return Region::EFG;
            return Region::COL_D;
        }
        return ref.c < k_center.c ? Region::ABC : Region::EFG;
    }

    // 区域内全部可成为墙壁的位置：ABC 与 EFG 各 39 个，D 列 6 个
    static std::vector<WallRef> RegionPositions(const Region region)
    {
        std::vector<WallRef> result;
        for (int c = 0; c < MAZE_SIZE; ++c) {
            for (int r = 0; r + 1 < MAZE_SIZE; ++r) {
                const WallRef ref{true, c, r};
                if (RegionOf(ref) == region) result.push_back(ref);
            }
        }
        for (int c = 0; c + 1 < MAZE_SIZE; ++c) {
            for (int r = 0; r < MAZE_SIZE; ++r) {
                const WallRef ref{false, c, r};
                if (RegionOf(ref) == region) result.push_back(ref);
            }
        }
        return result;
    }

    // 中心格四周无墙的方向
    std::vector<Direct> CenterOpenDirects() const
    {
        std::vector<Direct> result;
        for (int d = 0; d < 4; ++d) {
            const Direct direct = static_cast<Direct>(d);
            if (!HasWall(k_center, direct)) result.push_back(direct);
        }
        return result;
    }

    // 从 from 出发能否到达 to，exclude_center 为真时不得经过中心格
    bool Reachable(const Pos& from, const Pos& to, const bool exclude_center) const
    {
        if (exclude_center && (from == k_center || to == k_center)) {
            return false;
        }
        std::array<bool, MAZE_SIZE * MAZE_SIZE> visited{};
        std::queue<Pos> que;
        que.push(from);
        visited[CellId(from)] = true;
        while (!que.empty()) {
            const Pos cur = que.front();
            que.pop();
            if (cur == to) return true;
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (HasWall(cur, direct)) continue;
                const Pos next = Move(cur, direct);
                if (exclude_center && next == k_center) continue;
                if (visited[CellId(next)]) continue;
                visited[CellId(next)] = true;
                que.push(next);
            }
        }
        return false;
    }

    // 全图是否连通：不存在任何无法抵达的封闭区域
    bool IsFullyConnected() const
    {
        std::array<bool, MAZE_SIZE * MAZE_SIZE> visited{};
        std::queue<Pos> que;
        que.push(k_start[0]);
        visited[CellId(k_start[0])] = true;
        int count = 1;
        while (!que.empty()) {
            const Pos cur = que.front();
            que.pop();
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (HasWall(cur, direct)) continue;
                const Pos next = Move(cur, direct);
                if (visited[CellId(next)]) continue;
                visited[CellId(next)] = true;
                ++count;
                que.push(next);
            }
        }
        return count == MAZE_SIZE * MAZE_SIZE;
    }

    // 从 from 到 to 的最短路径长度，不可达返回 -1
    int ShortestDistance(const Pos& from, const Pos& to) const
    {
        std::array<int, MAZE_SIZE * MAZE_SIZE> distance;
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
                if (HasWall(cur, direct)) continue;
                const Pos next = Move(cur, direct);
                if (distance[CellId(next)] >= 0) continue;
                distance[CellId(next)] = distance[CellId(cur)] + 1;
                que.push(next);
            }
        }
        return -1;
    }

    /* ========== 迷宫生成 ========== */

    // region_wall_num：ABC 与 EFG 区域各自的墙数；walls_at_start：两个起点三面各自的墙数
    bool Generate(std::mt19937& g, const int region_wall_num, const int walls_at_start_0, const int walls_at_start_1)
    {
        for (int attempt = 0; attempt < GENERATE_ATTEMPT; ++attempt) {
            Clear();
            PlaceCenterColumn(g);
            PlaceStartWalls(g, 0, walls_at_start_0);
            PlaceStartWalls(g, 1, walls_at_start_1);
            PlaceCenterSideWalls(g);
            if (!BuildSkeleton(g)) continue;
            if (!FillRegion(g, Region::ABC, region_wall_num)) continue;
            if (!FillRegion(g, Region::EFG, region_wall_num)) continue;
            if (Validate(region_wall_num, walls_at_start_0, walls_at_start_1)) return true;
        }
        return false;
    }

    // 校验迷宫是否满足全部生成约束
    bool Validate(const int region_wall_num, const int walls_at_start_0, const int walls_at_start_1) const
    {
        if (CountRegionWalls(Region::ABC) != region_wall_num) return false;
        if (CountRegionWalls(Region::EFG) != region_wall_num) return false;
        if (CountRegionWalls(Region::COL_D) != CENTER_COLUMN_WALL) return false;
        // D 列 3 面墙必须分布为中心格上方二下方一，或上方一下方二
        int upper = 0;
        for (int r = 0; r < k_center.r; ++r) {
            if (wall_h[k_center.c][r]) ++upper;
        }
        if (upper != 1 && upper != 2) return false;
        if (CountStartWalls(0) != walls_at_start_0) return false;
        if (CountStartWalls(1) != walls_at_start_1) return false;
        // 中心格至少两面无墙，且每个开口都不是死路：绕开中心格仍能通往两个起点
        const std::vector<Direct> open_directs = CenterOpenDirects();
        if (static_cast<int>(open_directs.size()) < CENTER_OPEN_MIN) return false;
        for (const Direct direct : open_directs) {
            const Pos neighbor = Move(k_center, direct);
            if (!Reachable(neighbor, k_start[0], true)) return false;
            if (!Reachable(neighbor, k_start[1], true)) return false;
        }
        // 两个起点到中心格的最短距离必须相同且不低于下限，保证双方争夺宝物的路程对等且不会过近
        const int center_distance_0 = ShortestDistance(k_start[0], k_center);
        const int center_distance_1 = ShortestDistance(k_start[1], k_center);
        if (center_distance_0 < CENTER_DISTANCE_MIN || center_distance_0 != center_distance_1) return false;
        // 全图必须连通，不得存在任何无法抵达的封闭区域
        return IsFullyConnected();
    }

    /* ========== 情报统计 ========== */

    // 情报A：全部横线或竖线上的墙壁总数
    int CountWalls(const bool horizontal) const
    {
        int count = 0;
        if (horizontal) {
            for (int c = 0; c < MAZE_SIZE; ++c) {
                for (int r = 0; r + 1 < MAZE_SIZE; ++r) {
                    if (wall_h[c][r]) ++count;
                }
            }
        } else {
            for (int c = 0; c + 1 < MAZE_SIZE; ++c) {
                for (int r = 0; r < MAZE_SIZE; ++r) {
                    if (wall_v[c][r]) ++count;
                }
            }
        }
        return count;
    }

    int CountRegionWalls(const Region region) const
    {
        int count = 0;
        for (const auto& ref : RegionPositions(region)) {
            if (WallValue(ref)) ++count;
        }
        return count;
    }

    // 指定起点三面的墙数
    int CountStartWalls(const int index) const
    {
        int count = 0;
        for (int d = 0; d < 4; ++d) {
            const auto ref = WallAt(k_start[index], static_cast<Direct>(d));
            if (ref.has_value() && WallValue(*ref)) ++count;
        }
        return count;
    }

    // 情报B / C / D：沿贯穿线从头走到尾，途中被几面墙阻挡
    int CountLineWalls(const LineRef& line) const
    {
        int count = 0;
        for (int i = 0; i + 1 < MAZE_SIZE; ++i) {
            // 横向贯穿跨越的是竖线，纵向贯穿跨越的是横线
            const WallRef ref = line.horizontal ? WallRef{false, i, line.index} : WallRef{true, line.index, i};
            if (WallValue(ref)) ++count;
        }
        return count;
    }

    // 情报F：求 from 到 to 的一条最短通路，返回沿途经过的全部格子。不可达则返回空
    // exclude_center 为真时不得经过中心格；存在多条并列最短通路时随机取一条
    std::vector<Pos> ShortestPath(const Pos& from, const Pos& to, std::mt19937& g, const bool exclude_center) const
    {
        if (exclude_center && (from == k_center || to == k_center)) {
            return {};
        }
        std::vector<int> prev(MAZE_SIZE * MAZE_SIZE, -1);
        std::array<bool, MAZE_SIZE * MAZE_SIZE> visited{};
        std::queue<Pos> que;
        que.push(from);
        visited[CellId(from)] = true;
        while (!que.empty()) {
            const Pos cur = que.front();
            que.pop();
            if (cur == to) break;
            std::array<int, 4> order = {{0, 1, 2, 3}};
            std::shuffle(order.begin(), order.end(), g);
            for (const int d : order) {
                const Direct direct = static_cast<Direct>(d);
                if (HasWall(cur, direct)) continue;
                const Pos next = Move(cur, direct);
                if (exclude_center && next == k_center) continue;
                if (visited[CellId(next)]) continue;
                visited[CellId(next)] = true;
                prev[CellId(next)] = CellId(cur);
                que.push(next);
            }
        }
        if (!visited[CellId(to)]) {
            return {};
        }
        std::vector<Pos> path;
        for (Pos cur = to; ; ) {
            path.push_back(cur);
            if (cur == from) break;
            const int prev_id = prev[CellId(cur)];
            cur = Pos{prev_id / MAZE_SIZE, prev_id % MAZE_SIZE};
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    // 旧版情报：按端点相接对墙壁分组，边框不参与相接判定
    WallGroupResult CountWallGroups() const
    {
        std::vector<WallRef> walls;
        for (int c = 0; c < MAZE_SIZE; ++c) {
            for (int r = 0; r + 1 < MAZE_SIZE; ++r) {
                if (wall_h[c][r]) walls.push_back(WallRef{true, c, r});
            }
        }
        for (int c = 0; c + 1 < MAZE_SIZE; ++c) {
            for (int r = 0; r < MAZE_SIZE; ++r) {
                if (wall_v[c][r]) walls.push_back(WallRef{false, c, r});
            }
        }

        const int wall_num = static_cast<int>(walls.size());
        std::vector<int> parent(wall_num);
        for (int i = 0; i < wall_num; ++i) parent[i] = i;
        const auto find = [&parent](auto&& self, const int x) -> int {
            return parent[x] == x ? x : (parent[x] = self(self, parent[x]));
        };
        const auto unite = [&](const int x, const int y) {
            const int fx = find(find, x);
            const int fy = find(find, y);
            if (fx != fy) parent[fx] = fy;
        };

        // 每个格点上的墙壁两两相接，格点坐标范围为 0 ~ MAZE_SIZE
        const int point_num = (MAZE_SIZE + 1) * (MAZE_SIZE + 1);
        std::vector<int> first_wall(point_num, -1);
        for (int i = 0; i < wall_num; ++i) {
            for (const auto& point : EndPoints(walls[i])) {
                const int id = point.first * (MAZE_SIZE + 1) + point.second;
                if (first_wall[id] == -1) {
                    first_wall[id] = i;
                } else {
                    unite(i, first_wall[id]);
                }
            }
        }

        std::vector<int> group_size(wall_num, 0);
        for (int i = 0; i < wall_num; ++i) {
            ++group_size[find(find, i)];
        }
        WallGroupResult result;
        for (int i = 0; i < wall_num; ++i) {
            if (group_size[i] == 1) ++result.isolated;
            else if (group_size[i] == 2) ++result.pair;
            else if (group_size[i] >= 3) ++result.large;
        }
        return result;
    }

    // 情报F：排除经过中心格的、从 from 到 to 的简单路径数量，超出统计上限返回 -1
    long long CountSimplePaths(const Pos& from, const Pos& to) const
    {
        if (from == k_center || to == k_center) {
            return 0;
        }
        std::array<bool, MAZE_SIZE * MAZE_SIZE> visited{};
        long long count = 0;
        bool overflow = false;
        const auto dfs = [&](auto&& self, const Pos& pos) -> void {
            if (pos == to) {
                if (++count >= PATH_COUNT_LIMIT) overflow = true;
                return;
            }
            visited[CellId(pos)] = true;
            for (int d = 0; d < 4 && !overflow; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (HasWall(pos, direct)) continue;
                const Pos next = Move(pos, direct);
                if (next == k_center || visited[CellId(next)]) continue;
                self(self, next);
            }
            visited[CellId(pos)] = false;
        };
        dfs(dfs, from);
        return overflow ? -1 : count;
    }

  private:
    // 生成过程中已确定的墙壁位置，确定后不再改动
    bool locked_h_[MAZE_SIZE][MAZE_SIZE - 1] = {};
    bool locked_v_[MAZE_SIZE - 1][MAZE_SIZE] = {};

    bool IsLocked(const WallRef& ref) const
    {
        return ref.horizontal ? locked_h_[ref.c][ref.r] : locked_v_[ref.c][ref.r];
    }

    // 确定一个墙壁位置的最终状态
    void SetLocked(const WallRef& ref, const bool is_wall)
    {
        SetWallValue(ref, is_wall);
        (ref.horizontal ? locked_h_[ref.c][ref.r] : locked_v_[ref.c][ref.r]) = true;
    }

    void Clear()
    {
        for (int c = 0; c < MAZE_SIZE; ++c) {
            for (int r = 0; r + 1 < MAZE_SIZE; ++r) {
                wall_h[c][r] = false;
                locked_h_[c][r] = false;
            }
        }
        for (int c = 0; c + 1 < MAZE_SIZE; ++c) {
            for (int r = 0; r < MAZE_SIZE; ++r) {
                wall_v[c][r] = false;
                locked_v_[c][r] = false;
            }
        }
    }

    // 墙壁两端的格点坐标
    static std::array<std::pair<int, int>, 2> EndPoints(const WallRef& ref)
    {
        if (ref.horizontal) {
            return {{std::make_pair(ref.c, ref.r + 1), std::make_pair(ref.c + 1, ref.r + 1)}};
        }
        return {{std::make_pair(ref.c + 1, ref.r), std::make_pair(ref.c + 1, ref.r + 1)}};
    }

    // 在候选位置中随机确定 wall_num 个为墙壁，其余确定为无墙
    void PlaceInSlots(std::mt19937& g, std::vector<WallRef> slots, const int wall_num)
    {
        std::shuffle(slots.begin(), slots.end(), g);
        for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
            SetLocked(slots[i], i < wall_num);
        }
    }

    // D 列 6 条内部横线上放置 3 面墙，中心格上方与下方按二比一或一比二分配
    void PlaceCenterColumn(std::mt19937& g)
    {
        const int upper_num = (g() % 2 == 0) ? 2 : 1;
        std::vector<WallRef> upper_slots;
        std::vector<WallRef> lower_slots;
        for (int r = 0; r < k_center.r; ++r) {
            upper_slots.push_back(WallRef{true, k_center.c, r});
        }
        for (int r = k_center.r; r + 1 < MAZE_SIZE; ++r) {
            lower_slots.push_back(WallRef{true, k_center.c, r});
        }
        PlaceInSlots(g, std::move(upper_slots), upper_num);
        PlaceInSlots(g, std::move(lower_slots), CENTER_COLUMN_WALL - upper_num);
    }

    // 起点三面按对手提交的信息A 放置墙壁
    void PlaceStartWalls(std::mt19937& g, const int index, const int wall_num)
    {
        std::vector<WallRef> slots;
        for (int d = 0; d < 4; ++d) {
            const auto ref = WallAt(k_start[index], static_cast<Direct>(d));
            if (ref.has_value()) slots.push_back(*ref);
        }
        PlaceInSlots(g, std::move(slots), wall_num);
    }

    // 决定中心格左右两面，保证中心格至少两面无墙
    void PlaceCenterSideWalls(std::mt19937& g)
    {
        int open_num = 0;
        if (!HasWall(k_center, Direct::UP)) ++open_num;
        if (!HasWall(k_center, Direct::DOWN)) ++open_num;
        std::array<Direct, 2> sides = {{Direct::LEFT, Direct::RIGHT}};
        std::shuffle(sides.begin(), sides.end(), g);
        for (int i = 0; i < static_cast<int>(sides.size()); ++i) {
            const int rest = static_cast<int>(sides.size()) - i;
            const bool must_open = (CENTER_OPEN_MIN - open_num) >= rest;
            const bool open = must_open || (g() % 2 == 0);
            SetLocked(*WallAt(k_center, sides[i]), !open);
            if (open) ++open_num;
        }
    }

    // 铺设骨架通路：绕开中心格，将除中心格外的全部格子连成一个连通块。
    // 中心格通过已确定为无墙的开口接入该连通块，因此全图必定连通，且中心格的每个开口绕开中心格后都能通往两个起点
    bool BuildSkeleton(std::mt19937& g)
    {
        std::vector<Pos> required;
        for (int c = 0; c < MAZE_SIZE; ++c) {
            for (int r = 0; r < MAZE_SIZE; ++r) {
                if (Pos{c, r} != k_center) required.push_back(Pos{c, r});
            }
        }
        std::vector<bool> in_comp(MAZE_SIZE * MAZE_SIZE, false);
        in_comp[CellId(required[0])] = true;
        for (size_t i = 1; i < required.size(); ++i) {
            if (in_comp[CellId(required[i])]) continue;
            if (!ConnectToComponent(g, required[i], in_comp)) return false;
        }
        return true;
    }

    // 从 from 出发寻找一条到达当前连通块的最短通路，并将沿途墙壁位置确定为无墙
    bool ConnectToComponent(std::mt19937& g, const Pos& from, std::vector<bool>& in_comp)
    {
        std::vector<int> prev(MAZE_SIZE * MAZE_SIZE, -1);
        std::array<bool, MAZE_SIZE * MAZE_SIZE> visited{};
        std::queue<Pos> que;
        que.push(from);
        visited[CellId(from)] = true;
        Pos target{-1, -1};
        while (!que.empty() && target.c < 0) {
            const Pos cur = que.front();
            que.pop();
            std::array<int, 4> order = {{0, 1, 2, 3}};
            std::shuffle(order.begin(), order.end(), g);
            for (const int d : order) {
                const Direct direct = static_cast<Direct>(d);
                if (!PassableForSkeleton(cur, direct)) continue;
                const Pos next = Move(cur, direct);
                if (next == k_center || visited[CellId(next)]) continue;
                visited[CellId(next)] = true;
                prev[CellId(next)] = CellId(cur);
                if (in_comp[CellId(next)]) {
                    target = next;
                    break;
                }
                que.push(next);
            }
        }
        if (target.c < 0) {
            return false;
        }
        // 回溯路径，确定沿途每一段为无墙，并将沿途格子并入连通块
        Pos cur = target;
        in_comp[CellId(cur)] = true;
        while (cur != from) {
            const int prev_id = prev[CellId(cur)];
            const Pos before{prev_id / MAZE_SIZE, prev_id % MAZE_SIZE};
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (Move(before, direct) == cur) {
                    SetLocked(*WallAt(before, direct), false);
                    break;
                }
            }
            cur = before;
            in_comp[CellId(cur)] = true;
        }
        return true;
    }

    // 骨架搜索时，仅已确定为墙壁的位置不可通过
    bool PassableForSkeleton(const Pos& pos, const Direct direct) const
    {
        const auto ref = WallAt(pos, direct);
        if (!ref.has_value()) return false;
        return !(IsLocked(*ref) && WallValue(*ref));
    }

    // 在区域剩余的待定位置中随机补足墙数
    bool FillRegion(std::mt19937& g, const Region region, const int wall_num)
    {
        std::vector<WallRef> free_positions;
        int fixed_wall = 0;
        for (const auto& ref : RegionPositions(region)) {
            if (IsLocked(ref)) {
                if (WallValue(ref)) ++fixed_wall;
            } else {
                free_positions.push_back(ref);
            }
        }
        const int rest = wall_num - fixed_wall;
        if (rest < 0 || rest > static_cast<int>(free_positions.size())) {
            return false;
        }
        std::shuffle(free_positions.begin(), free_positions.end(), g);
        for (int i = 0; i < static_cast<int>(free_positions.size()); ++i) {
            SetLocked(free_positions[i], i < rest);
        }
        return true;
    }
};
