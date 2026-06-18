// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <array>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <map>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/random.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

#include "board.h"

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "欲罢不能", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "掷骰推进各列，及时锁定进度，率先登顶三列即获胜的策略游戏",
    .shuffled_player_id_ = true, // 座位随机打乱，先手固定为 pid 0
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 4; }
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 变体) == 0 ? 1 : 0; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 2 || generic_options_readonly.PlayerNum() > 4) {
        reply() << "该游戏为 2 - 4 人游戏，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("配置游戏模式和运动变体",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const std::vector<int32_t>& tokens)
            {
                bool single_user = false;
                for (const int32_t t : tokens) {
                    switch (t) {
                        case 0: single_user = true; break;                      // 单机
                        case 1: GET_OPTION_VALUE(game_options, 变体) = 1; break; // 跳跃变体
                        case 2: GET_OPTION_VALUE(game_options, 变体) = 2; break; // 强制运动变体
                        default: break;
                    }
                }
                if (single_user) {
                    generic_options.bench_computers_to_player_num_ = 2;
                    return NewGameMode::SINGLE_USER;
                }
                return NewGameMode::MULTIPLE_USERS;
            },
            RepeatableChecker<AlterChecker<int32_t>>(std::map<std::string, int32_t>{
                {"单机", 0},
                {"跳跃", 1}, {"强制运动", 2},
            })),
};

// ========== GAME STAGES ==========

// 一个玩家的回合本身是「掷骰 → 选配对推进 → 继续/停止」的多步循环，
class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前棋盘，可用于图片重发", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "选择一个走法推进跑子，可选在推进后结算本回合", &MainStage::Choose_,
                    ArithChecker<uint32_t>(1, 6, "编号"), OptionalDefaultChecker<BoolChecker>(false, "停止", "继续")))
        , round_(1)
        , player_scores_(Global().PlayerNum(), 0)
        , left_(Global().PlayerNum(), false)
        , ever_busted_(Global().PlayerNum(), false)
        , ach_one_breath_(Global().PlayerNum(), false)
        , ach_climb_(Global().PlayerNum(), false)
        , ach_tripod_(Global().PlayerNum(), false)
        , ach_quad_(Global().PlayerNum(), false)
        , ach_obsessed_(Global().PlayerNum(), false)
        , ach_skyward_(Global().PlayerNum(), false)
        , rng_(MakeRng(""))
    {
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    virtual void OnStageBegin() override
    {
        // 初始化棋盘：收集玩家昵称与头像供 UI 使用
        std::vector<std::string> names, avatars;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            names.emplace_back(Global().PlayerName(pid));
            avatars.emplace_back(Global().PlayerAvatar(pid, 40));
        }
        board_.Initialize(Global().PlayerNum(), std::move(names), std::move(avatars), GAME_OPTION(胜利列数),
                          Global().ResourceDir(), static_cast<Variant>(GAME_OPTION(变体)));

        // 仅当前行动玩家（先手 pid 0）需要行动，其余玩家置为已就绪
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (pid.Get() != board_.turn_pid_) {
                Global().SetReady(pid);
            }
        }

        // 播报本局规则变体
        if (GAME_OPTION(变体) == 1) {
            Global().Boardcast() << "【跳跃变体】跑子推进时，若目标格已有其他玩家的棋子，将跳过该格、落到上方最近的空格。";
        } else if (GAME_OPTION(变体) == 2) {
            Global().Boardcast() << "【强制运动变体】若你的任一跑子与其他玩家的棋子同处一格，则本回合不能「停止」，必须继续掷骰直到离开。";
        }

        StartTurnRoll_();
        BroadcastBoard_();
        PromptTurn_();
    }

    // 超时：当前玩家直接失去本回合的全部临时进度（等同于爆掉），轮到下一位玩家。
    virtual CheckoutErrCode OnStageTimeout() override
    {
        Global().Boardcast() << "玩家 " << At(PlayerID(board_.turn_pid_)) << " 行动超时，本回合进度全部作废！";
        // 超时丢弃进度，但不视为「爆掉」，不影响「稳扎稳打」成就（仅真正爆掉才记录 ever_busted_）
        board_.ResetTurn(); // 丢弃本回合所有临时进度（不结算、不夺列）
        return SetupNextTurn_();
    }

    // 退出：与超时一致——退出者若正在行动，本回合的全部临时进度直接作废（不结算、不夺列）；
    // 随后退出玩家离场、判负（-1）、不再参与后续回合，且退出者永远不会被判定为获胜方。
    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        if (left_[pid]) {
            return StageErrCode::CONTINUE; // 已退出（理论上不会重复触发）
        }
        const bool was_current = (pid.Get() == board_.turn_pid_);
        if (was_current) {
            board_.ResetTurn(); // 退出者本回合进度直接作废（不结算、不夺列）
        }
        left_[pid] = true;
        player_scores_[pid] = -1; // 退出判负

        // 仅剩一名未退出玩家：其获得胜利（退出者恒为 -1、在场者得分 >= 0，故该玩家结算必为第一）
        if (AlivePlayers_() <= 1) {
            const int survivor = FirstAlivePlayer_();
            if (survivor >= 0) {
                Global().Boardcast() << "仅剩玩家 " << At(PlayerID(survivor)) << " 在场，获得游戏胜利！";
            }
            SetFinalScores_();
            return StageErrCode::CHECKOUT;
        }
        // 退出者正在行动：轮到下一位玩家
        if (was_current) {
            return SetupNextTurn_();
        }
        // 退出者非当前行动玩家：当前玩家继续行动，无需轮转
        return StageErrCode::CONTINUE;
    }

    // 电脑行动：以随机选择为主，配合「步数越多越倾向停手」的简单判断，单次调用完成整个回合。
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK; // 非当前行动玩家
        }
        int iter = 0;
        while (true) {
            // 已有进度时，按概率停手（步数越多概率越大）；强制运动变体下若跑子与对手同格则不能停
            if (board_.turn_has_progress_ && !board_.MustContinue() && static_cast<int>(RandInt(rng_, 1, 100)) <= AiStopChance_()) {
                return DoStop_();
            }
            if (board_.options_.empty()) {
                return DoStop_(); // 理论不会出现（爆掉已在别处处理），防御性停手
            }
            if (++iter > 40) { // 防御：极端情况下强制停手，避免理论死循环
                return DoStop_();
            }
            // 随机选择一个走法
            const int idx = static_cast<int>(RandInt(rng_, 0, static_cast<uint32_t>(board_.options_.size()) - 1));
            const MoveOption opt = board_.options_[idx];
            board_.ApplyOption(opt);
            Global().Boardcast() << "电脑 " << At(pid) << " 选择：" << OptionText_(opt);
            BroadcastBoard_();          // 每次选择后更新并重发盘面
            RollDiceAndCompute_();      // 然后重新掷骰（保留本回合已推进的白子）
            if (board_.IsBust()) {
                Global().Boardcast() << Markdown(board_.GetDiceMessageUI("没有任何可推进的走法——本回合「爆掉」，临时进度全部作废！"))
                                     << "电脑 " << At(pid) << " 无法行动，本回合进度作废！";
                ever_busted_[board_.turn_pid_] = true; // 记录「爆掉」（用于「稳扎稳打」成就）
                board_.ResetTurn();
                return SetupNextTurn_();
            }
            Global().Boardcast() << Markdown(board_.GetDiceOptionsUI());
        }
    }

    // 不会到达：回合内始终仅当前玩家处于未就绪状态，故 OnStageOver 永不触发。
    virtual CheckoutErrCode OnStageOver() override { return StageErrCode::CHECKOUT; }

  private:
    // 赛况：重发当前棋盘图片（含玩家列表、各列进度、本回合跑子与可选走法）。
    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << BoardMd_();
        if (pid.Get() == board_.turn_pid_) {
            reply() << Markdown(board_.GetDiceOptionsUI());
        }
        return StageErrCode::OK;
    }

    // 选择走法：仅当前玩家可用。先推进所选走法，再按 stop 决定结算本回合或继续掷骰。
    //   「编号」/「编号 继续」→ stop=false；「编号 停止」→ stop=true。
    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t index, const bool stop)
    {
        if (pid.Get() != board_.turn_pid_) {
            reply() << "[错误] 现在不是您的回合，当前行动玩家是：" << Global().PlayerName(PlayerID(board_.turn_pid_));
            return StageErrCode::FAILED;
        }
        const uint32_t total = static_cast<uint32_t>(board_.options_.size());
        if (index < 1 || index > total) {
            reply() << "[错误] 编号 " << index << " 超出范围，本次共有 " << total << " 个可选走法（1 - " << total << "）";
            return StageErrCode::FAILED;
        }
        board_.ApplyOption(board_.options_[index - 1]); // 先推进所选走法
        if (stop && board_.MustContinue()) {
            // 强制运动变体：跑子与其他玩家棋子同处一格，本回合无法停止，强制继续
            Global().Boardcast() << "【强制运动变体】" << At(pid) << " 的跑子与其他玩家的棋子同处一格，本回合无法停止，必须继续掷骰！";
            return ContinueAfterAdvance_();
        }
        if (stop) {
            return DoStop_(); // 「编号 停止」：推进后立即结算本回合
        }
        return ContinueAfterAdvance_(); // 「编号」/「编号 继续」：推进后重新掷骰继续
    }

    // 推进之后（同一玩家继续）：更新盘面 → 重新掷骰 → 爆掉则本回合作废并轮转；否则继续提示。
    AtomReqErrCode ContinueAfterAdvance_()
    {
        BroadcastBoard_();          // 每次选择后更新并重发盘面图片
        RollDiceAndCompute_();      // 然后重新掷骰（保留本回合已推进的白子）
        if (board_.IsBust()) {
            Global().Boardcast() << Markdown(board_.GetDiceMessageUI("没有任何可推进的走法——本回合「爆掉」，临时进度全部作废！"))
                                 << "玩家 " << At(PlayerID(board_.turn_pid_)) << " 无法行动，本回合进度作废！";
            ever_busted_[board_.turn_pid_] = true; // 记录「爆掉」（用于「稳扎稳打」成就）
            board_.ResetTurn();
            return SetupNextTurn_();
        }
        PromptTurn_(); // 同一玩家继续：盘面已更新，播报新骰子与走法并重置计时
        return StageErrCode::OK;
    }

    // 停止结算：提交进度、结算夺列、判定胜利；未胜则轮转下一玩家。
    CheckoutErrCode DoStop_()
    {
        const int pid = board_.turn_pid_;
        const std::array<int, COL_MAX + 1> pre_progress = board_.progress_[pid]; // 提交前的进度快照（用于「平步青云」判定）
        const std::vector<int> claimed = board_.Commit(pid);
        board_.ResetTurn(); // 收回白子（已转为彩色棋子），避免盘面同时出现白圈与已保存的棋子
        const bool reached_target = board_.ClaimedCountOf(pid) >= board_.target_columns_;
        // 成就记录（彼此独立，可同回合同时获得）：
        //   一气呵成（恰好 2 列）与三足鼎立（3 列及以上）按夺列数互斥；
        //   三列且均从起点 0 登顶 → 一飞冲天（任意规则，可与三足鼎立同时获得）；
        //   平步青云（本回合存在从 0 登顶的列）独立判定。
        int from_zero = 0;
        for (const int c : claimed) {
            if (pre_progress[c] == 0) {
                ++from_zero;
            }
        }
        const int n_claimed = static_cast<int>(claimed.size());
        if (n_claimed == 2) {
            ach_one_breath_[pid] = true;
        } else if (n_claimed >= 3) {
            ach_tripod_[pid] = true;
            if (from_zero == n_claimed) {
                ach_skyward_[pid] = true;
            }
        }
        if (from_zero > 0) {
            ach_climb_[pid] = true;
        }
        // 欲罢不能：本回合投掷达 20 次仍能成功停止（DoStop_ 即未爆掉）
        if (turn_rolls_ >= 20) {
            ach_obsessed_[pid] = true;
        }
        {
            auto sender = Global().Boardcast();
            sender << "玩家 " << At(PlayerID(pid)) << " 停止行动，本回合进度已保存";
            if (!claimed.empty()) {
                sender << "。夺得列：";
                for (size_t i = 0; i < claimed.size(); ++i) {
                    if (i) {
                        sender << "、";
                    }
                    sender << "第 " << claimed[i] << " 列";
                }
                sender << "！";
            }
            // 本轮「首位」达成胜利条件者，在「夺得列」之后追加一次性提示（后续达成者不再提示）
            if (reached_target && !game_ending_) {
                sender << "\n" << At(PlayerID(pid)) << " 已夺得 " << board_.target_columns_ << " 列，达成胜利条件！游戏将在本轮所有玩家行动完成后结束。";
            }
        }
        if (reached_target) {
            game_ending_ = true; // 进入「收尾本轮」：不立即结束，待本轮回绕时再结束
        }
        // 仅剩一名玩家（理论上退出处理已覆盖，此处防御）：立即结束
        if (AlivePlayers_() <= 1) {
            Global().Boardcast() << BoardMd_();
            SetFinalScores_();
            return StageErrCode::CHECKOUT;
        }
        // 轮转下一玩家；若本轮已有玩家达成目标，则在本轮所有人行动完成（回绕）时于 SetupNextTurn_ 内结束
        return SetupNextTurn_();
    }

    // 收尾结束：本轮全部行动完成、且已有玩家达成胜利条件时调用。结算得分、公布最终盘面与胜者。
    CheckoutErrCode EndGameByVictory_()
    {
        SetFinalScores_();
        Global().Boardcast() << BoardMd_();
        // 收集所有达成胜利条件（夺列数 >= 目标）的在场玩家——均为获胜玩家（按各自夺列数计分排名），全部播报
        std::vector<int> winners;
        for (int pid = 0; pid < static_cast<int>(Global().PlayerNum()); ++pid) {
            if (!left_[pid] && board_.ClaimedCountOf(pid) >= board_.target_columns_) {
                winners.push_back(pid);
            }
        }
        auto sender = Global().Boardcast();
        if (!winners.empty()) {
            for (size_t i = 0; i < winners.size(); ++i) {
                if (i) {
                    sender << "、";
                }
                sender << At(PlayerID(winners[i]));
            }
            sender << " 达成胜利条件，游戏结束！";
        } else {
            // 防御：达成者已全部退出，则由当前在场得分最高者获胜
            int best_pid = -1;
            int64_t best = 0;
            for (int pid = 0; pid < static_cast<int>(Global().PlayerNum()); ++pid) {
                if (!left_[pid] && (best_pid < 0 || player_scores_[pid] > best)) {
                    best = player_scores_[pid];
                    best_pid = pid;
                }
            }
            if (best_pid >= 0) {
                sender << "玩家 " << At(PlayerID(best_pid)) << " 获得游戏胜利！";
            }
        }
        return StageErrCode::CHECKOUT;
    }

    // 轮转到下一位未退出玩家并开始其回合：掷骰、算走法；若开局即爆掉则自动跳过。
    CheckoutErrCode SetupNextTurn_()
    {
        while (true) {
            const bool round_wrapped = RotateToNextPlayer_();
            // 已有玩家达成胜利条件，且本轮所有人行动完成（轮转回绕到本轮起点）→ 游戏结束
            if (game_ending_ && round_wrapped) {
                return EndGameByVictory_();
            }
            ++round_;
            // 回合数上限兜底：达到上限即结束（round_ 每轮递增、单调，故循环必然退出，无需额外计数器）
            if (round_ > static_cast<int>(GAME_OPTION(回合数))) {
                Global().Boardcast() << "已达到回合数上限，游戏结束，按夺列数与进度结算名次！";
                Global().Boardcast() << BoardMd_();
                SetFinalScores_();
                return StageErrCode::CHECKOUT;
            }
            // 所有列均已被夺取，无人能再行动 → 立即结束：有获胜者则公布胜者，否则按夺列数与进度结算（平局）
            if (board_.AllColumnsClaimed()) {
                if (game_ending_) {
                    return EndGameByVictory_();
                }
                Global().Boardcast() << "所有列均已被夺取，无人达成胜利条件，游戏结束，按夺列数与进度结算名次！";
                Global().Boardcast() << BoardMd_();
                SetFinalScores_();
                return StageErrCode::CHECKOUT;
            }
            StartTurnRoll_();
            if (!board_.IsBust()) {
                BroadcastBoard_();
                PromptTurn_();
                return StageErrCode::CONTINUE;
            }
            // 开局即无可推进走法：展示骰子图 + 红字提示，跳过该玩家
            Global().Boardcast() << Markdown(board_.GetDiceMessageUI("开局即无可推进的走法——本回合跳过"))
                                 << "玩家 " << At(PlayerID(board_.turn_pid_)) << " 无法行动，本回合跳过！";
        }
    }

    // 轮转 turn_pid_ 到下一位未退出玩家；前一玩家置就绪、新玩家解除就绪。
    // 返回是否「回绕」（新 pid 小于原 pid，即越过最后一名玩家回到本轮起点）——用于判定本轮结束。
    bool RotateToNextPlayer_()
    {
        const int prev = board_.turn_pid_;
        Global().SetReady(PlayerID(prev));
        const int n = static_cast<int>(Global().PlayerNum());
        int next = prev;
        for (int step = 0; step < n; ++step) {
            next = (next + 1) % n;
            if (!left_[next]) {
                break;
            }
        }
        board_.turn_pid_ = next;
        Global().ClearReady(PlayerID(next));
        return next < prev;
    }

    // 重发当前棋盘图片
    void BroadcastBoard_()
    {
        Global().Boardcast() << BoardMd_();
    }

    // 播报回合标题 + 当前骰子 + 可选走法，并重置计时器（不含棋盘图片）。
    void PromptTurn_()
    {
        auto sender = Global().Boardcast();
        // 框架会自动把 At() 渲染为「[N号：@玩家]」，此处只需 At() 本身
        sender << "请 " << At(PlayerID(board_.turn_pid_)) << " 行动";
        if (board_.turn_steps_ > 0) {
            sender << "，本回合已推进 " << board_.turn_steps_ << " 步";
        }
        sender << "（时限 " << GAME_OPTION(时限) << " 秒）\n发送「编号」推进并继续，或「编号 停止」推进并结算本轮（如：1 或 1 停止）";
        sender << "\n" << Markdown(board_.GetDiceOptionsUI());
        Global().StartTimer(GAME_OPTION(时限));
    }

    // 回合开始：收回白子、重置本回合临时状态，再掷骰算走法
    void StartTurnRoll_()
    {
        board_.ResetTurn();
        turn_rolls_ = 0; // 重置本回合投掷计数（用于「欲罢不能」成就）
        RollDiceAndCompute_();
    }

    // 仅重新掷骰并计算走法，保留本回合已推进的白子（同一回合内连续掷骰用）
    void RollDiceAndCompute_()
    {
        ++turn_rolls_; // 本回合投掷次数 +1（用于「欲罢不能」成就）
        for (int i = 0; i < DICE_COUNT; ++i) {
            board_.dice_[i] = static_cast<int>(RandInt(rng_, 1, 6));
        }
        board_.ComputeOptions();
        // 成就「一掷乾坤」：四颗骰子同点且本次掷骰未爆掉（仍有合法走法）
        if (board_.AllDiceEqual() && !board_.IsBust()) {
            ach_quad_[board_.turn_pid_] = true;
        }
    }

    // 单个走法的文本表示（用于「选择」播报）
    std::string OptionText_(const MoveOption& o) const
    {
        const std::string combo = std::to_string(o.pair_a) + "+" + std::to_string(o.pair_b);
        if (o.advances.size() == 2 && o.advances[0] == o.advances[1]) {
            return "[" + combo + "] 第 " + std::to_string(o.advances[0]) + " 列连进 2 格";
        }
        if (o.advances.size() == 2) {
            return "[" + combo + "] 推进 第 " + std::to_string(o.advances[0]) + " 列、第 " + std::to_string(o.advances[1]) + " 列";
        }
        return "[" + combo + "] 推进 第 " + std::to_string(o.advances[0]) + " 列";
    }

    // 棋盘图片消息：回合标题 + 棋盘 HTML
    Markdown BoardMd_()
    {
        return Markdown("## 第 " + std::to_string(round_) + " 回合\n\n" + board_.GetUI(), 820);
    }

    // 存活（未退出）玩家数
    int AlivePlayers_() const
    {
        return static_cast<int>(std::count(left_.begin(), left_.end(), false));
    }

    // 第一个未退出玩家的 pid，全部退出返回 -1
    int FirstAlivePlayer_() const
    {
        for (int pid = 0; pid < static_cast<int>(left_.size()); ++pid) {
            if (!left_[pid]) {
                return pid;
            }
        }
        return -1;
    }

    // 电脑停手概率（百分比）：本回合步数越多、占用跑子越多越倾向停手，封顶 80% 保留随机性。
    int AiStopChance_() const
    {
        int chance = 12 * board_.turn_steps_ + 15 * board_.RunnersUsed();
        return chance > 80 ? 80 : chance;
    }

    // 结算最终得分：
    //   退出玩家 = -1；
    //   达成胜利条件（夺列数 >= 胜利列数）的获胜玩家 = 100 * 实际夺得列数（夺得越多分越高，且必高于所有未获胜玩家）；
    //   其余玩家 = 夺列数 * 100 + 盘面上未被夺取列的棋子进度之和（已夺列棋子已移除/只显示★，不计进度）。
    void SetFinalScores_()
    {
        const int n = static_cast<int>(Global().PlayerNum());
        for (int pid = 0; pid < n; ++pid) {
            if (left_[pid]) {
                player_scores_[pid] = -1;
                continue;
            }
            if (board_.ClaimedCountOf(pid) >= board_.target_columns_) {
                player_scores_[pid] = static_cast<int64_t>(board_.ClaimedCountOf(pid)) * 100; // 获胜玩家 = 100 * 实际夺得列数
                continue;
            }
            // 进度仅统计仍在盘面上的棋子：已被夺取的列（含自己夺得的）
            const int total = board_.OnBoardProgressOf(pid);
            player_scores_[pid] = static_cast<int64_t>(board_.ClaimedCountOf(pid)) * 100 + total;
        }
        AwardAchievements_(); // 终局统一授予成就（SetFinalScores_ 仅在游戏结束时调用一次）
    }

    // 终局授予成就（在 SetFinalScores_ 内调用，覆盖所有结束路径，仅授予未退出玩家）
    void AwardAchievements_()
    {
        const int n = static_cast<int>(Global().PlayerNum());
        const bool default_rules = (GAME_OPTION(变体) == 0); // 无任何运动变体（跳跃/强制运动）
        for (int pid = 0; pid < n; ++pid) {
            if (ach_one_breath_[pid]) {
                Global().Achieve(PlayerID(pid), Achievement::一气呵成);
            }
            const int claimed = board_.ClaimedCountOf(pid);
            if (claimed > board_.target_columns_) {
                Global().Achieve(PlayerID(pid), Achievement::贪得无厌);
            }
            if (claimed >= board_.target_columns_ && !ever_busted_[pid]) {
                Global().Achieve(PlayerID(pid), Achievement::稳扎稳打);
            }
            if (ach_climb_[pid] && default_rules) {
                Global().Achieve(PlayerID(pid), Achievement::平步青云);
            }
            if (ach_skyward_[pid]) {
                Global().Achieve(PlayerID(pid), Achievement::一飞冲天);
            }
            if (ach_tripod_[pid]) {
                Global().Achieve(PlayerID(pid), Achievement::三足鼎立);
            }
            if (ach_quad_[pid]) {
                Global().Achieve(PlayerID(pid), Achievement::一掷乾坤);
            }
            if (ach_obsessed_[pid]) {
                Global().Achieve(PlayerID(pid), Achievement::欲罢不能);
            }
            // 通吃两极：同一局中同时夺得最难掷出的第 2 列与第 12 列（不要求获胜）
            if (board_.claimed_[COL_MIN] == pid && board_.claimed_[COL_MAX] == pid) {
                Global().Achieve(PlayerID(pid), Achievement::通吃两极);
            }
            // 独占鳌头：达成胜利，且全程没有任何对手夺得过任何一列
            if (claimed >= board_.target_columns_) {
                bool opponents_claimed_none = true;
                for (int q = 0; q < n; ++q) {
                    if (q != pid && board_.ClaimedCountOf(q) > 0) {
                        opponents_claimed_none = false;
                        break;
                    }
                }
                if (opponents_claimed_none) {
                    Global().Achieve(PlayerID(pid), Achievement::独占鳌头);
                }
            }
        }
    }

    int round_;                          // 回合计数（每位玩家的一次行动为一回合，用于展示）
    std::vector<int64_t> player_scores_; // 计分：获胜者 100*实际夺列数；其余 夺列数*100+进度和；退出 = -1
    std::vector<bool> left_;             // 玩家是否已退出
    std::vector<bool> ever_busted_;      // 是否曾「爆掉」（用于「稳扎稳打」成就；超时不计入）
    std::vector<bool> ach_one_breath_;   // 是否达成「一气呵成」（单回合恰好夺得 2 列）
    std::vector<bool> ach_climb_;        // 是否达成「平步青云」（单回合某列从底部推到顶端夺得）
    std::vector<bool> ach_tripod_;       // 是否达成「三足鼎立」（单回合一次夺得 3 列）
    std::vector<bool> ach_quad_;         // 是否达成「一掷乾坤」（掷出四颗同点且该次未爆掉）
    std::vector<bool> ach_obsessed_;     // 是否达成「欲罢不能」（单回合投掷达 20 次且未爆掉）
    std::vector<bool> ach_skyward_;      // 是否达成「一飞冲天」（单回合三列均从起点推到顶端）
    int turn_rolls_ = 0;                 // 当前回合已投掷次数（用于「欲罢不能」成就；每回合开始清零）
    bool game_ending_ = false;           // 本轮已有玩家达成胜利条件，待本轮所有人行动完成后结束
    Board board_;                        // 棋盘与全部游戏状态
    std::mt19937 rng_;                   // 骰子随机数发生器
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
