
#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>

/* ========== 常量 ========== */
// 迷宫边长：字母 A~G 表示从左到右的 7 列，数字 1~7 表示从上到下的 7 行
inline constexpr int MAZE_SIZE = 7;
// 每回合最大行动步数
inline constexpr int MAX_STEP = 5;

// 信息A 的取值范围：对手起点三面的墙数
inline constexpr int INFO_A_MIN = 0;
inline constexpr int INFO_A_MAX = 2;
// 信息B 的取值范围：双方之和决定单个区域的墙数
// 上限受全图连通性限制：7x7 共 84 个墙位，全连通至少需要 48 条通路，
// 加上中心格至少两面无墙，墙数最多 35 面，即双方信息B 之和最多 16
inline constexpr int INFO_B_MIN = 2;
inline constexpr int INFO_B_MAX = 7;
// 每位玩家选择的情报条数
inline constexpr int INTEL_COUNT = 3;
// D 列内部横线上的墙壁总数
inline constexpr int CENTER_COLUMN_WALL = 3;
// 中心格必须保留的无墙方向数
inline constexpr int CENTER_OPEN_MIN = 2;
// 起点到中心格的最短距离下限。网格图中路径长度与曼哈顿距离同奇偶，
// 两个起点到中心格的曼哈顿距离均为 4，因此实际最短距离必为偶数
inline constexpr int CENTER_DISTANCE_MIN = 6;
// 迷宫生成的最大尝试次数。墙数最少时，同时满足全连通与起点距离下限的组合较为稀有，
// 需要足够的重试次数；单次尝试耗时约 20 微秒，取满也在毫秒级
inline constexpr int GENERATE_ATTEMPT = 20000;
// 情报F 的路径统计上限，超出后不再精确计数
inline constexpr long long PATH_COUNT_LIMIT = 100000LL;

// 绘图尺寸
inline constexpr int GRID_SIZE = 56;
inline constexpr int WALL_SIZE = 10;
inline constexpr int LABEL_SIZE = 34;


/* ========== 方向 ========== */
enum class Direct { UP, DOWN, LEFT, RIGHT };

// 各方向对应的列/行增量
inline constexpr int k_DC_Direct[4] = {0, 0, -1, 1};
inline constexpr int k_DR_Direct[4] = {-1, 1, 0, 0};

inline constexpr std::string_view dir_cn[4] = {"上", "下", "左", "右"};
inline constexpr std::string_view dir_arrow[4] = {"↑", "↓", "←", "→"};

inline const std::map<std::string, Direct> direction_map = {
    {"上", Direct::UP},    {"U", Direct::UP},    {"s", Direct::UP},
    {"下", Direct::DOWN},  {"D", Direct::DOWN},  {"x", Direct::DOWN},
    {"左", Direct::LEFT},  {"L", Direct::LEFT},  {"z", Direct::LEFT},
    {"右", Direct::RIGHT}, {"R", Direct::RIGHT}, {"y", Direct::RIGHT},
};

inline Direct Opposite(const Direct direct)
{
    switch (direct) {
        case Direct::UP:    return Direct::DOWN;
        case Direct::DOWN:  return Direct::UP;
        case Direct::LEFT:  return Direct::RIGHT;
        case Direct::RIGHT: return Direct::LEFT;
    }
    return Direct::UP;
}


/* ========== 坐标 ========== */
struct Pos
{
    int c = 0;  // 列：A~G 对应 0~6
    int r = 0;  // 行：1~7 对应 0~6

    bool operator==(const Pos& other) const { return c == other.c && r == other.r; }
    bool operator!=(const Pos& other) const { return !(*this == other); }
};

// 中心宝物格 D4
inline constexpr Pos k_center{3, 3};
// 两个起点：0 号位为 A3，1 号位为 G5
inline constexpr Pos k_start[2] = {Pos{0, 2}, Pos{6, 4}};

inline bool InMaze(const Pos& pos)
{
    return pos.c >= 0 && pos.c < MAZE_SIZE && pos.r >= 0 && pos.r < MAZE_SIZE;
}

// 是否为任意一方的起点
inline bool IsStart(const Pos& pos)
{
    return pos == k_start[0] || pos == k_start[1];
}

// 坐标转字符串，如 {0, 2} → "A3"
inline std::string PosName(const Pos& pos)
{
    return std::string(1, static_cast<char>('A' + pos.c)) + std::to_string(pos.r + 1);
}

// 解析坐标字符串，成功返回 true。大小写不敏感，如 "A3"、"a3"
inline bool ParsePos(const std::string& str, Pos& out)
{
    if (str.length() != 2) {
        return false;
    }
    char letter = str[0];
    if (letter >= 'a' && letter <= 'z') {
        letter = static_cast<char>(letter - 'a' + 'A');
    }
    if (letter < 'A' || letter >= static_cast<char>('A' + MAZE_SIZE)) {
        return false;
    }
    if (str[1] < '1' || str[1] >= static_cast<char>('1' + MAZE_SIZE)) {
        return false;
    }
    out.c = letter - 'A';
    out.r = str[1] - '1';
    return true;
}


/* ========== 贯穿线 ========== */
// 贯穿线：沿一整行或一整列从头走到尾，用于情报B/C/D
struct LineRef
{
    bool horizontal = true; // true：沿某一行横向贯穿；false：沿某一列纵向贯穿
    int index = 0;          // 横向时为行号，纵向时为列号
};

// 全部候选贯穿线的数量：7 行加 7 列
inline constexpr int LINE_CANDIDATE_NUM = MAZE_SIZE * 2;
// 每局抽取并写入情报B/C/D 的贯穿线数量
inline constexpr int LINE_INTEL_NUM = 3;

// 抽取序号转贯穿线：前 MAZE_SIZE 个为横向，其后为纵向
inline LineRef MakeLine(const int candidate)
{
    return candidate < MAZE_SIZE ? LineRef{true, candidate} : LineRef{false, candidate - MAZE_SIZE};
}


/* ========== 情报 ========== */
enum class Intel {
    WALL_COUNT,     // A：全部横线与竖线上的墙数，两者分别给出
    LINE_1,         // B：本局随机抽取的贯穿线一
    LINE_2,         // C：本局随机抽取的贯穿线二
    LINE_3,         // D：本局随机抽取的贯穿线三
    PATHS,          // E：排除经过 D4 后，到达对方起点的通路数
    SHORTCUT,       // F：自己附近两点之间的一条短通路，会直接标注在地图上
    OPPONENT_A,     // G：对手提交的信息A
};

inline constexpr int INTEL_TYPE_NUM = 7;

inline const std::map<std::string, Intel> intel_map = {
    {"A", Intel::WALL_COUNT},   {"a", Intel::WALL_COUNT},
    {"B", Intel::LINE_1},       {"b", Intel::LINE_1},
    {"C", Intel::LINE_2},       {"c", Intel::LINE_2},
    {"D", Intel::LINE_3},       {"d", Intel::LINE_3},
    {"E", Intel::PATHS},        {"e", Intel::PATHS},
    {"F", Intel::SHORTCUT},     {"f", Intel::SHORTCUT},
    {"G", Intel::OPPONENT_A},   {"g", Intel::OPPONENT_A},
};

// 情报B/C/D 对应的贯穿线序号：0 为 B，1 为 C，2 为 D。非贯穿线情报返回 -1
inline int LineIntelIndex(const Intel intel)
{
    switch (intel) {
        case Intel::LINE_1: return 0;
        case Intel::LINE_2: return 1;
        case Intel::LINE_3: return 2;
        default:            return -1;
    }
}

// 情报F：捷径通路的长度上限
inline constexpr int SHORTCUT_MAX_LENGTH = 8;
// 情报F 的两组端点，关于中心格点对称，按起点编号取离自己更近的一组
inline constexpr Pos k_shortcut[2][2] = {{Pos{1, 1}, Pos{3, 1}}, {Pos{3, 5}, Pos{5, 5}}};

inline std::string IntelCode(const Intel intel)
{
    return std::string(1, static_cast<char>('A' + static_cast<int>(intel)));
}

// 贯穿线的展示名称，如 A1→A7
inline std::string LineName(const LineRef& line)
{
    if (line.horizontal) {
        return PosName(Pos{0, line.index}) + "→" + PosName(Pos{MAZE_SIZE - 1, line.index}) + "（横向）";
    }
    return PosName(Pos{line.index, 0}) + "→" + PosName(Pos{line.index, MAZE_SIZE - 1}) + "（纵向）";
}

inline std::string IntelTitle(const Intel intel)
{
    switch (intel) {
        case Intel::WALL_COUNT: return "全部横线与竖线上的墙壁总数";
        case Intel::LINE_1:
        case Intel::LINE_2:
        case Intel::LINE_3:     return "贯穿线上阻挡的墙数";
        case Intel::PATHS:      return "排除经过 D4 后可到达对方起点的通路数";
        case Intel::SHORTCUT:   return "自己附近的一段捷径通路";
        case Intel::OPPONENT_A: return "对手提交的信息A";
    }
    return "[未知情报]";
}

// 情报选项的固定说明，用于规则指令。B/C/D 的贯穿线与 F 的端点在游戏中动态填入
inline const char* const k_intel_detail =
    "A：全部横线格与全部竖线格上各有多少个墙（两者分别告知）\n"
    "B/C/D：本局随机抽取 3 条贯穿线（一整行或一整列算一条，每局不同但双方一致），分别告知沿该线从头走到尾会被几面墙阻挡（0~6）\n"
    "E：排除经过 D4 的通路，一共有几条通路可以到达对方的起点\n"
    "F：B2与D2 或 D6与F6 离你更近的两点之间若存在不经过 D4 长度且不超过 8 步的通路，则获知通路并标注在你的地图上\n"
    "G：对手的信息 A 是多少，也就是对方在你起点周围放置了多少墙";


/* ========== 玩家名称 ========== */
// 提取玩家昵称：框架给出的完整名称形如 <昵称(ID)>，展示时只保留昵称部分
inline std::string PlayerNickname(const std::string& name)
{
    if (name.empty()) {
        return name;
    }
    size_t begin = 0;
    size_t end = name.size() - 1;
    if (name[begin] == '<') {
        ++begin;
    }
    if (name[end] == '>') {
        const size_t paren = name.rfind('(', end);
        if (paren != std::string::npos && paren > begin) {
            end = paren - 1;
        } else if (end > begin) {
            --end;  // 没有括号时仅去掉末尾的尖括号
        }
    }
    if (begin > end) {
        return name;
    }
    return name.substr(begin, end - begin + 1);
}


/* ========== 资源路径 ========== */
// 根据系统将本地路径转为文件 URL
inline std::string ToFileUrl(const std::string& path)
{
    std::string url = path;
    std::replace(url.begin(), url.end(), '\\', '/');
#ifdef _WIN32
    if (url.size() >= 2 && url[1] == ':') {
        url = "file:///" + url;
    } else {
        url = "file://" + url;
    }
#else
    url = "file://" + url;
#endif
    return url;
}


/* ========== 玩家行动结果 ========== */
enum class StepResult {
    MOVE,       // 正常移动
    NEW_GRID,   // 移动至从未到过的格子
    HIT_WALL,   // 撞墙（含边框），强制停止
};

// 回合内玩家的停止原因
enum class StopReason {
    NONE,       // 尚未停止
    WALL,       // 撞墙
    STEP_OUT,   // 步数耗尽
    STUNNED,    // 被击晕
    KNOCKOUT,   // 击晕对手后停止
    STUN_REST,  // 上回合被击晕，本回合强制停止
    QUIT,       // 已退出或已判负
};


/* ========== 工具函数 ========== */
inline std::string ReplaceBrWithLine(std::string str)
{
    const std::string from = "<br>";
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), "\n");
        pos += 1;
    }
    return str;
}
