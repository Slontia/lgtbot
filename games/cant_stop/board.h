#pragma once

// =============================================================================
//  欲罢不能（Can't Stop）棋盘与数据结构
//  原型：Sid Sackson 设计的桌游《Can't Stop》（1980）。
//  11 列编号 2–12，列高 3,5,7,9,11,13,11,9,7,5,3（中间高两边低，呈菱形）。
// =============================================================================

#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <utility>

// —— 列编号范围 [2, 12] ——
static constexpr int COL_MIN = 2;
static constexpr int COL_MAX = 12;
// 各列高度（从底部到顶部所需推进的格数）；下标即列号，0/1 不使用
static constexpr int COL_HEIGHT[COL_MAX + 1] = {
    0, 0, 3, 5, 7, 9, 11, 13, 11, 9, 7, 5, 3,
};
// 白色中立棋子（跑子 / runner）数量：每回合最多同时占据 3 列
static constexpr int RUNNER_COUNT = 3;
// 一回合掷 4 颗骰子
static constexpr int DICE_COUNT = 4;

// —— 列未被任何玩家夺取（开放）的标记 ——
static constexpr int UNCLAIMED = -1;

// 规则变体（互斥，同时只能启用其一）；取值与 options.h 中「变体」选项一致。
//   NONE   = 默认规则
//   JUMP   = 跳跃：跑子推进时跳过其他玩家棋子占据的格子，落到上方第一个空格
//   FORCED = 强制运动：跑子与其他玩家棋子同格时本回合不能停止
enum class Variant { NONE = 0, JUMP = 1, FORCED = 2 };

// 一个可供玩家选择的走法：源自某一种骰子配对（两两相加得到两个点数）。
// advances 为本走法实际推进的列号列表：
//   - 推进两列：{a, b}
//   - 仅能推进一列：{a} 或 {b}
//   - 同一列推进两步（如配对得到两个相同的和）：{a, a}
struct MoveOption {
    int pair_a = 0;             // 配对的第一个和（展示用，保证 pair_a <= pair_b）
    int pair_b = 0;             // 配对的第二个和（展示用）
    int da1 = 0, da2 = 0;       // 形成 pair_a 的两颗骰子点数（da1 + da2 == pair_a）
    int db1 = 0, db2 = 0;       // 形成 pair_b 的两颗骰子点数（db1 + db2 == pair_b）
    std::vector<int> advances;  // 实际推进的列（同列两步则该列出现两次）
};

class Board
{
  public:
    // ===== 全局持久状态 =====
    int num_players_ = 0;                          // 玩家总数
    int target_columns_ = 3;                       // 夺得多少列即获胜
    Variant variant_ = Variant::NONE;              // 规则变体（默认 / 跳跃 / 强制运动）
    std::string resource_path_;                    // 资源目录（末尾带 /），用于引用骰子图片
    std::vector<std::string> names_;               // 玩家昵称（已做 HTML 转义前的原始串）
    std::vector<std::string> avatars_;             // 玩家头像 HTML 片段
    // 永久进度：progress_[pid][col]，取值 0..COL_HEIGHT[col]
    //   0 = 尚未上路（位于底部之下）；COL_HEIGHT[col] = 已到顶
    std::vector<std::array<int, COL_MAX + 1>> progress_;
    // 已被夺取并永久封闭的列：claimed_[col] = 夺取者 pid；UNCLAIMED 表示开放
    std::array<int, COL_MAX + 1> claimed_;

    // ===== 当前回合临时状态（仅属于正在行动的 turn_pid_）=====
    int turn_pid_ = 0;                             // 当前行动玩家
    // 本回合白子（跑子）的临时位置：runner_[col]，0 表示该列本回合无白子
    std::array<int, COL_MAX + 1> runner_;
    bool turn_has_progress_ = false;               // 本回合是否已成功推进过白子
    int turn_steps_ = 0;                           // 本回合已推进的总步数（供电脑停手判断）
    std::array<int, DICE_COUNT> dice_{};           // 当前 4 颗骰子点数（由调用方填充）
    std::vector<MoveOption> options_;              // 当前可选走法（为空即「爆掉 / bust」）

    // 初始化整局：清空所有进度与夺列状态
    void Initialize(const int num_players, std::vector<std::string> names,
                    std::vector<std::string> avatars, const int target_columns,
                    std::string resource_path = "", const Variant variant = Variant::NONE)
    {
        num_players_ = num_players;
        target_columns_ = target_columns;
        variant_ = variant;
        resource_path_ = std::move(resource_path);
        names_ = std::move(names);
        avatars_ = std::move(avatars);
        progress_.assign(num_players_, std::array<int, COL_MAX + 1>{});
        claimed_.fill(UNCLAIMED);
        turn_pid_ = 0;
        ResetTurn();
    }

    // 重置当前回合临时状态（回合开始时调用：收回全部白子）
    void ResetTurn()
    {
        runner_.fill(0);
        turn_has_progress_ = false;
        turn_steps_ = 0;
        dice_.fill(0);
        options_.clear();
    }

    // 本回合已占用的白子数（runner_ 中位置非 0 的列数）
    int RunnersUsed() const
    {
        int used = 0;
        for (int col = COL_MIN; col <= COL_MAX; ++col) {
            if (runner_[col] != 0) {
                ++used;
            }
        }
        return used;
    }

    // 某玩家已夺取的列数
    int ClaimedCountOf(const int pid) const
    {
        int count = 0;
        for (int col = COL_MIN; col <= COL_MAX; ++col) {
            if (claimed_[col] == pid) {
                ++count;
            }
        }
        return count;
    }

    // 终局进度：仅统计仍留在盘面上的棋子（已被夺取的列只显示★、棋子已移除，不计入）。
    int OnBoardProgressOf(const int pid) const
    {
        int total = 0;
        for (int col = COL_MIN; col <= COL_MAX; ++col) {
            if (claimed_[col] == UNCLAIMED) {
                total += progress_[pid][col];
            }
        }
        return total;
    }

    // 是否所有列都已被夺取（棋盘已满，任何玩家都无法再行动）
    bool AllColumnsClaimed() const
    {
        for (int col = COL_MIN; col <= COL_MAX; ++col) {
            if (claimed_[col] == UNCLAIMED) {
                return false;
            }
        }
        return true;
    }

    // 当前是否爆掉（本次掷骰无任何合法走法）
    bool IsBust() const { return options_.empty(); }

    // 当前 4 颗骰子是否全部同点（用于「一掷乾坤」成就判定）
    bool AllDiceEqual() const
    {
        for (int i = 1; i < DICE_COUNT; ++i) {
            if (dice_[i] != dice_[0]) {
                return false;
            }
        }
        return true;
    }

    // 强制运动变体：当前回合是否有任一跑子与其他玩家的棋子同处一格（同格则本回合不能主动停止）。
    // 仅在 FORCED 变体下生效，其它变体始终返回 false。
    bool MustContinue() const
    {
        if (variant_ != Variant::FORCED) {
            return false;
        }
        for (int c = COL_MIN; c <= COL_MAX; ++c) {
            if (runner_[c] != 0 && OpponentOnSpace_(c, runner_[c])) {
                return true;
            }
        }
        return false;
    }

    // 根据当前骰子 dice_、白子占用与夺列状态，枚举全部合法走法到 options_。
    // 4 颗骰子恰有 3 种两两配对；每种配对按「尽量多推进」的规则求结果，
    // 仅在「两列皆为新列却只剩 1 个白子」时拆分为两个单列选项。最终去重。
    void ComputeOptions()
    {
        options_.clear();
        // 4 骰下标的 3 种两两配对方式
        static const int PAIRS[3][4] = {
            {0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2},
        };
        for (const auto& p : PAIRS) {
            int a1 = dice_[p[0]], a2 = dice_[p[1]];
            int b1 = dice_[p[2]], b2 = dice_[p[3]];
            int s = a1 + a2, t = b1 + b2;
            if (s > t) {
                std::swap(s, t); // 规范展示顺序：pair_a <= pair_b
                std::swap(a1, b1);
                std::swap(a2, b2);
            }
            AddOutcomesForPairing_(s, t, a1, a2, b1, b2);
        }
        DedupOptions_();
    }

    // 应用某个走法：按 advances 顺序推进白子（推进失败的步会被忽略）。
    void ApplyOption(const MoveOption& option)
    {
        int free_runners = RUNNER_COUNT - RunnersUsed();
        for (const int c : option.advances) {
            if (TryAdvance_(runner_, free_runners, c)) {
                ++turn_steps_;
            }
        }
        turn_has_progress_ = true;
    }

    // 停止时提交本回合进度：白子位置写入永久进度；登顶且开放的列被夺取并封闭，
    // 同时收回其他玩家在该列的进度。返回本回合新夺得的列号列表。
    std::vector<int> Commit(const int pid)
    {
        std::vector<int> newly_claimed;
        for (int c = COL_MIN; c <= COL_MAX; ++c) {
            if (runner_[c] == 0) {
                continue;
            }
            progress_[pid][c] = runner_[c];
            if (runner_[c] == COL_HEIGHT[c] && claimed_[c] == UNCLAIMED) {
                claimed_[c] = pid;
                newly_claimed.push_back(c);
                for (int p = 0; p < num_players_; ++p) {
                    if (p != pid) {
                        progress_[p][c] = 0; // 收回其他玩家在该列的棋子
                    }
                }
            }
        }
        return newly_claimed;
    }

    // 将昵称处理为可安全嵌入 HTML 的文本（去除外层 <>，转义特殊字符）。
    static std::string DisplayName(const std::string& raw)
    {
        std::string s = raw;
        if (s.size() >= 2 && s.front() == '<' && s.back() == '>') {
            s = s.substr(1, s.size() - 2);
        }
        std::string out;
        out.reserve(s.size());
        for (const char c : s) {
            switch (c) {
                case '&':  out += "&amp;";  break;
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&#39;";  break;
                default:   out += c;        break;
            }
        }
        return out;
    }

    // 生成棋盘图片 HTML：顶部玩家列表（高亮当前行动玩家）+ 菱形棋盘（11 列垂直居中）。
    std::string GetUI() const
    {
        std::string s = Css_();
        s += "<div class=\"cs-wrap\">";
        s += BuildHeader_();
        s += "<table class=\"cs-board\"><tr>";
        for (int c = COL_MIN; c <= COL_MAX; ++c) {
            s += BuildColumn_(c);
        }
        s += "</tr></table>";
        s += "<div class=\"cs-legend\">○ ＝本回合跑子（临时进度，停止时保存）　★ ＝已被夺得的列</div>";
        s += "</div>";
        return s;
    }

    // 生成「骰子 + 可选走法」图片 HTML：上方展示本次掷出的 4 颗骰子；
    // 下方表格逐行展示——左侧为该走法对应的骰子配对（AB + CD），右侧说明推进的列。
    std::string GetDiceOptionsUI() const
    {
        std::string s = DiceCss_();
        s += "<div class=\"cs-roll\">";
        s += "<div class=\"cs-dice-top\">";
        for (int i = 0; i < DICE_COUNT; ++i) {
            s += DieImg_(dice_[i], "cs-die");
        }
        s += "</div>";
        if (options_.empty()) {
            s += "<div class=\"cs-noopt\">无可推进的走法，本回合爆掉</div>";
        } else {
            s += "<table class=\"cs-opt\">";
            for (int i = 0; i < static_cast<int>(options_.size()); ++i) {
                const MoveOption& o = options_[i];
                s += "<tr>";
                s += "<td class=\"cs-optnum\">" + std::to_string(i + 1) + "</td>";
                s += "<td class=\"cs-optdice\">" + DieImg_(o.da1, "cs-die-s") + DieImg_(o.da2, "cs-die-s")
                   + "<span class=\"cs-plus\">+</span>" + DieImg_(o.db1, "cs-die-s") + DieImg_(o.db2, "cs-die-s") + "</td>";
                s += "<td class=\"cs-optdesc\">" + OptionDescHtml_(o) + "</td>";
                s += "</tr>";
            }
            s += "</table>";
        }
        s += "</div>";
        return s;
    }

    // 生成「4 颗骰子 + 一行红色提示」图片 HTML，不含走法表格。
    std::string GetDiceMessageUI(const std::string& red_text) const
    {
        std::string s = DiceCss_();
        s += "<div class=\"cs-roll\">";
        s += "<div class=\"cs-dice-top\">";
        for (int i = 0; i < DICE_COUNT; ++i) {
            s += DieImg_(dice_[i], "cs-die");
        }
        s += "</div>";
        s += "<div class=\"cs-noopt\">" + red_text + "</div>";
        s += "</div>";
        return s;
    }

  private:
    // 列 c 的位置 pos（1..高度）是否有其他玩家（非当前行动玩家）的已落定棋子
    bool OpponentOnSpace_(const int c, const int pos) const
    {
        for (int p = 0; p < num_players_; ++p) {
            if (p != turn_pid_ && progress_[p][c] == pos) {
                return true;
            }
        }
        return false;
    }

    // 在给定状态（可为临时副本）上尝试推进列 c。free_runners 按引用传入，占用新跑子时自减；成功推进返回 true。
    //   - 列已封闭/已到顶 → 失败；已有跑子 → 前进 1 格；无跑子 → 占用 1 个空闲跑子，置于「己方落定位置 + 1」。
    //   - 跳跃变体：若目标格被其他玩家棋子占据，则跳过连续被占据的格子，落到上方第一个空格（顶端始终为空，不会越过顶端）。
    bool TryAdvance_(std::array<int, COL_MAX + 1>& runner, int& free_runners, const int c) const
    {
        if (c < COL_MIN || c > COL_MAX) {
            return false;
        }
        if (claimed_[c] != UNCLAIMED) {
            return false; // 已封闭
        }
        const std::array<int, COL_MAX + 1>& prog = progress_[turn_pid_];
        const bool is_new = (runner[c] == 0);
        int target;
        if (!is_new) {
            if (runner[c] >= COL_HEIGHT[c]) {
                return false; // 已到顶
            }
            target = runner[c] + 1;
        } else {
            if (free_runners <= 0) {
                return false; // 无空闲白子
            }
            target = prog[c] + 1; // 底格（progress 0 → 1）或己方棋子上一格
        }
        if (variant_ == Variant::JUMP) {
            while (target < COL_HEIGHT[c] && OpponentOnSpace_(c, target)) {
                ++target; // 跳过其他玩家棋子占据的格子
            }
        }
        runner[c] = target;
        if (is_new) {
            --free_runners;
        }
        return true;
    }

    // 列 c 能否「单独」被推进（用于判断单列选项）
    bool CanAdvanceAlone_(const int c) const
    {
        std::array<int, COL_MAX + 1> tmp = runner_;
        int free_runners = RUNNER_COUNT - RunnersUsed();
        return TryAdvance_(tmp, free_runners, c);
    }

    // 计算配对 (s, t) 的合法走法并加入 options_。(da1,da2) 形成 s，(db1,db2) 形成 t，用于骰子图片展示。
    void AddOutcomesForPairing_(const int s, const int t, const int da1, const int da2,
                                const int db1, const int db2)
    {
        const int free_runners = RUNNER_COUNT - RunnersUsed();
        if (s == t) {
            // 同一列：尝试在该列连进至多 2 格（doubles 等情形）
            std::array<int, COL_MAX + 1> tmp = runner_;
            int f = free_runners;
            std::vector<int> adv;
            if (TryAdvance_(tmp, f, s)) {
                adv.push_back(s);
                if (TryAdvance_(tmp, f, s)) {
                    adv.push_back(s);
                }
            }
            if (!adv.empty()) {
                options_.push_back(MoveOption{s, t, da1, da2, db1, db2, std::move(adv)});
            }
            return;
        }
        // s != t：分别按两种顺序尝试，取「能推进列数」最多者
        auto trial = [&](const int first, const int second) {
            std::array<int, COL_MAX + 1> tmp = runner_;
            int f = free_runners;
            std::vector<int> adv;
            if (TryAdvance_(tmp, f, first))  { adv.push_back(first); }
            if (TryAdvance_(tmp, f, second)) { adv.push_back(second); }
            return adv;
        };
        std::vector<int> adv_st = trial(s, t);
        std::vector<int> adv_ts = trial(t, s);
        const int best = std::max(static_cast<int>(adv_st.size()), static_cast<int>(adv_ts.size()));
        if (best == 0) {
            return; // 该配对无法推进
        }
        if (best == 2) {
            // 可同时推进两列
            options_.push_back(MoveOption{s, t, da1, da2, db1, db2, adv_st.size() == 2 ? std::move(adv_st) : std::move(adv_ts)});
            return;
        }
        // best == 1：仅能推进一列
        const bool can_s = CanAdvanceAlone_(s);
        const bool can_t = CanAdvanceAlone_(t);
        if (can_s && can_t) {
            // 两列皆为新列却只剩 1 个白子 → 二选一
            options_.push_back(MoveOption{s, t, da1, da2, db1, db2, std::vector<int>{s}});
            options_.push_back(MoveOption{s, t, da1, da2, db1, db2, std::vector<int>{t}});
        } else if (can_s) {
            options_.push_back(MoveOption{s, t, da1, da2, db1, db2, std::vector<int>{s}});
        } else if (can_t) {
            options_.push_back(MoveOption{s, t, da1, da2, db1, db2, std::vector<int>{t}});
        }
    }

    // 去重：推进列的多重集合相同的走法只保留一个（避免出现效果完全相同的重复选项）
    void DedupOptions_()
    {
        std::vector<MoveOption> uniq;
        for (auto& o : options_) {
            std::vector<int> key = o.advances;
            std::sort(key.begin(), key.end());
            bool dup = false;
            for (const auto& u : uniq) {
                std::vector<int> k2 = u.advances;
                std::sort(k2.begin(), k2.end());
                if (k2 == key) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                uniq.push_back(std::move(o));
            }
        }
        options_ = std::move(uniq);
    }

    // ===== UI 绘制（纯 HTML/CSS，无图片素材）=====

    // 玩家颜色（最多 4 人），与棋盘上的彩色棋子一致
    static const char* PlayerColor_(const int pid)
    {
        static const char* const COLORS[4] = {"#FF7AA0", "#2A6BFF", "#8E44AD", "#F2A900"};
        return COLORS[pid % 4];
    }

    static std::string Css_()
    {
        return R"CSS(<style>
.cs-wrap { text-align:center; padding:8px; }
.cs-head { margin:0 auto 14px auto; border-collapse:separate; border-spacing:5px; }
.cs-prow td { padding:3px 6px; vertical-align:middle; }
.cs-cur td.cs-pname { background:#FFE7A3; border-radius:6px; }
.cs-avatar { width:40px; }
.cs-swatch { display:inline-block; width:20px; height:20px; border-radius:5px; border:1px solid rgba(0,0,0,0.15); }
.cs-pname { text-align:left; }
.cs-pname-box { width:280px; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; font-size:19px; color:#222222; }
.cs-pclaim { font-size:16px; color:#555555; white-space:nowrap; }
.cs-pclaim b { color:#2E7D32; font-size:19px; }
.cs-board { margin:0 auto; border-collapse:separate; border-spacing:0; background:#F4F6F9; border-radius:18px; padding:18px; }
.cs-colcell { vertical-align:middle; padding:0 5px; }
.cs-cell { width:54px; height:39px; box-sizing:border-box; border:1px solid #cdd2da; background:#ffffff; display:flex; align-items:center; justify-content:center; flex-wrap:wrap; }
.cs-cell-top { border:3px solid #D4AF37; background:#FFF7DD; }
.cs-col-claimed .cs-cell { background:#e9e9ec; border-color:#d6d6da; }
.cs-claimmark { width:100%; height:100%; display:flex; align-items:center; justify-content:center; color:#ffffff; font-weight:bold; font-size:26px; }
.cs-cube { min-width:21px; height:23px; line-height:23px; margin:2px; border-radius:5px; color:#ffffff; font-size:15px; font-weight:bold; text-align:center; padding:0 2px; box-sizing:border-box; }
.cs-runner { width:23px; height:23px; margin:2px; border-radius:50%; background:#ffffff; border:3px solid #333333; box-sizing:border-box; }
.cs-cell.cs-crowded .cs-cube { min-width:16px; height:16px; line-height:16px; margin:1px 3px; font-size:10px; border-radius:4px; padding:0 1px; }
.cs-cell.cs-crowded .cs-runner { width:16px; height:16px; margin:1px 3px; border-width:2px; }
.cs-collabel { margin-top:8px; width:54px; height:36px; line-height:36px; border-radius:8px; background:#39404e; color:#ffffff; font-weight:bold; font-size:26px; text-align:center; }
.cs-legend { margin:18px auto 0 auto; font-size:18px; color:#777777; }
</style>)CSS";
    }

    // 顶部玩家列表：头像 + 颜色色块 + 昵称（当前行动者标注） + 已夺列数
    std::string BuildHeader_() const
    {
        std::string s = "<table class=\"cs-head\">";
        for (int p = 0; p < num_players_; ++p) {
            s += std::string("<tr class=\"cs-prow") + (p == turn_pid_ ? " cs-cur" : "") + "\">";
            s += "<td class=\"cs-avatar\">" + avatars_[p] + "</td>";
            s += "<td class=\"cs-pname\"><div class=\"cs-pname-box\">" + DisplayName(names_[p]) + "</div></td>";
            s += "<td><span class=\"cs-swatch\" style=\"background:" + std::string(PlayerColor_(p)) + ";\"></span></td>";
            s += "<td class=\"cs-pclaim\">已夺 <b>" + std::to_string(ClaimedCountOf(p)) + "</b>/" + std::to_string(target_columns_) + " 列</td>";
            s += "</tr>";
        }
        s += "</table>";
        return s;
    }

    // 单列（一个表格单元 td，垂直居中）：从顶端到底端逐格 + 底部列号标签
    std::string BuildColumn_(const int c) const
    {
        const int H = COL_HEIGHT[c];
        const bool claimed = (claimed_[c] != UNCLAIMED);
        std::string s = "<td class=\"cs-colcell";
        if (claimed) {
            s += " cs-col-claimed";
        }
        s += "\">";
        for (int pos = H; pos >= 1; --pos) {
            s += BuildCell_(c, pos, H, claimed);
        }
        s += "<div class=\"cs-collabel\"";
        if (claimed) {
            s += " style=\"background:" + std::string(PlayerColor_(claimed_[c])) + ";\"";
        }
        s += ">" + std::to_string(c) + "</div>";
        s += "</td>";
        return s;
    }

    // 单格：已夺列的顶端显示夺取者标记并整列变灰；否则显示该位置的玩家棋子与本回合白色跑子
    std::string BuildCell_(const int c, const int pos, const int H, const bool claimed) const
    {
        const bool is_top = (pos == H);
        std::string s = "<div class=\"cs-cell";
        if (is_top) {
            s += " cs-cell-top";
        }
        if (claimed) {
            s += "\">";
            if (is_top) {
                s += "<div class=\"cs-claimmark\" style=\"background:" + std::string(PlayerColor_(claimed_[c])) + ";\">★</div>";
            }
            s += "</div>";
            return s;
        }
        // 统计该格标记数（在位玩家棋子 + 本回合白子）；≥3 个时标记为「拥挤格」，
        // 由 CSS 将标记缩小、经 flex-wrap 排成 2×2，恰好容纳最多 4 个且不溢出格高。
        int marker_count = (runner_[c] == pos) ? 1 : 0;
        for (int p = 0; p < num_players_; ++p) {
            if (progress_[p][c] == pos) {
                ++marker_count;
            }
        }
        if (marker_count >= 3) {
            s += " cs-crowded";
        }
        s += "\">";
        for (int p = 0; p < num_players_; ++p) {
            if (progress_[p][c] == pos) {
                // 棋子编号从 0 开始，与框架 @玩家 的编号一致
                s += "<span class=\"cs-cube\" style=\"background:" + std::string(PlayerColor_(p)) + ";\">" + std::to_string(p) + "</span>";
            }
        }
        if (runner_[c] == pos) {
            s += "<span class=\"cs-runner\"></span>";
        }
        s += "</div>";
        return s;
    }

    // 单颗骰子图片（引用 resource 目录下的 dice_<点数>.png）
    std::string DieImg_(const int value, const char* const cls) const
    {
        return "<img class=\"" + std::string(cls) + "\" src=\"file:///" + resource_path_ + "dice_" + std::to_string(value) + ".png\"/>";
    }

    // 走法推进信息文本
    std::string OptionDescHtml_(const MoveOption& o) const
    {
        if (o.advances.size() == 2 && o.advances[0] == o.advances[1]) {
            return "第 " + std::to_string(o.advances[0]) + " 列连进 2 格";
        }
        if (o.advances.size() == 2) {
            return "推进 第 " + std::to_string(o.advances[0]) + " 列、第 " + std::to_string(o.advances[1]) + " 列";
        }
        return "推进 第 " + std::to_string(o.advances[0]) + " 列";
    }

    static std::string DiceCss_()
    {
        return R"CSS(<style>
.cs-roll { text-align:center; padding:8px 10px; }
.cs-dice-top { margin-bottom:12px; }
.cs-die { width:46px; height:46px; margin:0 4px; vertical-align:middle; }
.cs-opt { margin:0 auto; border-collapse:collapse; }
.cs-opt td { padding:6px 8px; border-bottom:1px solid #ECEFF3; vertical-align:middle; }
.cs-optnum { width:30px; text-align:center; font-weight:bold; font-size:18px; color:#333333; background:#EEF1F6; border-radius:6px; }
.cs-die-s { width:26px; height:26px; margin:0 1px; vertical-align:middle; }
.cs-plus { font-size:16px; color:#999999; margin:0 4px; }
.cs-optdesc { text-align:left; font-size:17px; color:#222222; padding-left:10px; }
.cs-noopt { color:#E53935; font-weight:bold; font-size:18px; }
</style>)CSS";
    }
};
