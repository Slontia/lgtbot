
#pragma once

#include <string>
#include <vector>

#include "constants.h"
#include "maze.h"

// 单步行动记录
struct StepRecord
{
    Direct direct = Direct::UP;
    StepResult result = StepResult::MOVE;
};

// 单回合行动记录
struct TurnRecord
{
    std::vector<StepRecord> steps;
};


class Player
{
  public:
    Player(const lgtbot::PlayerID pid, const std::string& name, const std::string& avatar)
        : pid(pid), name(name), avatar(avatar) {}

    /* ========== 基础信息 ========== */
    const lgtbot::PlayerID pid;
    const std::string name;
    const std::string avatar;

    /* ========== 绘制阶段 ========== */
    Maze maze;
    bool submitted = false;

    /* ========== 对战阶段 ========== */
    // 自己位于对手绘制的迷宫上
    Pos pawn;
    // 走过的格子，起点计入
    bool visited[MAX_MAZE_SIZE][MAX_MAZE_SIZE] = {};

    // 已退出或已判负，不再参与后续游戏
    bool quit = false;

    /* ========== 行动记录 ========== */
    // 本回合的行动轨迹，用于回合结算时的文字提示
    TurnRecord record;

    // 对战开始：将玩家放到起点
    void StartBattle(const Pos& start)
    {
        pawn = start;
        for (int c = 0; c < MAX_MAZE_SIZE; ++c) {
            for (int r = 0; r < MAX_MAZE_SIZE; ++r) {
                visited[c][r] = false;
            }
        }
        MarkVisited(start);
    }

    void MarkVisited(const Pos& target) { visited[target.c][target.r] = true; }

    // 回合开始：重置本回合的行动轨迹
    void BeginTurn() { record = TurnRecord{}; }

    void AddStep(const Direct direct, const StepResult result)
    {
        record.steps.push_back(StepRecord{direct, result});
    }
};
