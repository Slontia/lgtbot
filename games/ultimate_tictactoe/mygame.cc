// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <array>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#include "board.h"  // Board / Mark / WIN_LINES 置于本游戏命名空间内
#include "ai.h"     // 电脑 AI（Negamax + Alpha-Beta），依赖 board.h 的 Mark / WIN_LINES

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "终极井字", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "在小九宫格中落子，赢得小棋盘以争夺大九宫格连线的游戏",
    .shuffled_player_id_ = true, // 座位随机打乱，先手固定为 pid 0
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

// 单一 AtomicStage：turn_pid_ 交替行动；pid 0 执 O（先手），pid 1 执 X。
class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前棋盘，可用于图片重发", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "认输并结束本局", &MainStage::Concede_,
                    AlterChecker<uint32_t>({{"认输", 0}, {"投降", 0}})),
                MakeStageCommand(*this, "落子（坐标为两位数字：大格号 + 小格号，如 53）", &MainStage::Set_,
                    AnyArg("坐标", "53")))
        , round_(1)
        , player_scores_(Global().PlayerNum(), 0)
        , turn_pid_(0)
    {
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    virtual void OnStageBegin() override
    {
        board_.Initialize();
        board_.name[0] = Global().PlayerName(0);
        board_.name[1] = Global().PlayerName(1);
        board_.resource_path_ = Global().ResourceDir();
        Global().Boardcast() << Markdown(BoardMarkdown_(turn_pid_));
        PromptTurn_();
        Global().StartTimer(GAME_OPTION(时限));
        Global().SetReady(1 - turn_pid_); // 仅当前玩家（先手 pid 0）需要行动
    }

    // 超时：当前行动玩家判负（与「认输」等效的合法结果）
    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << "玩家" << At(turn_pid_) << " 超时判负。";
        player_scores_[turn_pid_] = -1;
        player_scores_[1 - turn_pid_] = 1;
        return StageErrCode::CHECKOUT;
    }

    // 退出：退出玩家判负，对手获胜
    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << "玩家" << At(pid) << " 强退认负。";
        player_scores_[pid] = -1;
        player_scores_[1 - pid] = 1;
        return StageErrCode::CHECKOUT;
    }

    // 电脑行动：用带 Alpha-Beta 剪枝的 Negamax 搜索选出最佳落子
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK; // 非当前回合或已行动
        }
        const auto [big, idx] = ai::ChooseMove(board_.board, board_.bigboard, board_.forced, Pid2Mark_(pid));
        if (big < 0) {
            return StageErrCode::CHECKOUT; // 理论不可达：游戏未结束时必有合法点
        }
        const int result = board_.Place(big, idx, Pid2Mark_(pid));
        return AfterMove_(pid, result);
    }

    // 不会到达：每一手都通过 AfterMove_ 直接返回 CONTINUE / CHECKOUT，
    // 且 CONTINUE 时仅当前玩家处于未就绪状态，故 OnStageOver 永不触发。
    virtual CheckoutErrCode OnStageOver() override { return StageErrCode::CHECKOUT; }

  private:
    // 赛况：重发当前棋盘图片（reply 回显至指令来源，同 hex / normal_renju）
    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(BoardMarkdown_(turn_pid_));
        return StageErrCode::OK;
    }

    // 认输 / 投降：该玩家判负，对手获胜，本局立即结束
    AtomReqErrCode Concede_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t)
    {
        player_scores_[pid] = -1;
        player_scores_[1 - pid] = 1;
        Global().Boardcast() << "玩家" << At(pid) << " 认输，对手获得胜利！";
        return StageErrCode::CHECKOUT;
    }

    // 落子：校验回合，解析坐标后交由 HandleMove_ 做合法性校验与落子
    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& coor_str)
    {
        if (pid != turn_pid_) {
            reply() << "[错误] 落子失败：现在不是您的回合";
            return StageErrCode::FAILED;
        }
        int big = -1, idx = -1;
        if (!ParseCoor_(coor_str, reply, big, idx)) {
            return StageErrCode::FAILED;
        }
        return HandleMove_(pid, big, idx, reply);
    }

    // 合法性校验（逐条对应 Java input 中的三项校验）→ 落子 → 推进回合
    AtomReqErrCode HandleMove_(const PlayerID pid, const int big, const int idx, MsgSenderBase& reply)
    {
        // 1) 强制宫格限制（Java: last != -1 && last != bigIndex）
        if (board_.forced != -1 && board_.forced != big) {
            reply() << "[错误] 落子失败：本手被限制在第 " << (board_.forced + 1) << " 宫格内落子";
            return StageErrCode::FAILED;
        }
        // 2) 该小棋盘已分出胜负或已下满（Java: bigboard[bigIndex] != 0）
        if (board_.bigboard[big] != EMPTY) {
            reply() << "[错误] 落子失败：第 " << (big + 1) << " 宫格已分出胜负，请选择其它宫格";
            return StageErrCode::FAILED;
        }
        // 3) 该格已有棋子（Java: board[bigIndex][index] != 0）
        if (board_.board[big][idx] != EMPTY) {
            reply() << "[错误] 落子失败：第 " << (big + 1) << " 宫格第 " << (idx + 1) << " 格已经有棋子了";
            return StageErrCode::FAILED;
        }
        const int result = board_.Place(big, idx, Pid2Mark_(pid));
        return AfterMove_(pid, result);
    }

    // 一手落子后的统一结算：展示棋盘，判定胜/平或交换行动方继续
    CheckoutErrCode AfterMove_(const PlayerID pid, const int result)
    {
        if (result == MARK_O || result == MARK_X) {
            player_scores_[pid] = 1;
            player_scores_[1 - pid] = 0; // 正常对局：胜 1，负 0
            Global().Boardcast() << Markdown(BoardMarkdown_(pid)); // 最终棋盘，高亮获胜方
            Global().Boardcast() << "玩家" << MarkName_(pid) << "方" << At(pid) << " 在大棋盘连成三宫，获得胜利！";
            return StageErrCode::CHECKOUT;
        }
        if (result == DRAW) {
            Global().Boardcast() << Markdown(BoardMarkdown_(pid));
            Global().Boardcast() << "大棋盘已填满且无人连成三宫，游戏平局！";
            return StageErrCode::CHECKOUT;
        }
        // 继续：交换行动方，仅新的当前玩家处于未就绪状态
        turn_pid_ = 1 - turn_pid_;
        ++round_;
        Global().ClearReady();
        Global().SetReady(1 - turn_pid_);
        Global().Boardcast() << Markdown(BoardMarkdown_(turn_pid_));
        PromptTurn_();
        Global().StartTimer(GAME_OPTION(时限));
        return StageErrCode::CONTINUE;
    }

    // 解析两位坐标「大格号 + 小格号」（各为 1-9）→ big / idx（0-8）。
    // 失败时回复错误并返回 false。对应 Java：text.length()!=2 与 charAt(0/1)-'1'。
    bool ParseCoor_(const std::string& s, MsgSenderBase& reply, int& big, int& idx)
    {
        if (s.size() != 2 || s[0] < '1' || s[0] > '9' || s[1] < '1' || s[1] > '9') {
            reply() << "[错误] 落子失败：" << s << " 不是有效的坐标，应为两位 1-9 的数字（大格号 + 小格号，如 53）";
            return false;
        }
        big = s[0] - '1';
        idx = s[1] - '1';
        return true;
    }

    // 向群内提示当前行动方及落子限制
    void PromptTurn_()
    {
        auto sender = Global().Boardcast();
        sender << "请 " << MarkName_(turn_pid_) << "方" << At(turn_pid_) << " 落子";
        if (board_.forced == -1) {
            sender << "，可在任意未结束的宫格自由落子";
        } else {
            sender << "，限定在第 " << (board_.forced + 1) << " 宫格内落子";
        }
        sender << "（时限 " << GAME_OPTION(时限) << " 秒，超时判负）";
    }

    // 棋盘 markdown：回合标题 + 棋盘 HTML
    std::string BoardMarkdown_(const PlayerID highlight) const
    {
        return "## 第 " + std::to_string(round_) + " 回合\n\n" + board_.GetUI(highlight);
    }

    // pid → 标志值：pid 0 执 O，pid 1 执 X
    static int Pid2Mark_(const PlayerID pid) { return pid == 0 ? MARK_O : MARK_X; }
    // pid → 标志名
    static const char* MarkName_(const PlayerID pid) { return pid == 0 ? "O" : "X"; }

    int round_;
    std::vector<int64_t> player_scores_; // 正常胜 = 1，正常负/平局 = 0；认输·超时·退出方 = -1（对手 1）
    Board board_;
    PlayerID turn_pid_; // 当前行动方；先手固定 pid 0（座位由框架打乱）
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
