// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <algorithm>
#include <random>
#include <string>
#include <utility>
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
    .name_ = "迷宫",
    .developer_ = "铁蛋",
    .description_ = "双方私信绘制迷宫再互相挑战，在不可视的迷宫里率先抵达终点",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options{};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(ChildMsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    // 迷雾只在黑白模式下可启用，其余模式自动设回「无」
    if (GET_OPTION_VALUE(game_options, 模式) != GameMode::BLACK_WHITE &&
            GET_OPTION_VALUE(game_options, 迷雾) != FogMode::NONE) {
        GET_OPTION_VALUE(game_options, 迷雾) = FogMode::NONE;
        reply() << "[警告] 迷雾配置仅在黑白模式下生效，已自动设为「无」";
    }
    // 墙壁较多时绘制耗时明显增加，时限仍为默认值则自动延长
    if (GET_OPTION_VALUE(game_options, 墙数) >= LONG_DRAW_WALL_NUM && GET_OPTION_VALUE(game_options, 绘制时限) == DEFAULT_DRAW_TIME) {
        GET_OPTION_VALUE(game_options, 绘制时限) = LONG_DRAW_TIME;
        reply() << "[提示] 本局墙壁数量上限为 " << GET_OPTION_VALUE(game_options, 墙数) << "，已自动增加绘制时间。";
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("一键设置迷宫边长与墙壁数量上限",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options,
                const uint32_t& size, const uint32_t& wall_num)
            {
                GET_OPTION_VALUE(game_options, 边长) = size;
                GET_OPTION_VALUE(game_options, 墙数) = wall_num;
                return NewGameMode::MULTIPLE_USERS;
            },
            ArithChecker<uint32_t>(5, 9, "边长"),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(16, 5, 64, "墙壁数量")),
    InitOptionsCommand("一键开始黑白模式对局，可同时启用迷雾模式",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options,
                const FogMode& fog, const uint32_t& size, const uint32_t& wall_num)
            {
                GET_OPTION_VALUE(game_options, 模式) = GameMode::BLACK_WHITE;
                GET_OPTION_VALUE(game_options, 迷雾) = fog;
                GET_OPTION_VALUE(game_options, 边长) = size;
                GET_OPTION_VALUE(game_options, 墙数) = wall_num;
                return NewGameMode::MULTIPLE_USERS;
            },
            VoidChecker("黑白"),
            OptionalDefaultChecker<AlterChecker<FogMode>>(FogMode::NONE, map<string, FogMode>{
                {"无", FogMode::NONE}, {"当前", FogMode::CURRENT}, {"四周", FogMode::AROUND}}),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(5, 5, 9, "边长"),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(16, 5, 64, "墙壁数量")),
    InitOptionsCommand("一键开始边走边画模式对局",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options,
                const uint32_t& size, const uint32_t& wall_num)
            {
                GET_OPTION_VALUE(game_options, 模式) = GameMode::DRAW_WHILE_WALK;
                GET_OPTION_VALUE(game_options, 边长) = size;
                GET_OPTION_VALUE(game_options, 墙数) = wall_num;
                return NewGameMode::MULTIPLE_USERS;
            },
            VoidChecker("边走边画"),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(5, 5, 9, "边长"),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(16, 5, 64, "墙壁数量")),
    InitOptionsCommand("独自一人开始游戏，可设置边长与墙壁数量上限",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options,
                const uint32_t& size, const uint32_t& wall_num)
            {
                GET_OPTION_VALUE(game_options, 边长) = size;
                GET_OPTION_VALUE(game_options, 墙数) = wall_num;
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机"),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(5, 5, 9, "边长"),
            OptionalDefaultChecker<ArithChecker<uint32_t>>(16, 5, 64, "墙壁数量")),
};

// ========== GAME STAGES ==========

class DrawStage;
class TurnStage;

class MainStage : public MainGameStage<DrawStage, TurnStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , g(std::random_device{}())
        , size_(static_cast<int>(GAME_OPTION(边长)))
        , wall_limit_(static_cast<int>(GAME_OPTION(墙数)))
        , mode_(GAME_OPTION(模式))
        , fog_(GAME_OPTION(迷雾))
        , player_scores_(Global().PlayerNum(), 0)
        , board(players, size_, start_, goal_, mode_)
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(DrawStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(TurnStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    // 随机引擎
    std::mt19937 g;
    // 迷宫边长与每位玩家的墙壁数量上限
    int size_;
    int wall_limit_;
    // 游戏模式与黑白信息的可见范围
    GameMode mode_;
    FogMode fog_;
    // 双方共用的起点与终点
    Pos start_;
    Pos goal_;
    // 玩家，按 PlayerID 索引
    vector<Player> players;
    vector<int64_t> player_scores_;
    // 绘图
    Board board;

    // 回合数
    int turn_ = 0;
    // 当前行动的玩家
    PlayerID cur_pid_ = 0;
    // 游戏是否已结束
    bool game_over_ = false;

    PlayerID Opponent(const PlayerID pid) const { return PlayerID(pid == 0 ? 1 : 0); }

    // 该玩家正在挑战的迷宫，即对手绘制的那一张
    Maze& ChallengedMaze(const PlayerID pid) { return players[Opponent(pid)].maze; }

    // 黑白模式下公布玩家所在格附近的黑白信息
    void RevealParity(Maze& maze, const Pos& pos)
    {
        if (mode_ != GameMode::BLACK_WHITE) {
            return;
        }
        RevealParityByFog(maze, pos, fog_);
    }

    int StartId() const { return PosToId(start_, size_); }
    int GoalId() const { return PosToId(goal_, size_); }

    // 判负：退出或超时。若双方均已判负则视为无人获胜
    void MakeLose(const PlayerID pid, const string& reason)
    {
        if (players[pid].quit) {
            return;
        }
        players[pid].quit = true;
        player_scores_[pid] = -1;
        game_over_ = true;
        const PlayerID opponent = Opponent(pid);
        auto sender = Global().Boardcast();
        sender << At(pid) << " " << reason << "，判负";
        if (players[opponent].quit) {
            sender << "\n双方均未能完成游戏，游戏平局";
        } else {
            player_scores_[opponent] = 0;
            sender << "\n" << At(opponent) << " 获胜！";
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
    }

    // 终局公开双方迷宫的全部墙体
    void EndGame()
    {
        Global().Boardcast() << Markdown(board.GetFinalBoard(), Board::DualImageWidth(size_));
    }
};


// ========== 绘制阶段：各自私信绘制一座迷宫 ==========

class DrawStage : public SubGameStage<>
{
  public:
    DrawStage(MainStage& main_stage)
        : StageFsm(main_stage, "绘制阶段",
                MakeStageCommand(*this, "查看游戏进展：私信可查看自己绘制的迷宫",
                        &DrawStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "提交迷宫，提交后不可修改",
                        &DrawStage::Submit_, VoidChecker("提交")),
                MakeStageCommand(*this, "清空已放置的全部墙体",
                        &DrawStage::Clear_, VoidChecker("清空")),
                MakeStageCommand(*this, "放置/移除墙体，可一次放置多面；如果已有墙体则移除",
                        &DrawStage::Place_, RepeatableChecker<BasicChecker<string>>("墙壁", "10左 7s 3R")))
    {}

    virtual void OnStageBegin() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << "请私信绘制迷宫，本局墙体上限 " << Main().wall_limit_ << " 面，时限 " << GAME_OPTION(绘制时限) << " 秒\n"
                    "放置/移除墙体：<编号><方向>，可一次多面，如「10左 7s 3R」\n"
                    "清空全部墙体：「清空」\n"
                    "完成后「提交」，提交后不可修改\n"
                    << (Main().mode_ == GameMode::DRAW_WHILE_WALK
                            ? "\n[边走边画] 对战中可继续加墙，建议预留部分额度\n" : "")
                    << Markdown(Main().board.GetDrawView(pid, Main().wall_limit_), Board::ImageWidth(Main().size_));
        }
        Global().Boardcast() << "请双方私信裁判绘制自己的迷宫，时限 " << GAME_OPTION(绘制时限)
                             << " 秒\n超时将自动提交当前迷宫，迷宫不合规则判负\n\n"
                             << "网页草稿本：" << DRAFT_URL;
        Global().StartTimer(GAME_OPTION(绘制时限));
    }

  private:
    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown(Main().board.GetDrawPublic(), Board::ImageWidth(Main().size_));
        } else {
            reply() << Markdown(Main().board.GetDrawView(pid, Main().wall_limit_), Board::ImageWidth(Main().size_));
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode Place_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const vector<string>& tokens)
    {
        vector<WallRef> refs;
        string err;
        // 本指令没有指令名，未知消息都会落到这里，因此先解析再校验渠道与状态
        if (!ParseWallTokens(tokens, Main().size_, refs, err)) {
            reply() << err << "\n放置/移除多面墙壁，如「10左 7s 3R」\n本阶段可用指令：<编号><方向> / 清空 / 提交 / 赛况";
            return StageErrCode::FAILED;
        }
        if (!CheckEditable_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Maze& maze = Main().players[pid].maze;
        int added = 0;
        int removed = 0;
        for (const auto& ref : refs) {
            if (maze.ToggleWall(ref)) {
                ++added;
            } else {
                ++removed;
            }
        }
        auto sender = reply();
        sender << "放置 " << added << " 面墙";
        if (removed > 0) {
            sender << "，移除 " << removed << " 处已有墙体";
        }
        AppendCount_(sender, pid);
        sender << Markdown(Main().board.GetDrawView(pid, Main().wall_limit_), Board::ImageWidth(Main().size_));
        return StageErrCode::OK;
    }

    AtomReqErrCode Clear_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (!CheckEditable_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Main().players[pid].maze.ClearWalls();
        reply() << "已清空全部墙体，当前 0 / " << Main().wall_limit_ << " 面\n"
                << Markdown(Main().board.GetDrawView(pid, Main().wall_limit_), Board::ImageWidth(Main().size_));
        return StageErrCode::OK;
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经提交迷宫，无法修改";
            return StageErrCode::FAILED;
        }
        Maze& maze = Main().players[pid].maze;
        const int count = maze.WallCount();
        if (count > Main().wall_limit_) {
            reply() << "[错误] 当前共 " << count << " 面墙，超出上限 " << Main().wall_limit_ << " 面，请先移除 "
                    << (count - Main().wall_limit_) << " 面后再提交（重复放置即可移除）";
            return StageErrCode::FAILED;
        }
        if (!maze.Reachable(Main().start_, Main().goal_)) {
            reply() << "[错误] 起点 " << Main().StartId() << " 号格与终点 " << Main().GoalId()
                    << " 号格之间已经没有通路，请调整墙体后再提交";
            return StageErrCode::FAILED;
        }
        Main().players[pid].submitted = true;
        reply() << "提交成功，请等待对手完成绘制";
        return StageErrCode::READY;
    }

    // 改动墙体的指令的公共校验：必须私信，且尚未提交
    bool CheckEditable_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判绘制迷宫";
            return false;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经提交迷宫，无法修改";
            return false;
        }
        return true;
    }

    template <typename Sender>
    void AppendCount_(Sender& sender, const PlayerID pid)
    {
        const Maze& maze = Main().players[pid].maze;
        const int count = maze.WallCount();
        sender << "\n当前共 " << count << " / " << Main().wall_limit_ << " 面";
        if (count > Main().wall_limit_) {
            sender << "\n[警告] 已超出上限 " << (count - Main().wall_limit_) << " 面，需移除后才能提交（重复放置即可移除）";
        } else if (!maze.Reachable(Main().start_, Main().goal_)) {
            sender << "\n[警告] 起点与终点之间已经没有通路，需调整后才能提交";
        } else if (count == Main().wall_limit_) {
            sender << "\n墙壁已用完，可「提交」迷宫";
        }
        sender << "\n";
    }

    // 超时未提交：迷宫本身合规则自动提交；不合规则直接判负
    virtual CheckoutErrCode OnStageTimeout() override
    {
        // 先统一处理可以自动提交的一方，再处理判负的一方，避免判负公告插在自动提交公告之前
        vector<PlayerID> auto_submit;
        vector<std::pair<PlayerID, string>> lose;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Global().IsReady(pid)) {
                continue;
            }
            const Maze& maze = Main().players[pid].maze;
            const int count = maze.WallCount();
            if (count > Main().wall_limit_) {
                lose.emplace_back(pid, "绘制阶段超时，且已放置的 " + to_string(count) + " 面墙超出上限 " +
                        to_string(Main().wall_limit_) + " 面，迷宫无法提交");
            } else if (!maze.Reachable(Main().start_, Main().goal_)) {
                lose.emplace_back(pid, "绘制阶段超时，且起点与终点之间没有通路，迷宫无法提交");
            } else {
                auto_submit.push_back(pid);
            }
        }
        for (const PlayerID pid : auto_submit) {
            Player& player = Main().players[pid];
            player.submitted = true;
            Global().SetReady(pid);
            Global().Tell(pid) << "您超时未提交，已自动提交当前迷宫，共 " << player.maze.WallCount() << " 面墙";
            Global().Boardcast() << At(pid) << " 超时未提交，已自动提交当前迷宫";
        }
        for (const auto& [pid, reason] : lose) {
            Main().MakeLose(pid, reason);
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
        player.maze.GenerateForComputer(Main().g, Main().wall_limit_, Main().start_, Main().goal_);
        player.submitted = true;
        return StageErrCode::READY;
    }
};


// ========== 行动阶段：轮流探索对手迷宫 ==========

class TurnStage : public SubGameStage<>
{
  public:
    TurnStage(MainStage& main_stage, const int turn)
        : StageFsm(main_stage, "第 " + std::to_string(turn) + " 回合",
                MakeStageCommand(*this, "查看当前局面：私信可查看自己绘制的迷宫",
                        &TurnStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "[边走边画] 私信在自己的迷宫中追加墙体，可一次追加多面",
                        &TurnStage::AddWall_, TokenListChecker(true, "墙壁", "10左 7上")),
                MakeStageCommand(*this, "移动自己，可一次输入多个方向连续移动",
                        &TurnStage::Act_, TokenListChecker(false, "移动方向", "上上左")))
        , turn_(turn)
    {}

    virtual void OnStageBegin() override
    {
        const PlayerID cur = Main().cur_pid_;
        Main().players[cur].BeginTurn();
        // 轮流行动，非行动方本回合无需等待
        Global().SetReady(Main().Opponent(cur));
        Global().Boardcast() << "轮到 " << At(cur) << " 移动\n"
                "可一次输入多个方向连续移动。方向：上下左右 / UDLR / sxzy\n"
                "每步时限 " << GAME_OPTION(行动时限) << " 秒，超时判负\n"
                << (Main().mode_ == GameMode::DRAW_WHILE_WALK
                        ? "\n[边走边画] 可私信「<编号><方向>」往继续加墙\n" : "")
                << Markdown(Main().board.GetDualBoard(-1, turn_, static_cast<int>(cur)), Board::DualImageWidth(Main().size_));
        Global().StartTimer(GAME_OPTION(行动时限));
    }

  private:
    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply)
    {
        reply() << Markdown(Main().board.GetDualBoard(is_public ? -1 : static_cast<int>(pid), turn_,
                        static_cast<int>(Main().cur_pid_)), Board::DualImageWidth(Main().size_));
        return StageErrCode::OK;
    }

    // 边走边画：自己回合可以往自己绘制的迷宫里追加墙体，位置允许是对手已经走通的路
    AtomReqErrCode AddWall_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply,
            const vector<string>& tokens)
    {
        if (Main().mode_ != GameMode::DRAW_WHILE_WALK) {
            reply() << "[错误] 只有「边走边画」模式才能在对战阶段加墙";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid) || pid != Main().cur_pid_) {
            reply() << "[错误] 当前不是您的回合，请等待对手行动";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判加墙";
            return StageErrCode::FAILED;
        }
        vector<WallRef> refs;
        string err;
        if (!ParseWallTokens(tokens, Main().size_, refs, err)) {
            reply() << err;
            return StageErrCode::FAILED;
        }
        Maze& maze = Main().players[pid].maze;
        const int rest = Main().wall_limit_ - maze.WallCount();
        if (static_cast<int>(refs.size()) > rest) {
            reply() << "[错误] 本局墙壁上限 " << Main().wall_limit_ << " 面，当前还能追加 " << rest << " 面，本次添加将超出上限";
            return StageErrCode::FAILED;
        }
        // 先在副本上试放：不得落在已有墙体上，也不得切断对手当前位置到终点的通路
        Maze trial = maze;
        for (const auto& ref : refs) {
            if (trial.WallValue(ref)) {
                reply() << "[错误] " << WallName(ref, Main().size_) << "已经有墙壁了，或本条指令内重复指定了同一面墙";
                return StageErrCode::FAILED;
            }
            trial.SetWallValue(ref, true);
        }
        const Player& rival = Main().players[Main().Opponent(pid)];
        if (!trial.Reachable(rival.pawn, Main().goal_)) {
            reply() << "[错误] 加墙必须保留对手当前所在位置到终点的通路";
            return StageErrCode::FAILED;
        }
        for (const auto& ref : refs) {
            maze.SetWallValue(ref, true);
            maze.MarkAdded(ref);
        }
        reply() << "已追加 " << refs.size() << " 面墙，当前共 " << maze.WallCount() << " / " << Main().wall_limit_
                << " 面\n对手不会收到任何提示，新墙在被撞上之前保持隐藏\n"
                << Markdown(Main().board.GetSingleBoard(Main().Opponent(pid), static_cast<int>(pid)),
                        Board::ImageWidth(Main().size_));
        return StageErrCode::OK;
    }

    AtomReqErrCode Act_(const PlayerID pid, const bool is_public, ChildMsgSenderBase& reply, const vector<string>& tokens)
    {
        if (Global().IsReady(pid) || pid != Main().cur_pid_) {
            reply() << "[错误] 当前不是您的回合，请等待对手行动";
            return StageErrCode::FAILED;
        }
        string text;
        for (const auto& token : tokens) {
            text += token;
        }
        vector<Direct> directs;
        string err;
        if (!ParseDirectSequence(text, directs, err)) {
            reply() << err << "\n本阶段可用指令：<方向...> / 赛况";
            return StageErrCode::FAILED;
        }
        // 预检：整条指令按公开信息模拟一遍，撞上已知墙体或地图边界则整体拒绝
        const Maze& maze = Main().ChallengedMaze(pid);
        Pos probe = Main().players[pid].pawn;
        for (size_t i = 0; i < directs.size(); ++i) {
            const Direct direct = directs[i];
            const char* const dir_text = dir_cn[static_cast<int>(direct)].data();
            if (maze.IsKnownBlocked(probe, direct)) {
                reply() << "[错误] 第 " << (i + 1) << " 步：" << PosName(probe, Main().size_) << "的" << dir_text
                        << "侧" << (IsBorder(probe, direct, Main().size_) ? "是地图边界" : "有一面已知墙壁")
                        << "，无法通行，本次未执行移动";
                return StageErrCode::FAILED;
            }
            if (maze.HasWall(probe, direct)) {
                break;  // 会撞上尚未暴露的墙，本回合到此为止，其后的方向无需校验
            }
            probe = MovePos(probe, direct);
        }
        return Execute_(pid, directs);
    }

    // 依次执行全部方向，撞上尚未暴露的墙则停止并结束回合
    AtomReqErrCode Execute_(const PlayerID pid, const vector<Direct>& directs)
    {
        Maze& maze = Main().ChallengedMaze(pid);
        Player& player = Main().players[pid];
        const int size = Main().size_;
        int moved = 0;
        bool hit = false;
        bool win = false;
        Pos hit_pos = player.pawn;
        Direct hit_direct = Direct::UP;
        for (const Direct direct : directs) {
            if (maze.HasWall(player.pawn, direct)) {
                const auto ref = WallAt(player.pawn, direct, size);
                if (!ref.has_value()) {
                    break;  // 地图边界已在预检中拒绝，此处仅作兜底
                }
                maze.Reveal(*ref);
                player.AddStep(direct, StepResult::HIT_WALL);
                hit = true;
                hit_pos = player.pawn;
                hit_direct = direct;
                break;
            }
            if (const auto ref = WallAt(player.pawn, direct, size); ref.has_value()) {
                maze.MarkPassed(*ref);
            }
            player.pawn = MovePos(player.pawn, direct);
            player.MarkVisited(player.pawn);
            Main().RevealParity(maze, player.pawn);
            ++moved;
            if (player.pawn == Main().goal_) {
                player.AddStep(direct, StepResult::GOAL);
                win = true;
                break;
            }
            player.AddStep(direct, StepResult::MOVE);
        }

        if (win) {
            Global().Boardcast() << At(pid) << " 移动 " << moved << " 步，抵达终点 " << Main().GoalId() << " 号格！\n"
                    "本回合轨迹：" << Board::GetTurnTrace(player.record);
            Main().FinishWin(pid, "率先抵达终点");
            return StageErrCode::CHECKOUT;
        }
        if (hit) {
            const char* const dir_text = dir_cn[static_cast<int>(hit_direct)].data();
            Global().Boardcast() << At(pid) << " 移动 " << moved << " 步，在 " << PosName(hit_pos, size) << "向" << dir_text << "撞上了墙壁！\n"
                    "本回合轨迹：" << Board::GetTurnTrace(player.record);
            return StageErrCode::READY;
        }
        // 未撞墙，本回合继续，重置计时器
        Global().Boardcast() << At(pid) << " 移动 " << moved << " 步，当前位于 " << PosName(player.pawn, size) << "，请继续行动。\n"
                << Markdown(Main().board.GetSingleBoard(pid, -1), Board::ImageWidth(size));
        Global().StartTimer(GAME_OPTION(行动时限));
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                Main().MakeLose(pid, "行动超时");
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
        if (Global().IsReady(pid) || pid != Main().cur_pid_) {
            return StageErrCode::OK;
        }
        // 只依据公开信息规划一条通往终点的路线，沿其行进要么抵达终点，要么撞出一面新的墙
        const Maze& maze = Main().ChallengedMaze(pid);
        const Pos& pawn = Main().players[pid].pawn;
        vector<Direct> path = maze.OptimisticPath(pawn, Main().goal_, Main().g);
        if (path.empty()) {
            return StageErrCode::READY;
        }
        // 小概率改走次优路线：禁用最优路线的第一步后重新规划，增加随机性
        if (static_cast<int>(Main().g() % 100) < COMPUTER_ALT_PATH_PERCENT) {
            const vector<Direct> alternative = maze.OptimisticPath(pawn, Main().goal_, Main().g, path.front());
            if (!alternative.empty()) {
                path = alternative;
            }
        }
        const AtomReqErrCode rc = Execute_(pid, path);
        if (rc == StageErrCode::OK) {
            return StageErrCode::READY;
        }
        return rc;
    }

    const int turn_;
};


// ========== 主阶段流程 ==========

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    const int cell_num = size_ * size_;
    // 随机生成起点与终点，两点不重叠，双方迷宫完全一致
    const int start_id = static_cast<int>(g() % cell_num) + 1;
    int goal_id = static_cast<int>(g() % (cell_num - 1)) + 1;
    if (goal_id >= start_id) {
        ++goal_id;
    }
    start_ = IdToPos(start_id, size_);
    goal_ = IdToPos(goal_id, size_);

    players.reserve(Global().PlayerNum());
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        players.emplace_back(pid, PlayerNickname(Global().PlayerName(pid)), Global().PlayerAvatar(pid, 40));
        players.back().maze.Reset(size_);
        players.back().StartBattle(start_);
    }

    Global().Boardcast() << Markdown(board.GetOpeningBoard(), Board::ImageWidth(size_));
    setter.Emplace<DrawStage>(*this);
}

void MainStage::NextStageFsm(DrawStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (game_over_) {
        EndGame();
        return;
    }
    // 黑白模式：无迷雾时开局即公布全部格子，有迷雾时先公布起点附近
    if (mode_ == GameMode::BLACK_WHITE) {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (fog_ == FogMode::NONE) {
                players[pid].maze.RevealAllParity();
            } else {
                RevealParityByFog(players[pid].maze, start_, fog_);
            }
        }
    }
    // 系统随机决定先手玩家
    cur_pid_ = static_cast<PlayerID>(g() % 2);
    turn_ = 1;
    Global().Boardcast() << "双方迷宫均已提交，对战开始！\n先手：" << At(cur_pid_);
    setter.Emplace<TurnStage>(*this, turn_);
}

void MainStage::NextStageFsm(TurnStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (game_over_) {
        EndGame();
        return;
    }
    cur_pid_ = Opponent(cur_pid_);
    // 退出的玩家已经判负，正常不会走到这里，仅作兜底
    if (players[cur_pid_].quit) {
        EndGame();
        return;
    }
    setter.Emplace<TurnStage>(*this, ++turn_);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
