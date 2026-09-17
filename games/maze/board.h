
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "utility/html.h"

#include "constants.h"
#include "maze.h"
#include "player.h"

// 墙壁的展示口径
enum class WallView {
    PUBLIC,     // 公开视角：只展示已撞出的墙与已走通的位置，其余一律未探明
    TRUTH,      // 全知视角：额外用蓝色标出尚未撞出的墙，其余位置确认无墙
};

// 单张迷宫的绘制选项
struct MazeOptions
{
    int owner = 0;                      // 迷宫的绘制者
    WallView wall_view = WallView::PUBLIC;
    bool with_pawn = true;              // 是否绘制挑战者的小人与足迹
};


class Board
{
  public:
    Board(const std::vector<Player>& players, const int& size, const Pos& start, const Pos& goal)
        : players_(players), size_(size), start_(start), goal_(goal) {}

    /* ========== 图片宽度 ========== */
    static int MazeWidth(const int size) { return (GRID_SIZE + WALL_SIZE) * size + WALL_SIZE; }
    static int ImageWidth(const int size) { return MazeWidth(size) + 160; }
    static int DualImageWidth(const int size) { return MazeWidth(size) * 2 + 240; }

    /* ========== 绘制阶段 ========== */
    // 私信给绘制者本人：自己的迷宫全貌
    std::string GetDrawView(const PlayerID pid, const int wall_limit) const
    {
        const Player& self = players_[pid];
        const int count = self.maze.WallCount();
        std::string tips = "<div class=\"tips\">已放置 <b>" + std::to_string(count) + "</b> / " +
                std::to_string(wall_limit) + " 面墙</div><div class=\"tips\">";
        if (count > wall_limit) {
            tips += "<font color=red>已超出上限 " + std::to_string(count - wall_limit) + " 面，需要移除后才能提交</font>";
        } else if (!self.maze.Reachable(start_, goal_)) {
            tips += "<font color=red>起点与终点之间没有通路，无法提交</font>";
        } else {
            tips += "<font color=green>起点与终点之间存在通路，可以提交</font>";
        }
        tips += "</div>";
        return Style() + "<div class=\"title\">你绘制的迷宫</div>" + tips +
                GetMaze(MazeOptions{static_cast<int>(pid), WallView::TRUTH, false}) + GetDrawLegend();
    }

    // 公屏：只展示提交进度与不含任何墙壁的空白底图
    std::string GetDrawPublic() const
    {
        html::Table table(static_cast<uint32_t>(players_.size()) + 1, 3);
        table.SetTableStyle("align=\"center\" cellpadding=\"3\" cellspacing=\"0\" border=\"1\"");
        table.Get(0, 0).SetColor(COLOR_HEADER).SetStyle("style=\"width:50px;\"");
        table.Get(0, 1).SetColor(COLOR_HEADER).SetStyle("style=\"width:260px;\"").SetContent("玩家");
        table.Get(0, 2).SetColor(COLOR_HEADER).SetStyle("style=\"width:140px;\"").SetContent("绘制进度");
        for (size_t i = 0; i < players_.size(); ++i) {
            const auto row = static_cast<uint32_t>(i) + 1;
            table.Get(row, 0).SetContent(players_[i].avatar);
            table.Get(row, 1).SetStyle("style=\"text-align:left;\"").SetContent(players_[i].name);
            if (players_[i].quit) {
                table.Get(row, 2).SetColor("#E5E5E5").SetContent("已退出");
            } else if (players_[i].submitted) {
                table.Get(row, 2).SetColor("#D9F2D9").SetContent("已提交");
            } else {
                table.Get(row, 2).SetContent("绘制中…");
            }
        }
        return Style() + "<div class=\"title\">绘制迷宫</div>" + table.ToString() + GetPointLine() + GetBlankMaze();
    }

    // 开局公屏底图：只标出格子编号与双方共用的起点、终点
    std::string GetOpeningBoard() const
    {
        return Style() + "<div class=\"title\">本局迷宫</div>" + GetPointLine() + GetBlankMaze();
    }

    /* ========== 对战阶段 ========== */
    // 双方迷宫左右并排。viewer 为 -1 时只展示公开信息，私信赛况额外展示自己绘制的墙
    std::string GetDualBoard(const int viewer, const int turn, const int cur_pid) const
    {
        std::string title = "<div class=\"title\">迷宫 · 第 " + std::to_string(turn) + " 回合</div>";
        // 私信视角下自己绘制的墙体会以蓝色出现
        return Style() + title + GetStatusTable(cur_pid) + "<div class=\"gap\"></div>" +
                GetCompare(viewer, WallView::PUBLIC) +
                GetLegend(viewer >= 0 ? LegendKind::PRIVATE : LegendKind::PUBLIC);
    }

    // 单张迷宫：challenger 正在挑战的那一张
    std::string GetSingleBoard(const PlayerID challenger, const int viewer) const
    {
        const int owner = 1 - static_cast<int>(challenger);
        const bool truth = (viewer == owner);
        return Style() + MazeTitle(players_[owner].name) +
                GetMaze(MazeOptions{owner, truth ? WallView::TRUTH : WallView::PUBLIC, true});
    }

    // 终局：公开双方全部墙壁，已撞出的为黑色，全程隐藏的为蓝色
    std::string GetFinalBoard() const
    {
        return Style() + "<div class=\"title\">【终局】双方迷宫全貌</div>" + GetStatusTable(-1) +
                "<div class=\"gap\"></div>" + GetCompare(-1, WallView::TRUTH) + GetLegend(LegendKind::FINAL);
    }

    /* ========== 文本 ========== */
    // 单回合行动轨迹的文本形式
    static std::string GetTurnTrace(const TurnRecord& record)
    {
        if (record.steps.empty()) {
            return "（本回合未行动）";
        }
        std::string trace;
        for (const auto& step : record.steps) {
            const std::string arrow(dir_arrow[static_cast<int>(step.direct)]);
            switch (step.result) {
                case StepResult::HIT_WALL: trace += "(" + arrow + "墙)"; break;
                case StepResult::GOAL:     trace += arrow + std::string("[终点]"); break;
                case StepResult::MOVE:     trace += arrow; break;
            }
        }
        return trace;
    }

  private:
    /* ========== 配色 ========== */
    static constexpr const char* COLOR_WALL = "#2B2B2B";        // 已撞出的墙与地图边界
    static constexpr const char* COLOR_HIDDEN = "#3F6FD8";      // 尚未被撞出的墙
    static constexpr const char* COLOR_PASSABLE = "#FFFFFF";    // 已确认可通行的位置
    static constexpr const char* COLOR_UNKNOWN = "#BFBFBF";     // 尚未探明的位置
    static constexpr const char* COLOR_GRID = "#E4E4E4";        // 尚未走过的格子
    static constexpr const char* COLOR_VISITED = "#E4EEE4";     // 走过的格子
    static constexpr const char* COLOR_START = "#D6E7FF";       // 起点
    static constexpr const char* COLOR_GOAL = "#FFD9D9";        // 终点
    static constexpr const char* COLOR_HEADER = "#F2F2F2";

    /* ========== 迷宫绘制 ========== */
    std::string GetMaze(const MazeOptions& options) const
    {
        const Maze& maze = players_[options.owner].maze;
        const Player& challenger = players_[1 - options.owner];
        const uint32_t n = static_cast<uint32_t>(size_) * 2 + 1;
        html::Table map(n, n);
        map.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" style=\"border-collapse: collapse;\"");
        // 格子
        for (int c = 0; c < size_; ++c) {
            for (int r = 0; r < size_; ++r) {
                const auto [color, content] = GridStyle(Pos{c, r}, options, challenger);
                map.Get(r * 2 + 1, c * 2 + 1).SetStyle("class=\"grid\"").SetColor(color).SetContent(content);
            }
        }
        // 横向墙壁，最上与最下为地图边界
        for (int c = 0; c < size_; ++c) {
            for (int r = 0; r <= size_; ++r) {
                const bool border = (r == 0 || r == size_);
                map.Get(r * 2, c * 2 + 1).SetStyle("class=\"wall-row\"")
                    .SetColor(border ? COLOR_WALL : WallColor(maze, WallRef{true, c, r - 1}, options.wall_view));
            }
        }
        // 纵向墙壁，最左与最右为地图边界
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c <= size_; ++c) {
                const bool border = (c == 0 || c == size_);
                map.Get(r * 2 + 1, c * 2).SetStyle("class=\"wall-col\"")
                    .SetColor(border ? COLOR_WALL : WallColor(maze, WallRef{false, c - 1, r}, options.wall_view));
            }
        }
        // 交点
        for (int c = 0; c <= size_; ++c) {
            for (int r = 0; r <= size_; ++r) {
                map.Get(r * 2, c * 2).SetStyle("class=\"corner\"").SetColor(COLOR_WALL);
            }
        }
        return map.ToString();
    }

    // 不含任何墙壁信息的空白底图，只标出格子编号与起点、终点
    std::string GetBlankMaze() const
    {
        const uint32_t n = static_cast<uint32_t>(size_) * 2 + 1;
        html::Table map(n, n);
        map.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" style=\"border-collapse: collapse;\"");
        for (int c = 0; c < size_; ++c) {
            for (int r = 0; r < size_; ++r) {
                const Pos cell{c, r};
                std::string color = COLOR_GRID;
                std::string mark;
                if (cell == start_) {
                    color = COLOR_START;
                    mark = "<b>起</b>";
                } else if (cell == goal_) {
                    color = COLOR_GOAL;
                    mark = "<b>终</b>";
                }
                map.Get(r * 2 + 1, c * 2 + 1).SetStyle("class=\"grid\"").SetColor(color).SetContent(CellBox(cell, mark));
            }
        }
        for (int c = 0; c < size_; ++c) {
            for (int r = 0; r <= size_; ++r) {
                const bool border = (r == 0 || r == size_);
                map.Get(r * 2, c * 2 + 1).SetStyle("class=\"wall-row\"").SetColor(border ? COLOR_WALL : COLOR_UNKNOWN);
            }
        }
        for (int r = 0; r < size_; ++r) {
            for (int c = 0; c <= size_; ++c) {
                const bool border = (c == 0 || c == size_);
                map.Get(r * 2 + 1, c * 2).SetStyle("class=\"wall-col\"").SetColor(border ? COLOR_WALL : COLOR_UNKNOWN);
            }
        }
        for (int c = 0; c <= size_; ++c) {
            for (int r = 0; r <= size_; ++r) {
                map.Get(r * 2, c * 2).SetStyle("class=\"corner\"").SetColor(COLOR_WALL);
            }
        }
        return map.ToString();
    }

    // 迷宫上方的标题
    static std::string MazeTitle(const std::string& owner_name)
    {
        return "<div class=\"maze-title\">" HTML_SIZE_FONT_HEADER(4) "<b>" + owner_name + " 的迷宫</b>" HTML_FONT_TAIL "</div>";
    }

    // 两张迷宫左右并排
    std::string GetCompare(const int viewer, const WallView public_view) const
    {
        html::Table compare(2, static_cast<uint32_t>(players_.size()));
        compare.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\"");
        for (size_t i = 0; i < players_.size(); ++i) {
            const auto column = static_cast<uint32_t>(i);
            compare.Get(0, column).SetContent(MazeTitle(players_[i].name));
            // 自己绘制的迷宫对自己全部可见
            const WallView view = (viewer >= 0 && static_cast<size_t>(viewer) == i) ? WallView::TRUTH : public_view;
            compare.Get(1, column).SetStyle("style=\"padding: 10px 16px 10px 16px\"")
                .SetContent(GetMaze(MazeOptions{static_cast<int>(i), view, true}));
        }
        return compare.ToString();
    }

    /* ========== 单元格 ========== */
    // 格子内容：编号常驻左上角，标记居中
    std::string CellBox(const Pos& cell, const std::string& mark) const
    {
        return "<div class=\"cellbox\"><span class=\"num\">" + std::to_string(PosToId(cell, size_)) +
                "</span><span class=\"mark\">" + mark + "</span></div>";
    }

    std::pair<std::string, std::string> GridStyle(const Pos& cell, const MazeOptions& options,
            const Player& challenger) const
    {
        std::string color = COLOR_GRID;
        std::string mark;
        if (cell == start_) {
            color = COLOR_START;
            mark = "<b>起</b>";
        } else if (cell == goal_) {
            color = COLOR_GOAL;
            mark = "<b>终</b>";
        }
        if (options.with_pawn) {
            if (cell != start_ && cell != goal_ && challenger.visited[cell.c][cell.r]) {
                color = COLOR_VISITED;
            }
            if (challenger.pawn == cell) {
                mark = challenger.avatar.empty() ? "<b>●</b>" : challenger.avatar;
            }
        }
        return {color, CellBox(cell, mark)};
    }

    // 墙壁位置的颜色
    std::string WallColor(const Maze& maze, const WallRef& ref, const WallView view) const
    {
        if (maze.IsRevealed(ref)) {
            return COLOR_WALL;
        }
        if (view == WallView::TRUTH) {
            return maze.WallValue(ref) ? COLOR_HIDDEN : COLOR_PASSABLE;
        }
        return maze.IsPassed(ref) ? COLOR_PASSABLE : COLOR_UNKNOWN;
    }

    /* ========== 信息表 ========== */
    std::string GetStatusTable(const int cur_pid) const
    {
        html::Table table(static_cast<uint32_t>(players_.size()) + 1, 4);
        table.SetTableStyle("align=\"center\" cellpadding=\"3\" cellspacing=\"0\" border=\"1\"");
        table.Get(0, 0).SetColor(COLOR_HEADER).SetStyle("style=\"width:50px;\"");
        table.Get(0, 1).SetColor(COLOR_HEADER).SetStyle("style=\"width:240px;\"").SetContent("玩家");
        table.Get(0, 2).SetColor(COLOR_HEADER).SetStyle("style=\"width:100px;\"").SetContent("所在位置");
        table.Get(0, 3).SetColor(COLOR_HEADER).SetStyle("style=\"width:110px;\"").SetContent("撞出墙数");
        for (size_t i = 0; i < players_.size(); ++i) {
            const auto row = static_cast<uint32_t>(i) + 1;
            const Player& player = players_[i];
            // 该玩家撞出的墙位于对手绘制的迷宫上
            const Maze& challenged = players_[1 - i].maze;
            table.Get(row, 0).SetContent(player.avatar);
            std::string name = player.name;
            if (cur_pid >= 0 && static_cast<size_t>(cur_pid) == i) {
                name = "<b>▶ " + name + "</b>";
            }
            if (player.quit) {
                name += "<font color=red>（已退出）</font>";
            }
            table.Get(row, 1).SetStyle("style=\"text-align:left;\"").SetContent(name);
            table.Get(row, 2).SetContent(std::to_string(PosToId(player.pawn, size_)) + " 号");
            table.Get(row, 3).SetContent(std::to_string(challenged.RevealedCount()) + " 面");
        }
        return table.ToString();
    }

    std::string GetPointLine() const
    {
        return "<div class=\"tips\">起点 <b>" + std::to_string(PosToId(start_, size_)) + " 号格</b>　终点 <b>" +
                std::to_string(PosToId(goal_, size_)) + " 号格</b></div>";
    }

    /* ========== 图例 ========== */
    enum class LegendKind {
        PUBLIC,     // 对战公屏：只有公开信息
        PRIVATE,    // 对战私信：额外出现自己绘制的墙
        FINAL,      // 终局：全部墙体公开，不存在未探明的位置
    };

    static std::string GetDrawLegend()
    {
        const std::pair<const char*, const char*> items[] = {
            {COLOR_START, "起点"},
            {COLOR_GOAL, "终点"},
            {COLOR_HIDDEN, "你放置的墙"},
        };
        return MakeLegend(items, sizeof(items) / sizeof(items[0]), 3);
    }

    static std::string GetLegend(const LegendKind kind = LegendKind::PUBLIC)
    {
        if (kind == LegendKind::FINAL) {
            const std::pair<const char*, const char*> items[] = {
                {COLOR_START, "起点"},
                {COLOR_GOAL, "终点"},
                {COLOR_VISITED, "走过格子"},
                {COLOR_WALL, "撞出的墙"},
                {COLOR_HIDDEN, "隐藏的墙"},
                {COLOR_PASSABLE, "无墙"},
            };
            return MakeLegend(items, sizeof(items) / sizeof(items[0]), 3);
        }
        if (kind == LegendKind::PRIVATE) {
            const std::pair<const char*, const char*> items[] = {
                {COLOR_START, "起点"},
                {COLOR_GOAL, "终点"},
                {COLOR_VISITED, "走过格子"},
                {COLOR_WALL, "撞出的墙"},
                {COLOR_HIDDEN, "绘制的墙"},
                {COLOR_PASSABLE, "已知通路"},
                {COLOR_UNKNOWN, "尚未探明"},
            };
            return MakeLegend(items, sizeof(items) / sizeof(items[0]), 4);
        }
        const std::pair<const char*, const char*> items[] = {
            {COLOR_START, "起点"},
            {COLOR_GOAL, "终点"},
            {COLOR_VISITED, "走过格子"},
            {COLOR_WALL, "撞出的墙"},
            {COLOR_PASSABLE, "已知通路"},
            {COLOR_UNKNOWN, "尚未探明"},
        };
        return MakeLegend(items, sizeof(items) / sizeof(items[0]), 3);
    }

    static std::string MakeLegend(const std::pair<const char*, const char*>* const items, const uint32_t item_num,
            const uint32_t column)
    {
        html::Table legend((item_num + column - 1) / column, column);
        legend.SetTableStyle("align=\"center\" cellpadding=\"2\" cellspacing=\"0\"");
        const std::string width = std::to_string(column >= 4 ? 150 : 130);
        for (uint32_t i = 0; i < item_num; ++i) {
            std::string content = "<span class=\"swatch\" style=\"background:";
            content += items[i].first;
            content += ";\"></span>";
            content += items[i].second;
            legend.Get(i / column, i % column).SetStyle("style=\"width:" + width + "px; text-align:left;\"")
                .SetContent(content);
        }
        return "<div class=\"legend\">" + legend.ToString() + "</div>";
    }

    static std::string Style()
    {
        return R"(
<style>
    .grid {
        width: )" + std::to_string(GRID_SIZE) + R"(px;
        height: )" + std::to_string(GRID_SIZE) + R"(px;
        padding: 0;
    }
    .cellbox {
        position: relative;
        width: )" + std::to_string(GRID_SIZE) + R"(px;
        height: )" + std::to_string(GRID_SIZE) + R"(px;
    }
    .num {
        position: absolute;
        top: 1px;
        left: 4px;
        font-size: 13px;
        line-height: 1;
        color: #8A8A8A;
    }
    .mark {
        position: absolute;
        top: 0;
        left: 0;
        width: )" + std::to_string(GRID_SIZE) + R"(px;
        height: )" + std::to_string(GRID_SIZE) + R"(px;
        display: flex;
        align-items: center;
        justify-content: center;
        line-height: 1;
        font-size: 24px;
    }
    .wall-row {
        width: )" + std::to_string(GRID_SIZE) + R"(px;
        height: )" + std::to_string(WALL_SIZE) + R"(px;
    }
    .wall-col {
        width: )" + std::to_string(WALL_SIZE) + R"(px;
        height: )" + std::to_string(GRID_SIZE) + R"(px;
    }
    .corner {
        width: )" + std::to_string(WALL_SIZE) + R"(px;
        height: )" + std::to_string(WALL_SIZE) + R"(px;
    }
    .title {
        text-align: center;
        font-size: 26px;
        font-weight: bold;
        margin: 6px 0;
    }
    .maze-title {
        text-align: center;
        margin: 4px 0;
    }
    .tips {
        text-align: center;
        font-size: 18px;
        margin: 4px 0;
    }
    .gap {
        height: 18px;
    }
    .legend {
        text-align: center;
        font-size: 16px;
        margin: 8px 0;
    }
    .swatch {
        display: inline-block;
        width: 16px;
        height: 16px;
        border: 1px solid #999999;
        vertical-align: middle;
        margin-right: 4px;
    }
</style>
)";
    }

    const std::vector<Player>& players_;
    const int& size_;
    const Pos& start_;
    const Pos& goal_;
};
