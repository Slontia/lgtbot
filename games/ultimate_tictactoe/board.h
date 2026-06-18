#pragma once

// =============================================================================
//  终极井字（Ultimate Tic-Tac-Toe）棋盘
//  9 个 3×3 的小型九宫格构成一个大型九宫格，赢得小棋盘以争夺大棋盘的连线。
// =============================================================================

#include <array>
#include <string>

// —— 小棋盘 / 大棋盘通用的 8 条胜利连线（格子下标 0-8，按行优先排列）——
static constexpr int WIN_LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},  // 三横
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},  // 三竖
    {0, 4, 8}, {2, 4, 6},             // 两斜
};

// 格子取值；其中 DRAW 仅用于 bigboard，表示小棋盘已下满且无连线
enum Mark {
    EMPTY  = 0,  // 空格 / 小棋盘进行中
    MARK_O = 1,  // 先手（pid 0）的标志
    MARK_X = 2,  // 后手（pid 1）的标志
    DRAW   = 3,  // 仅 bigboard：小棋盘平局（已满且无连线）
};

class Board
{
  public:
    // 玩家昵称，按 pid 索引（pid 0 执 O，pid 1 执 X）
    std::string name[2];
    // 资源目录（末尾带 /），用于 @font-face 引用 resource 中的字体
    std::string resource_path_;
    // 每个小棋盘的归属：EMPTY=进行中 / MARK_O / MARK_X / DRAW
    int bigboard[9];
    // 全部 81 格：board[大格下标][小格下标]，取值 EMPTY / MARK_O / MARK_X
    int board[9][9];
    // 下一手被限定落子的小棋盘下标（0-8）；-1 表示可在任意小棋盘自由落子
    int forced;

    // 初始化空棋盘（首手可自由落子）
    void Initialize()
    {
        forced = -1;
        for (int i = 0; i < 9; ++i) {
            bigboard[i] = EMPTY;
            for (int j = 0; j < 9; ++j) {
                board[i][j] = EMPTY;
            }
        }
    }

    // 落子并推进胜负判定。
    // 返回：-1 = 游戏继续；MARK_O / MARK_X = 对应玩家赢得整局；DRAW = 大棋盘填满平局。
    // 对应 Java：board[bigIndex][index]=p+1; last=index; check(bigIndex); 再更新 last。
    int Place(int big, int idx, int mark)
    {
        board[big][idx] = mark;
        forced = idx;                 // 暂定下一手限定到第 idx 个小棋盘
        const int result = Check(big);
        if (result != -1) {
            forced = -1;              // 整局结束（对应 Java 分出胜负时 last = -1）
            return result;
        }
        // 若被指定的小棋盘已分出胜负或已下满（DRAW），则下一手可自由落子
        if (bigboard[idx] != EMPTY) {
            forced = -1;
        }
        return -1;
    }

    // 检测小棋盘 i 是否新成连线（更新 bigboard[i]），再检测大棋盘整体胜负。
    // 返回：-1 = 继续；MARK_O / MARK_X = 大棋盘连线获胜方；DRAW = 大棋盘填满平局。
    // 逐条对应 Java 的 check(i)。
    int Check(int i)
    {
        // 1) 小棋盘 i 是否连成三子
        for (const auto& line : WIN_LINES) {
            const int a = board[i][line[0]];
            if (a != EMPTY && a == board[i][line[1]] && a == board[i][line[2]]) {
                bigboard[i] = a;
                break;
            }
        }
        // 2) 未连线但已下满 → 小棋盘平局
        if (bigboard[i] == EMPTY) {
            bool full = true;
            for (int j = 0; j < 9; ++j) {
                if (board[i][j] == EMPTY) {
                    full = false;
                    break;
                }
            }
            if (full) {
                bigboard[i] = DRAW;
            }
        }
        // 3) 大棋盘是否连成三宫（平局格 DRAW 不参与连线）
        for (const auto& line : WIN_LINES) {
            const int a = bigboard[line[0]];
            if (a != EMPTY && a != DRAW && a == bigboard[line[1]] && a == bigboard[line[2]]) {
                return a;
            }
        }
        // 4) 大棋盘是否填满 → 平局
        for (int k = 0; k < 9; ++k) {
            if (bigboard[k] == EMPTY) {
                return -1;
            }
        }
        return DRAW;
    }

    // 将昵称处理为可安全嵌入 HTML 的文本：
    //   1) 去除部分机器人在昵称外附加的 < > 包裹
    //   2) 转义 HTML 特殊字符，避免昵称中的 < > & 等被渲染器当作标签而导致整个名字消失
    static std::string DisplayName_(const std::string& raw)
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

    // 生成棋盘的 HTML：
    //   - 9×9 网格分为 3×3 个小棋盘；空格显示坐标「大格小格」，O 粉色、X 蓝色
    //   - 已结束的小棋盘覆盖半透明黑色遮罩，并居中显示大号 O / X / 平
    //   - 当前可落子的小棋盘橙色高亮边框；forced==-1（自由落子）时整盘橙色外框
    // 参数 turn_pid 用于表头高亮当前行动方（0 执 O，1 执 X）。
    std::string GetUI(int turn_pid) const
    {
        // 字体仅作用于盘面内文字（坐标数字、O/X、遮罩大字），表头回合数/昵称沿用默认字体。
        std::string s = R"CSS(<style>
@font-face { font-family:'msyh'; src:url("file:///)CSS" + resource_path_ + R"CSS(msyh.ttf"); }
.ut-wrap { text-align:center; }
.ut-head { margin:0 auto 14px auto; border-collapse:separate; border-spacing:6px; }
.ut-badge { width:42px; height:42px; text-align:center; vertical-align:middle; color:#fff; font-family:'msyh'; font-weight:bold; font-size:28px; border-radius:6px; }
.ut-badge-o { background:#FF7AA0; }
.ut-badge-x { background:#2A6BFF; }
.ut-name { padding:2px 14px; font-size:21px; text-align:left; vertical-align:middle; }
.ut-cur { background:#FFE08A; border-radius:6px; }
.ut-board { margin:0 auto; border-collapse:separate; border-spacing:0; background:#fff; border:4px solid #000; }
.ut-board.free { border-color:#FF9A00; }
.ut-sub { padding:0; border:3px solid #000; }
.ut-sub.active { border-color:#FF9A00; }
.ut-subwrap { position:relative; }
.ut-inner { border-collapse:collapse; }
.ut-cell { width:44px; height:44px; text-align:center; vertical-align:middle; border:1px solid #d0d0d0; box-sizing:border-box; }
.ut-lbl { font-family:'msyh'; color:#444444; font-size:19px; font-weight:bold; }
.ut-o { font-family:'msyh'; color:#FF7AA0; font-weight:bold; font-size:26px; }
.ut-x { font-family:'msyh'; color:#2A6BFF; font-weight:bold; font-size:26px; }
.ut-ovl { position:absolute; top:0; left:0; right:0; bottom:0; display:flex; align-items:center; justify-content:center; background:rgba(0,0,0,0.5); font-family:'msyh'; font-weight:bold; font-size:58px; }
.ut-ovl-o { color:#FF7AA0; }
.ut-ovl-x { color:#2A6BFF; }
.ut-ovl-d { color:#fff; }
</style>)CSS";

        s += "<div class=\"ut-wrap\">";

        // —— 玩家信息表头：O / X 居中显示于各自的颜色方格，上下排列；当前行动方仅用黄色高亮 ——
        s += "<table class=\"ut-head\">";
        s += "<tr><td class=\"ut-badge ut-badge-o\">O</td>";
        s += "<td class=\"ut-name" + std::string(turn_pid == 0 ? " ut-cur" : "") + "\">" + DisplayName_(name[0]) + "</td></tr>";
        s += "<tr><td class=\"ut-badge ut-badge-x\">X</td>";
        s += "<td class=\"ut-name" + std::string(turn_pid == 1 ? " ut-cur" : "") + "\">" + DisplayName_(name[1]) + "</td></tr>";
        s += "</table>";

        // —— 棋盘：3×3 个小棋盘 ——
        s += "<table class=\"ut-board" + std::string(forced == -1 ? " free" : "") + "\">";
        for (int by = 0; by < 3; ++by) {
            s += "<tr>";
            for (int bx = 0; bx < 3; ++bx) {
                const int b = by * 3 + bx;
                s += "<td class=\"ut-sub" + std::string(forced == b ? " active" : "") + "\"><div class=\"ut-subwrap\">";
                s += "<table class=\"ut-inner\">";
                for (int cy = 0; cy < 3; ++cy) {
                    s += "<tr>";
                    for (int cx = 0; cx < 3; ++cx) {
                        const int j = cy * 3 + cx;
                        s += "<td class=\"ut-cell\">";
                        if (board[b][j] == MARK_O) {
                            s += "<span class=\"ut-o\">O</span>";
                        } else if (board[b][j] == MARK_X) {
                            s += "<span class=\"ut-x\">X</span>";
                        } else {
                            s += "<span class=\"ut-lbl\">" + std::to_string(b + 1) + std::to_string(j + 1) + "</span>";
                        }
                        s += "</td>";
                    }
                    s += "</tr>";
                }
                s += "</table>";
                // 已结束的小棋盘：半透明遮罩 + 大号符号
                if (bigboard[b] != EMPTY) {
                    const char* cls = bigboard[b] == MARK_O ? "ut-ovl-o" : (bigboard[b] == MARK_X ? "ut-ovl-x" : "ut-ovl-d");
                    const char* sym = bigboard[b] == MARK_O ? "O" : (bigboard[b] == MARK_X ? "X" : "平");
                    s += "<div class=\"ut-ovl " + std::string(cls) + "\">" + sym + "</div>";
                }
                s += "</div></td>";
            }
            s += "</tr>";
        }
        s += "</table>";

        s += "</div>";
        return s;
    }
};
