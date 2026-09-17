
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

/* ========== 常量 ========== */
inline constexpr int MAX_MAZE_SIZE = 9;
// 墙数达到该值时，若绘制时限仍为默认值则自动延长
inline constexpr int LONG_DRAW_WALL_NUM = 25;
inline constexpr uint32_t DEFAULT_DRAW_TIME = 360;
inline constexpr uint32_t LONG_DRAW_TIME = 720;
// 单条多步指令允许携带的最大方向数
inline constexpr int MAX_MULTI_STEP = 50;

// 电脑行动时改走次优路线的概率（百分比），其余情况按最优路线行动
inline constexpr int COMPUTER_ALT_PATH_PERCENT = 25;
// 绘图尺寸
inline constexpr int GRID_SIZE = 56;
inline constexpr int WALL_SIZE = 10;

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

// 解析单个方向字符，识别失败返回 false
inline bool ParseDirect(const std::string& text, Direct& out)
{
    const auto it = direction_map.find(text);
    if (it == direction_map.end()) {
        return false;
    }
    out = it->second;
    return true;
}

// 解析连续的方向串，如「上上左」「sxz」「UDL」
inline bool ParseDirectSequence(const std::string& text, std::vector<Direct>& out, std::string& err)
{
    out.clear();
    size_t i = 0;
    while (i < text.size()) {
        bool matched = false;
        for (const size_t len : {size_t{3}, size_t{1}}) {
            if (i + len > text.size()) {
                continue;
            }
            const auto it = direction_map.find(text.substr(i, len));
            if (it == direction_map.end()) {
                continue;
            }
            out.push_back(it->second);
            i += len;
            matched = true;
            break;
        }
        if (!matched) {
            // 按 UTF-8 截取无法识别的那个字符，避免把多字节字符拆开
            size_t len = 1;
            const unsigned char lead = static_cast<unsigned char>(text[i]);
            if ((lead & 0xF8) == 0xF0) {
                len = 4;
            } else if ((lead & 0xF0) == 0xE0) {
                len = 3;
            } else if ((lead & 0xE0) == 0xC0) {
                len = 2;
            }
            err = "[错误] 无法识别的方向「" + text.substr(i, len) + "」，可用方向：上下左右 / UDLR / sxzy";
            return false;
        }
    }
    if (out.empty()) {
        err = "[错误] 请至少给出一个移动方向，如「上」或「上上左」";
        return false;
    }
    if (static_cast<int>(out.size()) > MAX_MULTI_STEP) {
        err = "[错误] 单条指令最多包含 " + std::to_string(MAX_MULTI_STEP) + " 个方向";
        return false;
    }
    return true;
}


/* ========== 坐标 ========== */
struct Pos
{
    int c = 0;  // 列，从左到右由 0 开始
    int r = 0;  // 行，从上到下由 0 开始

    bool operator==(const Pos& other) const { return c == other.c && r == other.r; }
    bool operator!=(const Pos& other) const { return !(*this == other); }
};

inline bool InMaze(const Pos& pos, const int size)
{
    return pos.c >= 0 && pos.c < size && pos.r >= 0 && pos.r < size;
}

// 格子编号与坐标的互转。编号自左上角起按行优先从 1 开始递增
inline Pos IdToPos(const int id, const int size)
{
    return Pos{(id - 1) % size, (id - 1) / size};
}

inline int PosToId(const Pos& pos, const int size)
{
    return pos.r * size + pos.c + 1;
}

inline std::string PosName(const Pos& pos, const int size)
{
    return std::to_string(PosToId(pos, size)) + " 号格";
}

// 单步移动结果
enum class StepResult {
    MOVE,       // 正常移动至相邻格
    GOAL,       // 移动至终点，立即获胜
    HIT_WALL,   // 撞上尚未暴露的墙体，该墙转为公开，回合结束
};

// 提取玩家昵称：只保留昵称部分
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
            --end;
        }
    }
    if (begin > end) {
        return name;
    }
    return name.substr(begin, end - begin + 1);
}
