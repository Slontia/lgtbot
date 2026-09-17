
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "constants.h"
#include "maze.h"

// 单步行动记录
struct StepRecord
{
    Direct direct = Direct::UP;
    StepResult result = StepResult::MOVE;
    std::string tag;    // 该步触发的额外事件，如夺宝、击晕
};

// 单回合行动记录
struct RoundRecord
{
    int round = 0;
    std::vector<StepRecord> steps;
    std::string summary;    // 回合结算摘要
};


class Player
{
  public:
    Player(const PlayerID pid, const std::string& name, const std::string& avatar, const int index)
        : pid(pid), name(name), avatar(avatar), index(index), pos(k_start[index]) {}

    /* ========== 基础信息 ========== */
    const PlayerID pid;
    const std::string name;
    const std::string avatar;
    const int index;            // 起点编号：0 为 A3，1 为 G5

    Pos Start() const { return k_start[index]; }
    Pos OpponentStart() const { return k_start[1 - index]; }

    /* ========== 开局提交与情报 ========== */
    int info_a = -1;                    // 提交的信息A：为对手起点三面放置的墙数
    int info_b = -1;                    // 提交的信息B
    std::vector<Intel> intels;          // 选择的情报
    std::vector<std::string> answers;   // 情报对应的答复

    bool HasSubmitInfo() const { return info_a >= 0 && info_b >= 0; }
    bool HasSubmitIntel() const { return static_cast<int>(intels.size()) == INTEL_COUNT; }
    bool HasChosenIntel(const Intel intel) const
    {
        return std::find(intels.begin(), intels.end(), intel) != intels.end();
    }

    /* ========== 局面状态 ========== */
    Pos pos;                    // 当前位置
    bool treasure = false;      // 是否携带宝物
    bool quit = false;          // 已退出或已判负，不再参与后续游戏

    // 走过的格子：两个起点不计入
    bool visited[MAZE_SIZE][MAZE_SIZE] = {};
    int visited_count = 0;

    // 已发现的墙壁：仅记录自己撞到过的
    bool known_h[MAZE_SIZE][MAZE_SIZE - 1] = {};
    bool known_v[MAZE_SIZE - 1][MAZE_SIZE] = {};

    // 已走通的通路：仅记录自己实际穿过的位置，据此确认该处无墙
    bool passed_h[MAZE_SIZE][MAZE_SIZE - 1] = {};
    bool passed_v[MAZE_SIZE - 1][MAZE_SIZE] = {};

    // 情报揭示的格子：已知可通行，但并未真正走过，不计入走过的格子数
    bool revealed[MAZE_SIZE][MAZE_SIZE] = {};

    // 上回合结束时所在的格子。戒指被取得后，该位置会在回合开始时向对手公开
    Pos last_round_pos;
    bool has_last_round_pos = false;

    /* ========== 回合内状态 ========== */
    int steps_used = 0;                             // 本回合已消耗步数
    StopReason stop_reason = StopReason::NONE;      // 本回合停止原因
    bool moved_this_step = false;                   // 本步是否发生了位移，用于相遇判定

    /* ========== 跨回合状态 ========== */
    bool pending_stun = false;      // 本回合被击晕，下回合强制停止
    bool stun_rest = false;         // 本回合因上回合被击晕而强制停止
    bool pending_forbid = false;    // 本回合击晕对手，下回合禁止进入指定格
    Pos pending_forbid_pos;
    bool forbid = false;            // 本回合存在禁止进入的格子
    Pos forbid_pos;

    /* ========== 行动记录 ========== */
    RoundRecord record;
    std::vector<RoundRecord> all_records;

    /* ========== 状态查询与更新 ========== */

    bool IsStopped() const { return stop_reason != StopReason::NONE; }

    // 本回合是否需要提交行动
    bool NeedAct() const { return !quit && !stun_rest && !IsStopped(); }

    // 记录到访格子，返回是否为新增格子（两个起点不计入）
    bool MarkVisited(const Pos& target)
    {
        if (IsStart(target) || visited[target.c][target.r]) {
            return false;
        }
        visited[target.c][target.r] = true;
        ++visited_count;
        return true;
    }

    // 记录撞到的墙壁
    void DiscoverWall(const WallRef& ref)
    {
        (ref.horizontal ? known_h[ref.c][ref.r] : known_v[ref.c][ref.r]) = true;
    }

    bool IsKnownWall(const WallRef& ref) const
    {
        return ref.horizontal ? known_h[ref.c][ref.r] : known_v[ref.c][ref.r];
    }

    // 记录走通的通路
    void PassWall(const WallRef& ref)
    {
        (ref.horizontal ? passed_h[ref.c][ref.r] : passed_v[ref.c][ref.r]) = true;
    }

    bool IsPassedWall(const WallRef& ref) const
    {
        return ref.horizontal ? passed_h[ref.c][ref.r] : passed_v[ref.c][ref.r];
    }

    // 玩家视角下，指定格子的指定方向是否已知存在墙壁（边框始终已知）
    bool KnowsWall(const Pos& target, const Direct direct) const
    {
        const auto ref = Maze::WallAt(target, direct);
        return !ref.has_value() || IsKnownWall(*ref);
    }

    // 回合开始：结算跨回合状态并重置回合内状态
    void BeginRound(const int round)
    {
        stun_rest = pending_stun;
        pending_stun = false;
        forbid = pending_forbid;
        forbid_pos = pending_forbid_pos;
        pending_forbid = false;

        steps_used = 0;
        moved_this_step = false;
        stop_reason = StopReason::NONE;
        if (quit) {
            stop_reason = StopReason::QUIT;
        } else if (stun_rest) {
            stop_reason = StopReason::STUN_REST;
        }

        record = RoundRecord{};
        record.round = round;
    }

    // 回合结束：归档本回合记录
    void EndRound(const std::string& summary)
    {
        record.summary = summary;
        all_records.push_back(record);
    }

    void AddStep(const Direct direct, const StepResult result, const std::string& tag = "")
    {
        record.steps.push_back(StepRecord{direct, result, tag});
    }

    // 为本回合最后一步补充事件标记
    void AppendTag(const std::string& tag)
    {
        if (record.steps.empty()) {
            return;
        }
        if (!record.steps.back().tag.empty()) {
            record.steps.back().tag += "/";
        }
        record.steps.back().tag += tag;
    }
};
