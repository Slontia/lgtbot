// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <algorithm>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

#include "constants.h"
#include "maze.h"
#include "player.h"
#include "board.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "法则迷宫",
    .developer_ = "铁蛋",
    .description_ = "在共同生成的未知迷宫中夺取水晶戒指，并将其带往对手起点",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options{
        .is_formal_{false},
};
const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看情报阶段可选情报的完整说明",
            []() { return k_intel_detail; },
            VoidChecker("情报")),
};

bool AdaptOptions(ChildMsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
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

class SetupStage;
class IntelStage;
class RoundStage;

class MainStage : public MainGameStage<SetupStage, IntelStage, RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展：私信查询可获得自己视角的迷宫地图", &MainStage::Status_, VoidChecker("赛况")))
        , g(std::random_device{}())
        , player_scores_(Global().PlayerNum(), 0)
        , board(maze, players, Global().ResourceDir())
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(SetupStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(IntelStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    // 随机引擎
    std::mt19937 g;
    // 迷宫
    Maze maze;
    // 玩家，按 PlayerID 索引
    vector<Player> players;
    vector<int64_t> player_scores_;
    // 绘图
    Board board;

    // 回合数
    int round_ = 0;
    // 迷宫是否已生成
    bool maze_ready_ = false;
    // 游戏是否已结束
    bool game_over_ = false;
    // 公开事件记录
    string public_record_;

    // 宝物是否仍停留在中心格
    bool treasure_at_center_ = true;
    // 宝物冻结剩余回合数，冻结期间任何人都无法取得
    int treasure_freeze_ = 0;
    // 戒指被取得的回合数，0 表示尚未被取得。对手位置从该回合的下一回合起公开
    int treasure_taken_round_ = 0;
    // 本回合是否关闭相遇判定
    bool no_encounter_ = false;
    // 下回合是否关闭相遇判定
    bool pending_no_encounter_ = false;
    // 本局写入情报B/C/D 的贯穿线，每局随机但双方一致
    vector<LineRef> intel_lines_;

    PlayerID Opponent(const PlayerID pid) const { return PlayerID(pid == 0 ? 1 : 0); }

    ViewInfo MakeViewInfo()
    {
        // 夺宝当回合不公开对手位置，需等到下一回合开始
        const bool reveal_rival = (treasure_taken_round_ > 0 && round_ > treasure_taken_round_);
        return ViewInfo{round_, static_cast<int>(GAME_OPTION(回合数)), treasure_at_center_, treasure_freeze_,
                no_encounter_, reveal_rival};
    }

    // 追加一条公开事件记录
    void AddRecord(const string& text)
    {
        if (!public_record_.empty()) {
            public_record_ += "<br>";
        }
        public_record_ += "【第 " + to_string(round_) + " 回合】" + text;
    }

    // 回合开始：结算跨回合状态
    void BeginRound()
    {
        no_encounter_ = pending_no_encounter_;
        pending_no_encounter_ = false;
        for (auto& player : players) {
            player.BeginRound(round_);
        }
    }

    // 判负：退出或超时。若双方均已判负则视为平局
    void MakeLose(const PlayerID pid, const string& reason)
    {
        if (players[pid].quit) {
            return;
        }
        players[pid].quit = true;
        players[pid].stop_reason = StopReason::QUIT;
        players[pid].treasure = false;
        player_scores_[pid] = -1;
        game_over_ = true;
        const PlayerID opponent = Opponent(pid);
        auto sender = Global().Boardcast();
        sender << At(pid) << " " << reason << "，判负";
        if (players[opponent].quit) {
            sender << "\n双方均未能完成游戏，本局无人获胜";
            AddRecord(players[pid].name + " " + reason + "，判负；双方均已判负");
        } else {
            player_scores_[opponent] = 0;
            sender << "\n" << At(opponent) << " 获胜！";
            AddRecord(players[pid].name + " " + reason + "，判负，" + players[opponent].name + " 获胜");
        }
    }

    void FinishWin(const PlayerID pid, const string& reason)
    {
        if (game_over_) {
            return;
        }
        game_over_ = true;
        player_scores_[pid] = 1;
        player_scores_[Opponent(pid)] = 0;
        Global().Boardcast() << "游戏结束！" << At(pid) << " " << reason << "，获得胜利！";
        AddRecord(players[pid].name + " " + reason + "，获得胜利");
    }

    void FinishDraw(const string& reason)
    {
        if (game_over_) {
            return;
        }
        game_over_ = true;
        for (auto& score : player_scores_) {
            score = 0;
        }
        Global().Boardcast() << "游戏结束！" << reason << "，本局为平局";
        AddRecord(reason + "，本局为平局");
    }

    // 到达回合数上限时的判定：持有戒指者未能送达，判负；双方都没有戒指则比拼走过的格子数
    void JudgeByRoundLimit()
    {
        Global().Boardcast() << "已到达最大回合数 " << GAME_OPTION(回合数) << "，开始判定胜负";
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (players[pid].treasure) {
                FinishWin(Opponent(pid), "因对手在回合数耗尽时仍持有水晶戒指、未能将其送达");
                return;
            }
        }
        if (players[0].visited_count != players[1].visited_count) {
            const PlayerID winner = players[0].visited_count > players[1].visited_count ? PlayerID(0) : PlayerID(1);
            FinishWin(winner, "在双方均未持有水晶戒指的情况下走过的格子更多");
            return;
        }
        FinishDraw("双方均未持有水晶戒指，且走过的格子数相同");
    }

    // 终局公开真实迷宫
    void EndGame()
    {
        if (!maze_ready_) {
            return;
        }
        Global().Boardcast() << Markdown(board.GetFinalBoard(MakeViewInfo(), public_record_), Board::ImageWidth() * 2);
    }

    // 每局随机抽取贯穿线写入情报B/C/D，双方一致
    void DrawIntelLines()
    {
        vector<int> candidates(LINE_CANDIDATE_NUM);
        for (int i = 0; i < LINE_CANDIDATE_NUM; ++i) {
            candidates[i] = i;
        }
        std::shuffle(candidates.begin(), candidates.end(), g);
        intel_lines_.clear();
        for (int i = 0; i < LINE_INTEL_NUM; ++i) {
            intel_lines_.push_back(MakeLine(candidates[i]));
        }
    }

    // 情报的展示名称，贯穿线情报附带本局抽到的具体线路
    string IntelLabel(const Intel intel) const
    {
        const int line_index = LineIntelIndex(intel);
        if (line_index >= 0 && line_index < static_cast<int>(intel_lines_.size())) {
            return "贯穿线 " + LineName(intel_lines_[line_index]);
        }
        return IntelTitle(intel);
    }

    // 单条情报的说明。贯穿线填入本局抽到的线路，捷径填入该玩家对应的两个端点
    string IntelDescription(const PlayerID pid, const Intel intel) const
    {
        const int line_index = LineIntelIndex(intel);
        if (line_index >= 0) {
            if (line_index >= static_cast<int>(intel_lines_.size())) {
                return "贯穿线尚未抽取";
            }
            return "贯穿线 " + LineName(intel_lines_[line_index]) + "上有几面墙阻挡";
        }
        switch (intel) {
            case Intel::WALL_COUNT:
                return "全部横线格与全部竖线格上各有多少个墙（两者分别告知）";
            case Intel::PATHS:
                return "排除经过 " + PosName(k_center) + " 的通路，一共有几条通路可以到达对方的起点";
            case Intel::SHORTCUT: {
                const int index = players[pid].index;
                return PosName(k_shortcut[index][0]) + " 与 " + PosName(k_shortcut[index][1]) +
                        " 之间若存在不经过 " + PosName(k_center) + " 且长度不超过 " + to_string(SHORTCUT_MAX_LENGTH) +
                        " 步的通路，则获知通路并标注在你的地图上";
            }
            case Intel::OPPONENT_A:
                return "对手的信息 A 是多少，也就是对方在你起点周围放置了多少墙";
            default:
                break;
        }
        return "[未知情报]";
    }

    // 情报阶段的完整菜单：逐项列出，本局随机与因人而异的内容直接填入对应选项
    string IntelMenu(const PlayerID pid) const
    {
        string menu;
        for (int i = 0; i < INTEL_TYPE_NUM; ++i) {
            const Intel intel = static_cast<Intel>(i);
            if (i > 0) {
                menu += "\n";
            }
            menu += IntelCode(intel) + "：" + IntelDescription(pid, intel);
        }
        return menu;
    }

    // 生成情报答复
    string AnswerIntel(const PlayerID pid, const Intel intel)
    {
        Player& player = players[pid];
        const int line_index = LineIntelIndex(intel);
        if (line_index >= 0) {
            return to_string(maze.CountLineWalls(intel_lines_[line_index])) + " 面墙阻挡";
        }
        switch (intel) {
            case Intel::WALL_COUNT: return "横线 " + to_string(maze.CountWalls(true)) + " 面，竖线 " + to_string(maze.CountWalls(false)) + " 面";
            case Intel::PATHS: {
                const long long paths = maze.CountSimplePaths(player.Start(), player.OpponentStart());
                if (paths < 0) {
                    return "超过 " + to_string(PATH_COUNT_LIMIT) + " 条（已达统计上限）";
                }
                return to_string(paths) + " 条";
            }
            case Intel::SHORTCUT:   return RevealShortcut(player);
            case Intel::OPPONENT_A: return to_string(players[Opponent(pid)].info_a) + " 面";
            default:                break;
        }
        return "[未知情报]";
    }

    // 情报F：揭示离自己更近的两点之间的一条短通路，并标注到该玩家的地图上
    // 通路不得经过中心格，因此只有经过中心格才能走通的情况同样视为没有可用通路
    string RevealShortcut(Player& player)
    {
        const Pos from = k_shortcut[player.index][0];
        const Pos to = k_shortcut[player.index][1];
        const string endpoints = PosName(from) + " 与 " + PosName(to);
        const vector<Pos> path = maze.ShortestPath(from, to, g, true);
        if (path.empty()) {
            return endpoints + " 之间没有不经过 " + PosName(k_center) + " 且不超过 8 步的通路，本条情报无收获";
        }
        const int length = static_cast<int>(path.size()) - 1;
        if (length > SHORTCUT_MAX_LENGTH) {
            return endpoints + " 之间没有不经过 " + PosName(k_center) + " 且不超过 8 步的通路，本条情报无收获";
        }
        // 标注沿途格子与通路，玩家仍需真正走过才计入走过的格子数
        for (size_t i = 0; i < path.size(); ++i) {
            player.revealed[path[i].c][path[i].r] = true;
            if (i + 1 >= path.size()) {
                continue;
            }
            for (int d = 0; d < 4; ++d) {
                const Direct direct = static_cast<Direct>(d);
                if (Maze::Move(path[i], direct) != path[i + 1]) {
                    continue;
                }
                if (const auto ref = Maze::WallAt(path[i], direct); ref.has_value()) {
                    player.PassWall(*ref);
                }
                break;
            }
        }
        return endpoints + " 之间存在不经过 " + PosName(k_center) + " 的通路，长度为 " + to_string(length) + " 步，已标注在你的地图上";
    }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown(board.GetPublicStatus(MakeViewInfo(), public_record_), Board::TextImageWidth());
        } else if (!maze_ready_) {
            reply() << "迷宫尚未生成，请先在准备阶段提交 信息A 与 信息B";
        } else {
            reply() << Markdown(board.GetPlayerView(pid, MakeViewInfo()), Board::ImageWidth());
        }
        return StageErrCode::OK;
    }
};


// ========== 准备阶段：提交 信息A 与 信息B ==========

class SetupStage : public SubGameStage<>
{
  public:
    SetupStage(MainStage& main_stage)
        : StageFsm(main_stage, "准备阶段",
                MakeStageCommand(*this, "提交开局信息：信息A 为对手起点三面的墙数，信息B 与对手的 信息B 相加决定迷宫墙数",
                        CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &SetupStage::Submit_,
                        ArithChecker<int>(INFO_A_MIN, INFO_A_MAX, "信息A"), ArithChecker<int>(INFO_B_MIN, INFO_B_MAX, "信息B")))
    {}

    virtual void OnStageBegin() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << "请私信提交开局信息，格式：<信息A> <信息B>\n"
                    "信息A（" << INFO_A_MIN << "~" << INFO_A_MAX << "）：你将在对手起点 "
                    << PosName(Main().players[pid].OpponentStart()) << " 的三面放置的墙数\n"
                    "信息B（" << INFO_B_MIN << "~" << INFO_B_MAX << "）：双方信息B 之和 S 决定迷宫墙数，"
                    "迷宫总墙数为 S×2+3，A~C 列与 E~G 列范围内各有 S 面墙\n"
                    "例如：1 5";
        }
        Global().Boardcast() << "请双方私信裁判提交信息A 与信息B，时限 " << GAME_OPTION(准备时限) << " 秒，超时判负";
        Global().StartTimer(GAME_OPTION(准备时限));
    }

  private:
    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const int info_a, const int info_b)
    {
        Player& player = Main().players[pid];
        player.info_a = info_a;
        player.info_b = info_b;
        reply() << "提交成功\n信息A = " << info_a << "：对手起点 " << PosName(player.OpponentStart())
                << " 的三面将存在 " << info_a << " 面墙\n信息B = " << info_b;
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                Main().MakeLose(pid, "准备阶段超时未提交开局信息");
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().MakeLose(pid, "退出游戏");
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        Player& player = Main().players[pid];
        player.info_a = INFO_A_MIN + static_cast<int>(Main().g() % (INFO_A_MAX - INFO_A_MIN + 1));
        player.info_b = INFO_B_MIN + static_cast<int>(Main().g() % (INFO_B_MAX - INFO_B_MIN + 1));
        return StageErrCode::READY;
    }
};


// ========== 情报阶段：选择三条情报 ==========

class IntelStage : public SubGameStage<>
{
  public:
    IntelStage(MainStage& main_stage)
        : StageFsm(main_stage, "情报阶段",
                MakeStageCommand(*this, "选择三条互不相同的情报", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                        &IntelStage::Choose_, AlterChecker<Intel>(intel_map), AlterChecker<Intel>(intel_map), AlterChecker<Intel>(intel_map)))
    {}

    virtual void OnStageBegin() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            // 先发一张示意图，标出本局 B/C/D 对应的贯穿线
            Global().Tell(pid) << Markdown(Main().board.GetIntelLineBoard(Main().intel_lines_, Main().MakeViewInfo()),
                    Board::ImageWidth());
            Global().Tell(pid) << "请私信选择三条互不相同的情报，格式：<X> <Y> <Z>\n"
                    << Main().IntelMenu(pid) << "\n\n例如：A C F";
        }
        Global().Boardcast() << "请双方私信裁判选择三条互不相同的情报，时限 " << GAME_OPTION(准备时限) << " 秒，超时判负";
        Global().StartTimer(GAME_OPTION(准备时限));
    }

  private:
    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply,
            const Intel first, const Intel second, const Intel third)
    {
        if (first == second || first == third || second == third) {
            reply() << "[错误] 三条情报必须互不相同";
            return StageErrCode::FAILED;
        }
        Player& player = Main().players[pid];
        player.intels = {first, second, third};
        reply() << "提交成功，已选择情报：" << IntelCode(first) << " " << IntelCode(second) << " " << IntelCode(third)
                << "\n答复将在双方均完成选择后统一私信送达";
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                Main().MakeLose(pid, "情报阶段超时未选择情报");
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().MakeLose(pid, "退出游戏");
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        vector<Intel> all;
        for (int i = 0; i < INTEL_TYPE_NUM; ++i) {
            all.push_back(static_cast<Intel>(i));
        }
        std::shuffle(all.begin(), all.end(), Main().g);
        Main().players[pid].intels.assign(all.begin(), all.begin() + INTEL_COUNT);
        return StageErrCode::READY;
    }
};


// ========== 回合阶段：双方同时逐步行动 ==========

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const int round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand(*this, "选择方向行动一步", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY,
                        &RoundStage::Act_, AlterChecker<Direct>(direction_map)))
        , submitted_(main_stage.Global().PlayerNum(), Direct::UP)
        , step_msg_(main_stage.Global().PlayerNum())
    {}

    virtual void OnStageBegin() override
    {
        Main().BeginRound();
        {
            auto sender = Global().Boardcast();
            sender << Name() << "开始，双方同时行动，每回合最多 " << MAX_STEP << " 步，请私信裁判选择方向";
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (Main().players[pid].stun_rest) {
                    sender << "\n" << At(pid) << " 上回合被击晕，本回合强制停止行动";
                }
            }
            if (Main().no_encounter_) {
                sender << "\n本回合双方失去相遇判定，各走各的";
            }
            if (Main().treasure_freeze_ > 0) {
                sender << "\n本回合【水晶戒指】停留在 " << PosName(k_center) << "，任何人都无法取得";
            }
        }
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().players[pid];
            if (!player.NeedAct()) {
                Global().SetReady(pid);
                continue;
            }
            auto tell = Global().Tell(pid);
            tell << "第 " << Main().round_ << " 回合开始，请私信选择方向（上下左右 / sxzy / UDLR），每步时限 "
                 << GAME_OPTION(行动时限) << " 秒，超时判负";
            if (player.forbid) {
                tell << "\n【限制】本回合第一步必须离开 " << PosName(player.forbid_pos)
                     << "（不得撞墙停留），且本回合内不得再次进入该格";
            }
            tell << "\n" << Markdown(Main().board.GetPlayerView(pid, Main().MakeViewInfo()), Board::ImageWidth());
        }
        Global().StartTimer(GAME_OPTION(行动时限));
    }

  private:
    AtomReqErrCode Act_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const Direct direct)
    {
        Player& player = Main().players[pid];
        if (player.quit) {
            reply() << "[错误] 您已不在游戏中";
            return StageErrCode::FAILED;
        }
        if (player.stun_rest) {
            reply() << "[错误] 您上回合被击晕，本回合强制停止行动，无需行动";
            return StageErrCode::FAILED;
        }
        if (player.IsStopped()) {
            reply() << "[错误] 您本回合已停止行动，请等待对手完成本回合";
            return StageErrCode::FAILED;
        }
        // 击晕对手后的下一回合限制：第一步必须离开该格，且本回合不得再次进入
        if (player.forbid) {
            const bool has_wall = Main().maze.HasWall(player.pos, direct);
            if (player.steps_used == 0 && has_wall) {
                reply() << "[错误] 击晕对手后，本回合第一步必须离开 " << PosName(player.forbid_pos)
                        << "，不得撞墙停留，请选择无墙的方向";
                return StageErrCode::FAILED;
            }
            if (!has_wall && Maze::Move(player.pos, direct) == player.forbid_pos) {
                reply() << "[错误] 本回合内不得再次进入 " << PosName(player.forbid_pos);
                return StageErrCode::FAILED;
            }
        }
        submitted_[pid] = direct;
        reply() << "已提交第 " << (player.steps_used + 1) << " 步：向 "
                << dir_cn[static_cast<int>(direct)] << " 移动，等待对手行动后统一结算";
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        ResolveStep();
        if (Main().game_over_ || AllStopped()) {
            FinishRound();
            return StageErrCode::CHECKOUT;
        }
        // 进入下一步：必须先清空全部准备状态，再将无需行动的玩家标记为已准备
        Global().ClearReady();
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Main().players[pid].NeedAct()) {
                Global().SetReady(pid);
            }
        }
        Global().StartTimer(GAME_OPTION(行动时限));
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().players[pid].NeedAct() && !Global().IsReady(pid)) {
                Main().MakeLose(pid, "行动超时");
            }
        }
        FinishRound();
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().MakeLose(pid, "退出游戏");
        FinishRound();
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, ChildMsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        Player& player = Main().players[pid];
        if (!player.NeedAct()) {
            return StageErrCode::OK;
        }
        submitted_[pid] = ChooseComputerDirect(player);
        return StageErrCode::READY;
    }

    bool AllStopped() const
    {
        for (const auto& player : Main().players) {
            if (player.NeedAct()) {
                return false;
            }
        }
        return true;
    }

    // 结算本步：先统一移动，再依次判定胜利、相遇与宝物归属
    void ResolveStep()
    {
        for (auto& msg : step_msg_) {
            msg.clear();
        }
        // 玩家之间不互相阻挡，因此依次结算移动与同时结算等价
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().players[pid];
            player.moved_this_step = false;
            if (!player.NeedAct()) {
                continue;
            }
            const Direct direct = submitted_[pid];
            const int d = static_cast<int>(direct);
            ++player.steps_used;
            step_msg_[pid] += "[第 " + to_string(player.steps_used) + " 步] 向" + string(dir_cn[d]) + "行动";
            if (Main().maze.HasWall(player.pos, direct)) {
                const auto ref = Maze::WallAt(player.pos, direct);
                if (ref.has_value()) {
                    player.DiscoverWall(*ref);
                    step_msg_[pid] += "\n撞上了【墙壁】！" + PosName(player.pos) + " 的" + string(dir_cn[d]) +
                            "方存在墙壁，本回合强制停止行动";
                } else {
                    step_msg_[pid] += "\n撞上了【迷宫边框】！本回合强制停止行动";
                }
                player.AddStep(direct, StepResult::HIT_WALL);
                player.stop_reason = StopReason::WALL;
            } else {
                // 走通的位置确认无墙，记入自己的已知信息
                if (const auto ref = Maze::WallAt(player.pos, direct); ref.has_value()) {
                    player.PassWall(*ref);
                }
                player.pos = Maze::Move(player.pos, direct);
                const bool is_new = player.MarkVisited(player.pos);
                player.AddStep(direct, is_new ? StepResult::NEW_GRID : StepResult::MOVE);
                player.moved_this_step = true;
                step_msg_[pid] += "\n成功移动至 " + PosName(player.pos);
                if (is_new) {
                    step_msg_[pid] += "，这是一个新的格子，走过格子数增加至 " + to_string(player.visited_count);
                }
            }
        }

        // 携带宝物抵达对手起点即刻获胜，该判定优先于相遇判定
        if (!CheckVictory()) {
            const optional<PlayerID> winner = ResolveEncounter();
            SettleTreasure(winner);
            CheckVictory();
        }

        // 步数耗尽的玩家停止行动，该判定在相遇判定之后
        for (auto& player : Main().players) {
            if (!player.quit && !player.stun_rest && !player.IsStopped() && player.steps_used >= MAX_STEP) {
                player.stop_reason = StopReason::STEP_OUT;
                step_msg_[player.pid] += "\n本回合 " + to_string(MAX_STEP) + " 步已用尽，停止行动";
            }
        }

        SendStepMessage();
    }

    // 相遇判定，返回击晕成功的一方
    optional<PlayerID> ResolveEncounter()
    {
        Player& first = Main().players[0];
        Player& second = Main().players[1];
        if (first.quit || second.quit || Main().no_encounter_ || first.pos != second.pos) {
            return nullopt;
        }
        const bool at_center = (first.pos == k_center && Main().treasure_at_center_ && Main().treasure_freeze_ == 0);
        if (first.moved_this_step && second.moved_this_step) {
            if (first.visited_count != second.visited_count) {
                const PlayerID winner = first.visited_count > second.visited_count ? PlayerID(0) : PlayerID(1);
                Knockout(winner, Main().Opponent(winner), at_center ? "情况四" : "情况二");
                return winner;
            }
            // 双方走过的格子数相同：本回合与下回合均不再触发相遇判定。在中心格相遇属于情况四，平局时同样按情况三处理
            Main().no_encounter_ = true;
            Main().pending_no_encounter_ = true;
            auto sender = Global().Boardcast();
            const char* const situation = at_center ? "情况四" : "情况三";
            sender << "【" << situation << "】\n"
                   << "双方在 " << PosName(first.pos) << " 相遇，且走过的格子数相同，" << (at_center ? "按情况三处理，" : "") << "无事发生"
                   << "\n本回合剩余步数与下一回合均不再触发相遇判定";
            string record = string("【") + situation + "】双方在 " + PosName(first.pos) + " 相遇，走过格子数相同，无事发生";
            if (at_center) {
                Main().treasure_freeze_ = 2;
                sender << "\n【水晶戒指】本回合与下回合都将停留在 " << PosName(k_center) << "，任何人都无法取得";
                record += "；水晶戒指冻结两个回合";
            }
            Main().AddRecord(record);
            for (auto& player : Main().players) {
                player.AppendTag("相遇");
            }
            return nullopt;
        }
        if (first.moved_this_step != second.moved_this_step) {
            // 一方已停止，另一方移动至其所在格
            const PlayerID winner = first.moved_this_step ? PlayerID(0) : PlayerID(1);
            Knockout(winner, Main().Opponent(winner), "情况一");
            return winner;
        }
        return nullopt;
    }

    // 击晕结算
    void Knockout(const PlayerID winner, const PlayerID loser, const char* const situation)
    {
        Player& win_player = Main().players[winner];
        Player& lose_player = Main().players[loser];
        win_player.stop_reason = StopReason::KNOCKOUT;
        if (!lose_player.IsStopped()) {
            lose_player.stop_reason = StopReason::STUNNED;
        }
        lose_player.pending_stun = true;
        win_player.pending_forbid = true;
        win_player.pending_forbid_pos = win_player.pos;
        win_player.AppendTag("击晕");
        lose_player.AppendTag("被击晕");

        const bool rob = lose_player.treasure;
        if (rob) {
            lose_player.treasure = false;
            win_player.treasure = true;
            win_player.AppendTag("夺宝");
        }
        {
            auto sender = Global().Boardcast();
            sender << "【" << situation << "】\n"
                   << At(winner) << " 在 " << PosName(win_player.pos) << " 击晕了 " << At(loser) << "，双方本回合均停止行动";
            if (rob) {
                sender << "\n" << At(winner) << " 夺走了对手携带的【水晶戒指】！";
            }
            sender << "\n下回合必须离开 " << PosName(win_player.pos) << " 且不得再次进入该格";
        }
        Main().AddRecord(string("【") + situation + "】" + win_player.name + " 在 " + PosName(win_player.pos) +
                " 击晕了 " + lose_player.name + (rob ? "，并夺走了水晶戒指" : ""));
        TellSurroundingWalls(winner);
    }

    // 私信击晕方所在格四周的墙壁信息，并计入其已探明的墙壁
    void TellSurroundingWalls(const PlayerID pid)
    {
        Player& player = Main().players[pid];
        string info;
        for (int d = 0; d < 4; ++d) {
            const Direct direct = static_cast<Direct>(d);
            const bool has_wall = Main().maze.HasWall(player.pos, direct);
            // 四面信息一并计入已知：有墙记为墙壁，无墙记为可通行
            if (const auto ref = Maze::WallAt(player.pos, direct); ref.has_value()) {
                if (has_wall) {
                    player.DiscoverWall(*ref);
                } else {
                    player.PassWall(*ref);
                }
            }
            info += string(dir_cn[d]) + (has_wall ? "墙" : "空") + " ";
        }
        step_msg_[pid] += "\n\n【击晕方情报】" + PosName(player.pos) + " 四周墙壁信息（上下左右）：" + info +
                "\n下回合第一步必须走向无墙方向离开该格，且下回合内不得再次进入";
    }

    // 宝物归属结算
    void SettleTreasure(const optional<PlayerID>& encounter_winner)
    {
        if (!Main().treasure_at_center_ || Main().treasure_freeze_ > 0) {
            return;
        }
        vector<PlayerID> at_center;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Main().players[pid].quit && Main().players[pid].pos == k_center) {
                at_center.push_back(pid);
            }
        }
        optional<PlayerID> taker;
        if (at_center.size() == 1) {
            taker = at_center.front();
        } else if (at_center.size() > 1 && encounter_winner.has_value()) {
            // 双方同处中心格时由本步相遇判定的胜者取得
            taker = *encounter_winner;
        }
        if (!taker.has_value()) {
            return;
        }
        Main().treasure_at_center_ = false;
        Main().treasure_taken_round_ = Main().round_;
        Main().players[*taker].treasure = true;
        Main().players[*taker].AppendTag("夺宝");
        // 公屏提醒附带地图，仅公开夺宝者位于中心格，不泄露对手位置与墙壁信息
        Global().Boardcast() << "【宝物公告】\n"
                             << At(*taker) << " 抵达了 " << PosName(k_center) << "，取得了【水晶戒指】！\n"
                             << "带着戒指抵达对手起点即可获胜\n"
                             << "此后每回合结束都将在公屏公开双方所在格\n"
                             << Markdown(Main().board.GetTreasureBoard(*taker, Main().MakeViewInfo()), Board::ImageWidth());
        // 同时私信未取得戒指的一方，直接附带戒指图片
        const PlayerID loser = Main().Opponent(*taker);
        if (!Main().players[loser].quit) {
            Global().Tell(loser) << "【宝物公告】\n"
                                 << "对手已在 " << PosName(k_center) << " 取得【水晶戒指】！\n"
                                 << "对方带着戒指抵达你的起点 " << PosName(Main().players[loser].Start()) << " 就会获胜，务必设法拦截！\n"
                                 << Image(string(Global().ResourceDir()) + "ring.png");
        }
        Main().AddRecord(Main().players[*taker].name + " 在 " + PosName(k_center) + " 取得了水晶戒指");
    }

    // 胜利判定：携带宝物抵达对手起点
    bool CheckVictory()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().players[pid];
            if (player.quit || !player.treasure || player.pos != player.OpponentStart()) {
                continue;
            }
            player.AppendTag("胜利");
            Main().FinishWin(pid, "携带水晶戒指抵达对手起点 " + PosName(player.pos));
            return true;
        }
        return false;
    }

    void SendStepMessage()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (step_msg_[pid].empty() || Main().players[pid].quit) {
                continue;
            }
            Global().Tell(pid) << step_msg_[pid] << "\n"
                               << Markdown(Main().board.GetPlayerView(pid, Main().MakeViewInfo()), Board::ImageWidth());
        }
    }

    void FinishRound()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Player& player = Main().players[pid];
            player.EndRound(StopReasonText(player.stop_reason));
            // 记录回合结束位置，戒指被取得后会在下回合开始时向对手公开
            player.last_round_pos = player.pos;
            player.has_last_round_pos = true;
            if (Main().game_over_ || player.quit) {
                continue;
            }
            Global().Tell(pid) << "第 " << Main().round_ << " 回合结束（" << StopReasonText(player.stop_reason) << "）\n"
                               << "本回合轨迹：" << Board::GetRoundTrace(player.record) << "\n"
                               << "当前位置 " << PosName(player.pos) << "，累计走过 " << player.visited_count << " 格";
        }
        if (Main().treasure_freeze_ > 0) {
            --Main().treasure_freeze_;
        }
        // 水晶戒指被取得之后，每回合结束都公开双方所在格
        if (!Main().game_over_ && !Main().treasure_at_center_) {
            auto sender = Global().Boardcast();
            sender << "第 " << Main().round_ << " 回合结束";
            sender << "\n" << Markdown(Main().board.GetPositionBoard(Main().MakeViewInfo()), Board::ImageWidth());
        }
    }

    static const char* StopReasonText(const StopReason reason)
    {
        switch (reason) {
            case StopReason::NONE:      return "尚未停止";
            case StopReason::WALL:      return "撞墙停止";
            case StopReason::STEP_OUT:  return "步数用尽";
            case StopReason::STUNNED:   return "被对手击晕";
            case StopReason::KNOCKOUT:  return "击晕对手后停止";
            case StopReason::STUN_REST: return "被击晕后强制停止";
            case StopReason::QUIT:      return "已退出游戏";
        }
        return "未知";
    }

    // 电脑玩家行动：以随机为主，并带有向目标靠近的倾向
    Direct ChooseComputerDirect(const Player& player)
    {
        const Pos target = player.treasure ? player.OpponentStart() : k_center;
        const auto distance = [](const Pos& from, const Pos& to) { return abs(from.c - to.c) + abs(from.r - to.r); };
        vector<Direct> closer;
        vector<Direct> valid;
        vector<Direct> fallback;
        for (int d = 0; d < 4; ++d) {
            const Direct direct = static_cast<Direct>(d);
            const Pos next = Maze::Move(player.pos, direct);
            const bool has_wall = Main().maze.HasWall(player.pos, direct);
            // 击晕方的下回合限制：第一步不得撞墙，且不得再次进入指定格
            if (player.forbid) {
                if (player.steps_used == 0 && has_wall) {
                    continue;
                }
                if (!has_wall && next == player.forbid_pos) {
                    continue;
                }
            }
            fallback.push_back(direct);
            // 电脑不使用未探明的墙壁信息，仅规避边框与自己撞到过的墙
            if (!InMaze(next) || player.KnowsWall(player.pos, direct)) {
                continue;
            }
            valid.push_back(direct);
            if (distance(next, target) < distance(player.pos, target)) {
                closer.push_back(direct);
            }
        }
        if (!closer.empty() && Main().g() % 100 < 70) {
            return closer[Main().g() % closer.size()];
        }
        if (!valid.empty()) {
            return valid[Main().g() % valid.size()];
        }
        if (!fallback.empty()) {
            return fallback[Main().g() % fallback.size()];
        }
        return Direct::UP;
    }

    vector<Direct> submitted_;
    vector<string> step_msg_;
};


// ========== 主阶段流程 ==========

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    const int first_index = static_cast<int>(g() % 2);
    players.reserve(Global().PlayerNum());
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        players.emplace_back(pid, PlayerNickname(Global().PlayerName(pid)), Global().PlayerAvatar(pid, 40),
                pid == 0 ? first_index : 1 - first_index);
    }
    auto sender = Global().Boardcast();
    sender << "【代达洛斯的法则迷宫】\n"
           << "迷宫为 7×7，字母 A~G 表示从左到右的 7 列，数字 1~7 表示从上到下的 7 行\n"
           << "水晶戒指位于正中央 " << PosName(k_center) << "，双方需要夺取戒指并将其带往对手的起点\n\n"
           << "本局起点分配：";
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        sender << "\n" << At(pid) << " → " << PosName(players[pid].Start());
    }
    setter.Emplace<SetupStage>(*this);
}

void MainStage::NextStageFsm(SetupStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (game_over_) {
        EndGame();
        return;
    }
    // 双方信息B 之和决定单个区域的墙数；对手的信息A 决定己方起点三面的墙数
    const int region_wall = players[0].info_b + players[1].info_b;
    int walls_at_start[2] = {0, 0};
    for (const auto& player : players) {
        walls_at_start[player.index] = players[Opponent(player.pid)].info_a;
    }
    if (!maze.Generate(g, region_wall, walls_at_start[0], walls_at_start[1])) {
        Global().Boardcast() << "[错误] 迷宫生成失败，游戏无法继续，请联系管理员";
        FinishDraw("迷宫生成失败");
        return;
    }
    maze_ready_ = true;
    // 抽取本局写入情报B/C/D 的贯穿线，双方一致
    DrawIntelLines();
    // 迷宫绘制完毕后公开底图，仅展示双方起点与水晶戒指的位置，不泄露任何墙壁信息
    Global().Boardcast() << "迷宫已按照双方提交的信息绘制完毕，总墙数为 " << (region_wall * 2 + CENTER_COLUMN_WALL) << " 面（不含边框）\n"
                         << Markdown(board.GetOpeningBoard(MakeViewInfo()), Board::ImageWidth());
    setter.Emplace<IntelStage>(*this);
}

void MainStage::NextStageFsm(IntelStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (game_over_) {
        EndGame();
        return;
    }
    ++round_;
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        Player& player = players[pid];
        {
            auto tell = Global().Tell(pid);
            tell << "【情报答复】";
            for (const Intel intel : player.intels) {
                const string answer = AnswerIntel(pid, intel);
                player.answers.push_back(answer);
                tell << "\n" << IntelCode(intel) << "（" << IntelLabel(intel) << "）：" << answer;
            }
            tell << "\n\n以下是你的地图视角，可随时使用「赛况」指令重新查看";
        }
    }
    Global().Boardcast() << "情报已私信送达，比赛正式开始！";
    setter.Emplace<RoundStage>(*this, round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (game_over_) {
        EndGame();
        return;
    }
    if (round_ >= static_cast<int>(GAME_OPTION(回合数))) {
        JudgeByRoundLimit();
        EndGame();
        return;
    }
    setter.Emplace<RoundStage>(*this, ++round_);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
