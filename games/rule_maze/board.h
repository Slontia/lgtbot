
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "utility/html.h"

#include "constants.h"
#include "maze.h"
#include "player.h"

// 绘制盘面时需要的公共局面信息
struct ViewInfo
{
    int round = 0;
    int max_round = 0;
    bool treasure_at_center = true;
    int treasure_freeze = 0;
    bool no_encounter = false;
    // 是否公开对手上回合结束的位置。戒指被取得的当回合尚不公开，需等到下一回合开始
    bool reveal_rival_last_pos = false;
};

// 迷宫绘制选项
struct MazeOptions
{
    int wall_pid = -1;              // -1 绘制真实墙壁，否则仅绘制该玩家已探明的墙壁
    int path_pid = -1;              // 以该玩家的视角绘制起点、目标、轨迹与当前位置
    bool treasure_at_center = true; // 宝物是否仍在中心格
    bool hide_walls = false;        // 为真时不绘制任何内部墙壁，仅保留地图边框
    unsigned reveal_mask = 0;       // 位掩码，第 i 位为 1 表示在公开视角下展示 i 号玩家的当前位置
    int ghost_pid = -1;             // 额外标出该玩家上回合结束时的位置，-1 表示不标注
    const std::vector<LineRef>* highlight_lines = nullptr;  // 需要高亮的贯穿线，用于情报阶段示意图
};


class Board
{
  public:
    Board(const Maze& maze, const std::vector<Player>& players, std::string image_path)
        : maze_(maze), players_(players), image_path_(std::move(image_path)) {}

    // 宝物图标：透明底图片，不会遮盖格子底色
    std::string RingImage(const bool taken, const int size) const
    {
        const std::string url = ToFileUrl(image_path_ + (taken ? "ring_gray.png" : "ring.png"));
        return "<img src=\"" + url + "\" style=\"width:" + std::to_string(size) + "px; height:" +
                std::to_string(size) + "px; vertical-align:middle;\">";
    }

    // 迷宫格内的玩家标记：携带戒指时在头像右上角叠加一个戒指角标
    std::string PlayerMarker(const Player& player) const
    {
        const std::string avatar = player.avatar.empty() ? "<b>●</b>" : player.avatar;
        if (!player.treasure) {
            return avatar;
        }
        return "<span class=\"marker\">" + avatar + "<span class=\"badge\">" + RingImage(false, 24) + "</span></span>";
    }

    static int ImageWidth() { return (GRID_SIZE + WALL_SIZE) * MAZE_SIZE + WALL_SIZE + LABEL_SIZE * 2 + 60; }
    static int TextImageWidth() { return 700; }

    // 玩家私人视角：状态表 + 自己已探明的迷宫
    std::string GetPlayerView(const PlayerID pid, const ViewInfo& info) const
    {
        MazeOptions options{static_cast<int>(pid), static_cast<int>(pid), info.treasure_at_center};
        // 戒指被取得之后，回合开始尚未行动时可以看到对手上回合结束的位置，一旦行动便不再显示
        if (ShowRivalGhost(pid, info)) {
            options.ghost_pid = static_cast<int>(1 - pid);
        }
        return Style() + GetPlayerStatus(pid, info) + GetMaze(options) + GetLegend();
    }

    // 情报阶段示意图：在公开底图上高亮本局抽到的贯穿线
    std::string GetIntelLineBoard(const std::vector<LineRef>& lines, const ViewInfo& info) const
    {
        MazeOptions options{-1, -1, info.treasure_at_center, true, AllPlayerMask()};
        options.highlight_lines = &lines;
        std::string title = "<div class=\"title\">本局 B / C / D 的贯穿线</div><div class=\"tips\">";
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) {
                title += "　";
            }
            title += static_cast<char>('B' + i);
            title += "：" + LineName(lines[i]);
        }
        title += "</div>";
        return Style() + title + GetMaze(options);
    }

    // 开局公屏底图：不包含任何墙壁信息，仅展示双方起点与水晶戒指
    std::string GetOpeningBoard(const ViewInfo& info) const
    {
        const MazeOptions options{-1, -1, info.treasure_at_center, true, AllPlayerMask()};
        return Style() + "<div class=\"title\">本局迷宫</div>" + GetPublicTable(info, false) +
                GetMaze(options) + GetOpeningLegend();
    }

    // 夺宝公屏提醒图：仅公开取得戒指的玩家位于中心格，不公开对手位置与任何墙壁信息
    std::string GetTreasureBoard(const PlayerID pid, const ViewInfo& info) const
    {
        const MazeOptions options{-1, -1, info.treasure_at_center, true, 1u << pid};
        return Style() + "<div class=\"title\">水晶戒指已被取得</div>" + GetPublicTable(info, false) +
                GetMaze(options) + GetOpeningLegend();
    }

    // 回合结束公屏位置图：戒指被取得后公开双方所在格，但不公开步数与走过的格子
    std::string GetPositionBoard(const ViewInfo& info) const
    {
        const MazeOptions options{-1, -1, info.treasure_at_center, true, AllPlayerMask()};
        return Style() + GetPublicTable(info) + "<div class=\"tips\">下图公开双方当前所在位置</div>"+
                GetMaze(options) + GetOpeningLegend();
    }

    // 公屏赛况：不含任何地图信息
    std::string GetPublicStatus(const ViewInfo& info, const std::string& record) const
    {
        return Style() + GetPublicTable(info) + GetRecordBlock(record);
    }

    // 终局：公开真实迷宫与双方轨迹
    std::string GetFinalBoard(const ViewInfo& info, const std::string& record) const
    {
        html::Table compare(2, static_cast<uint32_t>(players_.size()));
        compare.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\"");
        for (size_t i = 0; i < players_.size(); ++i) {
            const auto column = static_cast<uint32_t>(i);
            compare.Get(0, column).SetContent(HTML_SIZE_FONT_HEADER(4) "<b>" + PlayerLabel(players_[i]) +
                    "</b>" HTML_FONT_TAIL);
            compare.Get(1, column).SetStyle("style=\"padding: 10px 16px 10px 16px\"")
                .SetContent(GetMaze(MazeOptions{-1, static_cast<int>(i), info.treasure_at_center}));
        }
        return Style() + GetPublicTable(info, true, true) +
                "<div class=\"title\">【终局迷宫】下图为迷宫的真实全貌，两侧分别展示双方走过的格子</div>" +
                compare.ToString() + GetLegend() + GetRecordBlock(record);
    }

    // 单回合行动轨迹的文本形式
    static std::string GetRoundTrace(const RoundRecord& record)
    {
        if (record.steps.empty()) {
            return "（本回合未行动）";
        }
        std::string trace;
        for (const auto& step : record.steps) {
            if (!trace.empty()) {
                trace += " ";
            }
            const std::string arrow(dir_arrow[static_cast<int>(step.direct)]);
            switch (step.result) {
                case StepResult::HIT_WALL: trace += "(" + arrow + "墙)"; break;
                case StepResult::NEW_GRID: trace += arrow + std::string("+"); break;
                case StepResult::MOVE:     trace += arrow; break;
            }
            if (!step.tag.empty()) {
                trace += "[" + step.tag + "]";
            }
        }
        return trace;
    }

  private:
    /* ========== 配色 ========== */
    static constexpr const char* COLOR_WALL = "#2B2B2B";        // 已探明的墙壁与地图边框
    static constexpr const char* COLOR_PASSABLE = "#FFFFFF";    // 已确认可通行的位置
    static constexpr const char* COLOR_UNKNOWN = "#BFBFBF";     // 尚未探明的墙壁位置
    static constexpr const char* COLOR_GRID = "#E4E4E4";        // 尚未到过的格子
    static constexpr const char* COLOR_EMPTY = "#F7F7F7";       // 终局全揭示视角下没走过的空地
    static constexpr const char* COLOR_VISITED = "#E4EEE4";     // 走过的格子
    static constexpr const char* COLOR_START = "#D6E7FF";       // 自己的起点
    static constexpr const char* COLOR_GOAL = "#FFD9D9";        // 对手的起点，即胜利目标
    static constexpr const char* COLOR_CENTER = "#FFF0B8";      // 中心格 D4
    static constexpr const char* COLOR_FORBID = "#E6DAF5";      // 本回合禁止进入的格子
    static constexpr const char* COLOR_LINE = "#FFE680";        // 情报示意图中被高亮的贯穿线格子
    static constexpr const char* COLOR_LINE_WALL = "#C08A1E";   // 情报示意图中贯穿线跨越的墙壁位置
    static constexpr const char* COLOR_HEADER = "#F2F2F2";

    // 绘制迷宫
    std::string GetMaze(const MazeOptions& options) const
    {
        const uint32_t n = MAZE_SIZE * 2 + 3;
        html::Table map(n, n);
        map.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" style=\"border-collapse: collapse;\"");
        // 四周坐标标注：字母为列，数字为行
        for (int c = 0; c < MAZE_SIZE; ++c) {
            const std::string letter(1, static_cast<char>('A' + c));
            map.Get(0, c * 2 + 2).SetStyle("class=\"pos\"").SetContent(letter);
            map.Get(n - 1, c * 2 + 2).SetStyle("class=\"pos\"").SetContent(letter);
        }
        for (int r = 0; r < MAZE_SIZE; ++r) {
            const std::string digit = std::to_string(r + 1);
            map.Get(r * 2 + 2, 0).SetStyle("class=\"pos\"").SetContent(digit);
            map.Get(r * 2 + 2, n - 1).SetStyle("class=\"pos\"").SetContent(digit);
        }
        // 格子
        for (int c = 0; c < MAZE_SIZE; ++c) {
            for (int r = 0; r < MAZE_SIZE; ++r) {
                const auto [color, content] = GridStyle(Pos{c, r}, options);
                map.Get(r * 2 + 2, c * 2 + 2).SetStyle("class=\"grid\"").SetColor(color).SetContent(content);
            }
        }
        // 横向墙壁，最上与最下为地图边框
        for (int c = 0; c < MAZE_SIZE; ++c) {
            for (int r = 0; r <= MAZE_SIZE; ++r) {
                const bool border = (r == 0 || r == MAZE_SIZE);
                map.Get(r * 2 + 1, c * 2 + 2).SetStyle("class=\"wall-row\"")
                    .SetColor(border ? COLOR_WALL : WallColor(WallRef{true, c, r - 1}, options));
            }
        }
        // 纵向墙壁，最左与最右为地图边框
        for (int r = 0; r < MAZE_SIZE; ++r) {
            for (int c = 0; c <= MAZE_SIZE; ++c) {
                const bool border = (c == 0 || c == MAZE_SIZE);
                map.Get(r * 2 + 2, c * 2 + 1).SetStyle("class=\"wall-col\"")
                    .SetColor(border ? COLOR_WALL : WallColor(WallRef{false, c - 1, r}, options));
            }
        }
        // 交点
        for (int c = 0; c <= MAZE_SIZE; ++c) {
            for (int r = 0; r <= MAZE_SIZE; ++r) {
                map.Get(r * 2 + 1, c * 2 + 1).SetStyle("class=\"corner\"").SetColor(COLOR_WALL);
            }
        }
        return map.ToString();
    }

    // 是否应向该玩家展示对手上回合结束的位置：已进入公开阶段、有记录、且本回合尚未行动
    bool ShowRivalGhost(const PlayerID pid, const ViewInfo& info) const
    {
        const Player& self = players_[pid];
        const Player& rival = players_[1 - pid];
        return info.reveal_rival_last_pos && rival.has_last_round_pos && self.steps_used == 0;
    }

    // 格子是否位于该贯穿线上
    static bool CellOnLine(const Pos& cell, const LineRef& line)
    {
        return line.horizontal ? cell.r == line.index : cell.c == line.index;
    }

    // 墙壁位置是否被该贯穿线跨越：横向贯穿跨越竖线，纵向贯穿跨越横线
    static bool WallOnLine(const WallRef& ref, const LineRef& line)
    {
        return line.horizontal ? (!ref.horizontal && ref.r == line.index)
                               : (ref.horizontal && ref.c == line.index);
    }

    // 公开视角下同格时该显示哪位玩家：持有戒指者优先，其次是击晕对手的一方，
    // 双方条件完全相同时只显示编号更小的玩家。该格无人则返回 -1
    int RevealPriorityPid(const Pos& cell, const unsigned reveal_mask) const
    {
        int best = -1;
        for (size_t i = 0; i < players_.size(); ++i) {
            if (!(reveal_mask & (1u << i)) || players_[i].pos != cell) {
                continue;
            }
            if (best < 0 || BetterRevealCandidate(players_[i], players_[best])) {
                best = static_cast<int>(i);
            }
        }
        return best;
    }

    // lhs 是否比 rhs 更应当被显示。两者相同则返回 false，从而保留编号更小的一方
    static bool BetterRevealCandidate(const Player& lhs, const Player& rhs)
    {
        if (lhs.treasure != rhs.treasure) {
            return lhs.treasure;                            // 持有戒指者优先
        }
        if (lhs.pending_stun != rhs.pending_stun) {
            return !lhs.pending_stun;                       // 被击晕的一方不显示
        }
        if (lhs.visited_count != rhs.visited_count) {
            return lhs.visited_count > rhs.visited_count;   // 走过格子更多者即击晕方
        }
        return false;
    }

    // 墙壁位置的绘制颜色：已探明为墙壁则涂深色，已确认可通行则涂白，尚未探明则涂灰
    std::string WallColor(const WallRef& ref, const MazeOptions& options) const
    {
        // 贯穿线跨越的墙壁位置用土黄色标出，即该情报统计的 6 个位置
        if (options.highlight_lines != nullptr) {
            for (const auto& line : *options.highlight_lines) {
                if (WallOnLine(ref, line)) {
                    return COLOR_LINE_WALL;
                }
            }
        }
        if (options.hide_walls) {
            return COLOR_UNKNOWN;
        }
        if (options.wall_pid < 0) {
            return maze_.WallValue(ref) ? COLOR_WALL : COLOR_PASSABLE;
        }
        const Player& player = players_[options.wall_pid];
        if (player.IsKnownWall(ref)) {
            return COLOR_WALL;
        }
        return player.IsPassedWall(ref) ? COLOR_PASSABLE : COLOR_UNKNOWN;
    }

    // 格子的底色与内容
    std::pair<std::string, std::string> GridStyle(const Pos& cell, const MazeOptions& options) const
    {
        const bool revealed = (options.wall_pid < 0 && !options.hide_walls);
        std::string color = revealed ? COLOR_EMPTY : COLOR_GRID;
        std::string content;
        const bool is_center = (cell == k_center);
        if (options.path_pid < 0) {
            // 公开视角：底色只标出中心格与两个起点
            if (is_center) {
                color = COLOR_CENTER;
                content = RingImage(!options.treasure_at_center, GRID_SIZE - 8);
            } else if (IsStart(cell)) {
                // 有人持有戒指时，终点始终是持有者的目标格，即其对手的起点
                const int holder = TreasureHolder();
                if (holder >= 0 && cell == players_[holder].OpponentStart()) {
                    color = COLOR_GOAL;
                    content = "<b>终</b>";
                } else {
                    color = COLOR_START;
                    content = "<b>起</b>";
                }
            } else if (options.highlight_lines != nullptr) {
                // 贯穿线经过的普通格子高亮，中心格与起点保留原有底色以免丢失地标
                for (const auto& line : *options.highlight_lines) {
                    if (CellOnLine(cell, line)) {
                        color = COLOR_LINE;
                        break;
                    }
                }
            }
            // 被指定公开位置的玩家显示在其当前所在格，双方同格时只显示优先级更高的一位
            const int reveal_pid = RevealPriorityPid(cell, options.reveal_mask);
            if (reveal_pid >= 0) {
                content = PlayerMarker(players_[reveal_pid]);
            }
            return {color, content};
        }
        const Player& player = players_[options.path_pid];
        const bool is_goal = (cell == player.OpponentStart());
        const bool is_self_start = (cell == player.Start());
        const bool is_forbid = (player.forbid && cell == player.forbid_pos);
        const bool visited = player.visited[cell.c][cell.r];
        if (is_center) {
            color = COLOR_CENTER;
        } else if (is_goal) {
            color = COLOR_GOAL;
        } else if (is_self_start) {
            color = COLOR_START;
        } else if (is_forbid) {
            color = COLOR_FORBID;
        } else if (visited) {
            color = COLOR_VISITED;
        } else if (player.revealed[cell.c][cell.r]) {
            color = COLOR_EMPTY;
        }
        // 对手上回合结束的位置：仅在回合开始尚未行动时展示，用半透明头像表示这是旧情报
        const bool is_ghost = (options.ghost_pid >= 0 && players_[options.ghost_pid].last_round_pos == cell);
        if (player.pos == cell) {
            content = PlayerMarker(player);
        } else if (is_ghost) {
            content = "<span class=\"ghost\">" + PlayerMarker(players_[options.ghost_pid]) + "</span>";
        } else if (is_center) {
            content = RingImage(!options.treasure_at_center, GRID_SIZE - 8);
        } else if (is_goal) {
            content = "<b>终</b>";
        } else if (is_self_start) {
            content = "<b>起</b>";
        } else if (is_forbid) {
            content = "<b>✕</b>";
        } else if (visited) {
            content = "<span class=\"dot\">•</span>";
        }
        return {color, content};
    }

    // 玩家私人视角的状态表
    std::string GetPlayerStatus(const PlayerID pid, const ViewInfo& info) const
    {
        const Player& self = players_[pid];
        const Player& rival = players_[1 - pid];
        html::Table table(3, 5);
        table.SetTableStyle("align=\"center\" cellpadding=\"3\" cellspacing=\"0\" border=\"1\"");
        table.Get(0, 0).SetColor(COLOR_HEADER).SetStyle("style=\"width:50px;\"");
        table.Get(0, 1).SetColor(COLOR_HEADER).SetStyle("style=\"width:220px;\"").SetContent("玩家");
        table.Get(0, 2).SetColor(COLOR_HEADER).SetStyle("style=\"width:90px;\"").SetContent("当前位置");
        table.Get(0, 3).SetColor(COLOR_HEADER).SetStyle("style=\"width:100px;\"").SetContent("走过格子");
        table.Get(0, 4).SetColor(COLOR_HEADER).SetStyle("style=\"width:80px;\"").SetContent("宝物");
        table.Get(1, 0).SetContent(self.avatar);
        table.Get(1, 1).SetStyle("style=\"text-align:left;\"").SetContent("<b>你</b>　" + self.name);
        table.Get(1, 2).SetContent(PosName(self.pos));
        table.Get(1, 3).SetContent(std::to_string(self.visited_count));
        table.Get(1, 4).SetContent(self.treasure ? RingImage(false, 32) : "－");
        table.Get(2, 0).SetContent(rival.avatar);
        table.Get(2, 1).SetStyle("style=\"text-align:left;\"").SetContent("对手　" + rival.name);
        table.Get(2, 2).SetContent("？");
        table.Get(2, 3).SetContent("？");
        table.Get(2, 4).SetContent(rival.treasure ? RingImage(false, 32) : "－");

        std::string tips = "<b>本回合已行动 " + std::to_string(self.steps_used) + " / " +
                std::to_string(MAX_STEP) + " 步</b>";
        if (self.stun_rest) {
            tips += "　<font color=red>本回合被击晕，强制停止行动</font>";
        }
        if (self.forbid) {
            tips += "　<font color=red>本回合不得进入 " + PosName(self.forbid_pos) + "，且第一步不得撞墙</font>";
        }
        if (info.no_encounter) {
            tips += "　<font color=teal>本回合不触发相遇判定</font>";
        }
        // 进入公开阶段后，持续告知对手上回合结束时的位置
        std::string rival_line;
        if (info.reveal_rival_last_pos && rival.has_last_round_pos) {
            rival_line = "<div class=\"tips\">对手 " + rival.name + " 上回合结束时位于 " + PosName(rival.last_round_pos);
        }
        return GetTitle(info) + table.ToString() + "<div class=\"tips\">" + tips + "</div>" +
                GetTreasureLine(info) + rival_line;
    }

    // 公屏可见的信息表。走过的格子数属于对局中的机密，仅在终局公开
    std::string GetPublicTable(const ViewInfo& info, const bool with_title = true, const bool with_visited = false) const
    {
        const uint32_t treasure_col = with_visited ? 4 : 3;
        html::Table table(static_cast<uint32_t>(players_.size()) + 1, treasure_col + 1);
        table.SetTableStyle("align=\"center\" cellpadding=\"3\" cellspacing=\"0\" border=\"1\"");
        table.Get(0, 0).SetColor(COLOR_HEADER).SetStyle("style=\"width:50px;\"");
        table.Get(0, 1).SetColor(COLOR_HEADER).SetStyle("style=\"width:240px;\"").SetContent("玩家");
        table.Get(0, 2).SetColor(COLOR_HEADER).SetStyle("style=\"width:90px;\"").SetContent("起点");
        if (with_visited) {
            table.Get(0, 3).SetColor(COLOR_HEADER).SetStyle("style=\"width:100px;\"").SetContent("走过格子");
        }
        table.Get(0, treasure_col).SetColor(COLOR_HEADER).SetStyle("style=\"width:80px;\"").SetContent("宝物");
        for (size_t i = 0; i < players_.size(); ++i) {
            const auto row = static_cast<uint32_t>(i) + 1;
            table.Get(row, 0).SetContent(players_[i].avatar);
            table.Get(row, 1).SetStyle("style=\"text-align:left;\"").SetContent(PlayerLabel(players_[i]));
            table.Get(row, 2).SetContent(PosName(players_[i].Start()));
            if (with_visited) {
                table.Get(row, 3).SetContent(std::to_string(players_[i].visited_count) + " 格");
            }
            table.Get(row, treasure_col).SetContent(players_[i].treasure ? RingImage(false, 32) : "－");
        }
        return (with_title ? GetTitle(info) : "") + table.ToString() + GetTreasureLine(info);
    }

    // 当前持有戒指的玩家序号，无人持有返回 -1
    int TreasureHolder() const
    {
        for (size_t i = 0; i < players_.size(); ++i) {
            if (players_[i].treasure) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    // 全部玩家的位掩码
    unsigned AllPlayerMask() const
    {
        unsigned mask = 0;
        for (size_t i = 0; i < players_.size(); ++i) {
            mask |= 1u << i;
        }
        return mask;
    }

    static std::string PlayerLabel(const Player& player)
    {
        return "[" + PosName(player.Start()) + "] " + player.name;
    }

    static std::string GetTitle(const ViewInfo& info)
    {
        return "<div class=\"title\">第 " + std::to_string(info.round) + " / " + std::to_string(info.max_round) + " 回合</div>";
    }

    std::string GetTreasureLine(const ViewInfo& info) const
    {
        std::string text = RingImage(false, 26) + "【水晶戒指】";
        const Player* holder = nullptr;
        for (const auto& player : players_) {
            if (player.treasure) {
                holder = &player;
            }
        }
        if (holder != nullptr) {
            text += "由 " + PlayerLabel(*holder) + " 携带";
        } else if (info.treasure_at_center) {
            text += "仍在中心格 " + PosName(k_center);
        } else {
            text += "状态异常";
        }
        if (info.treasure_freeze > 0) {
            text += "　<font color=red>（冻结中，本回合无法被任何人取得）</font>";
        }
        return "<div class=\"tips\">" + text + "</div>";
    }

    static std::string GetRecordBlock(const std::string& record)
    {
        if (record.empty()) {
            return "";
        }
        return "<div class=\"record\"><b>【公开事件记录】</b><br>" + record + "</div>";
    }

    // 公开底图图例：只说明公开信息。有人持有戒指时额外说明终点
    std::string GetOpeningLegend() const
    {
        std::vector<std::pair<const char*, const char*>> items = {
            {COLOR_START, "起始位置"},
            {COLOR_CENTER, "中心 D4"},
            {COLOR_GRID, "尚未探明"},
        };
        if (TreasureHolder() >= 0) {
            items.insert(items.begin() + 1, {COLOR_GOAL, "戒指终点"});
        }
        const uint32_t column = static_cast<uint32_t>(items.size());
        const std::string width = std::to_string(column >= 4 ? 135 : 180);
        html::Table legend(1, column);
        legend.SetTableStyle("align=\"center\" cellpadding=\"2\" cellspacing=\"0\"");
        for (uint32_t i = 0; i < column; ++i) {
            std::string content = "<span class=\"swatch\" style=\"background:";
            content += items[i].first;
            content += ";\"></span>";
            content += items[i].second;
            legend.Get(0, i).SetStyle("style=\"width:" + width + "px; text-align:left;\"").SetContent(content);
        }
        return "<div class=\"legend\">" + legend.ToString() + "</div>";
    }

    static std::string GetLegend()
    {
        const std::pair<const char*, const char*> items[] = {
            {COLOR_START, "自己起点"},
            {COLOR_GOAL, "对手起点·目标"},
            {COLOR_CENTER, "中心 D4"},
            {COLOR_FORBID, "禁止进入"},
            {COLOR_VISITED, "走过的格子"},
            {COLOR_PASSABLE, "已知通路"},
            {COLOR_WALL, "已知墙壁"},
            {COLOR_UNKNOWN, "尚未探明"},
        };
        const uint32_t item_num = sizeof(items) / sizeof(items[0]);
        const uint32_t column = 4;
        html::Table legend((item_num + column - 1) / column, column);
        legend.SetTableStyle("align=\"center\" cellpadding=\"2\" cellspacing=\"0\"");
        for (uint32_t i = 0; i < item_num; ++i) {
            std::string content = "<span class=\"swatch\" style=\"background:";
            content += items[i].first;
            content += ";\"></span>";
            content += items[i].second;
            legend.Get(i / column, i % column).SetStyle("style=\"width:135px; text-align:left;\"").SetContent(content);
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
        font-size: 22px;
        line-height: 1;
        text-align: center;
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
    .dot {
        font-size: 34px;
        color: #333333;
        line-height: 1;
    }
    .marker {
        position: relative;
        display: inline-block;
        line-height: 0;
    }
    .ghost {
        display: inline-block;
        opacity: 0.5;
    }
    .badge {
        position: absolute;
        top: -6px;
        right: -6px;
        line-height: 0;
    }
    .pos {
        width: )" + std::to_string(LABEL_SIZE) + R"(px;
        height: )" + std::to_string(LABEL_SIZE) + R"(px;
        font-size: 20px;
        color: #666666;
        text-align: center;
    }
    .title {
        text-align: center;
        font-size: 26px;
        font-weight: bold;
        margin: 6px 0;
    }
    .tips {
        text-align: center;
        font-size: 18px;
        margin: 4px 0;
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
    .record {
        font-size: 17px;
        line-height: 1.5;
        margin: 10px 0;
        padding: 8px;
        background: #F5F7FA;
        border-left: 4px solid #4C7AAF;
    }
</style>
)";
    }

    const Maze& maze_;
    const std::vector<Player>& players_;
    const std::string image_path_;
};
