#pragma once

// =============================================================================
//  终极井字 电脑 AI
//  算法：带 Alpha-Beta 剪枝的 Negamax（极小化极大）搜索 + 启发式局面评估。
//  这是该棋类公认的通用 AI 思路（minimax/alpha-beta + 子盘控制、连线威胁、中心权重等启发式）
// =============================================================================

#include <array>
#include <vector>
#include <utility>
#include <algorithm>

namespace ai {

// 各格 / 子盘所处的连线条数（中心 4、角 3、边 2），用作位置权重与走法排序键
static constexpr int LINE_WEIGHT[9] = {3, 2, 3, 2, 4, 2, 3, 2, 3};

// 取胜分值，远大于任何启发式评估，保证“能赢就赢”
static constexpr int WIN_SCORE = 1000000;

// 轻量搜索状态：仅含规则字段，POD 拷贝快、不含字符串。
// 落子与胜负判定逻辑与 Board::Place / Board::Check 完全一致。
struct State
{
    int board[9][9];
    int bigboard[9];
    int forced;

    // 落子并推进：返回 -1=继续 / mark=该方赢得整局 / DRAW=大盘平局
    int Apply(int big, int idx, int mark)
    {
        board[big][idx] = mark;
        forced = idx;
        const int r = Check_(big);
        if (r != -1) {
            forced = -1;
            return r;
        }
        if (bigboard[idx] != EMPTY) {
            forced = -1;
        }
        return -1;
    }

    int Check_(int i)
    {
        for (const auto& L : WIN_LINES) {
            const int a = board[i][L[0]];
            if (a != EMPTY && a == board[i][L[1]] && a == board[i][L[2]]) {
                bigboard[i] = a;
                break;
            }
        }
        if (bigboard[i] == EMPTY) {
            bool full = true;
            for (int j = 0; j < 9; ++j) {
                if (board[i][j] == EMPTY) { full = false; break; }
            }
            if (full) {
                bigboard[i] = DRAW;
            }
        }
        for (const auto& L : WIN_LINES) {
            const int a = bigboard[L[0]];
            if (a != EMPTY && a != DRAW && a == bigboard[L[1]] && a == bigboard[L[2]]) {
                return a;
            }
        }
        for (int k = 0; k < 9; ++k) {
            if (bigboard[k] == EMPTY) {
                return -1;
            }
        }
        return DRAW;
    }
};

// 收集合法落子点（遵循 forced 限制）
inline std::vector<std::pair<int, int>> LegalMoves(const State& s)
{
    std::vector<std::pair<int, int>> moves;
    for (int b = 0; b < 9; ++b) {
        if (s.forced != -1 && s.forced != b) {
            continue;
        }
        if (s.bigboard[b] != EMPTY) {
            continue;
        }
        for (int j = 0; j < 9; ++j) {
            if (s.board[b][j] == EMPTY) {
                moves.emplace_back(b, j);
            }
        }
    }
    return moves;
}

// 中心优先的走法排序键，使 Alpha-Beta 更早剪枝
inline void OrderMoves(std::vector<std::pair<int, int>>& moves)
{
    std::sort(moves.begin(), moves.end(), [](const auto& a, const auto& b) {
        return LINE_WEIGHT[a.first] + LINE_WEIGHT[a.second] > LINE_WEIGHT[b.first] + LINE_WEIGHT[b.second];
    });
}

// 进行中小盘对 me 的内部潜力（连线威胁 + 中心格）
inline int SubScore(const State& s, int b, int me)
{
    const int opp = 3 - me;
    int sc = 0;
    for (const auto& L : WIN_LINES) {
        int m = 0, o = 0;
        for (int k = 0; k < 3; ++k) {
            const int v = s.board[b][L[k]];
            if (v == me) ++m;
            else if (v == opp) ++o;
        }
        if (o == 0 && m > 0) sc += (m == 2 ? 5 : 1);   // 仅我方、且差一子 → 威胁更大
        else if (m == 0 && o > 0) sc -= (o == 2 ? 5 : 1);
    }
    if (s.board[b][4] == me) sc += 2;
    else if (s.board[b][4] == opp) sc -= 2;
    return sc;
}

// 启发式评估非终局局面（从 me 视角，零和：Evaluate(s,me) == -Evaluate(s,opp)）
inline int Evaluate(const State& s, int me)
{
    const int opp = 3 - me;
    int score = 0;
    for (int b = 0; b < 9; ++b) {
        if (s.bigboard[b] == me) score += 25 * LINE_WEIGHT[b];
        else if (s.bigboard[b] == opp) score -= 25 * LINE_WEIGHT[b];
        else if (s.bigboard[b] == EMPTY) score += LINE_WEIGHT[b] * SubScore(s, b, me);
    }
    // 大盘连线威胁：占两子且第三子可成 → 高威胁
    for (const auto& L : WIN_LINES) {
        int m = 0, o = 0, d = 0;
        for (int k = 0; k < 3; ++k) {
            const int v = s.bigboard[L[k]];
            if (v == me) ++m;
            else if (v == opp) ++o;
            else if (v == DRAW) ++d;
        }
        if (d == 0 && o == 0 && m > 0) score += (m == 2 ? 120 : 20);
        else if (d == 0 && m == 0 && o > 0) score -= (o == 2 ? 120 : 20);
    }
    return score;
}

// Negamax + Alpha-Beta，返回 me 视角的局面分
inline int Negamax(const State& s, int me, int depth, int alpha, int beta)
{
    if (depth == 0) {
        return Evaluate(s, me);
    }
    auto moves = LegalMoves(s);
    if (moves.empty()) {
        return Evaluate(s, me); // 理论不可达（未终局必有合法点）
    }
    OrderMoves(moves);
    int best = -2 * WIN_SCORE;
    for (const auto& [b, i] : moves) {
        State ns = s;
        const int r = ns.Apply(b, i, me);
        int val;
        if (r == me) val = WIN_SCORE - (64 - depth);     // 越快取胜分越高
        else if (r == DRAW) val = 0;
        else val = -Negamax(ns, 3 - me, depth - 1, -beta, -alpha);
        if (val > best) best = val;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;                          // 剪枝
    }
    return best;
}

// 为 me 选择最佳落子。board/bigboard/forced 为当前真实局面（来自 Board）。
inline std::pair<int, int> ChooseMove(const int board[9][9], const int bigboard[9], int forced, int me)
{
    State s;
    s.forced = forced;
    bool empty = true;
    for (int b = 0; b < 9; ++b) {
        s.bigboard[b] = bigboard[b];
        for (int j = 0; j < 9; ++j) {
            s.board[b][j] = board[b][j];
            if (board[b][j] != EMPTY) empty = false;
        }
    }
    // 标准强开局：空盘直接占据中心盘中心，省去 81 分支的深搜
    if (empty) {
        return {4, 4};
    }

    auto moves = LegalMoves(s);
    if (moves.empty()) {
        return {-1, -1}; // 理论不可达
    }
    OrderMoves(moves);

    // 自由落子（forced==-1）分支多，适当降低深度以控制耗时
    const int depth = (forced == -1) ? 4 : 6;
    std::pair<int, int> best_move = moves.front();
    int best = -2 * WIN_SCORE;
    int alpha = -2 * WIN_SCORE;
    const int beta = 2 * WIN_SCORE;
    for (const auto& [b, i] : moves) {
        State ns = s;
        const int r = ns.Apply(b, i, me);
        int val;
        if (r == me) val = WIN_SCORE;
        else if (r == DRAW) val = 0;
        else val = -Negamax(ns, 3 - me, depth - 1, -beta, -alpha);
        if (val > best) {
            best = val;
            best_move = {b, i};
        }
        if (best > alpha) alpha = best;
    }
    return best_move;
}

} // namespace ai
