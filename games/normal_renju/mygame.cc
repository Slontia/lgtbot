// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/coding.h"
#include "game_util/renju.h"

using namespace lgtbot::game_util::renju;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "五子棋", // the game name which should be unique among all the games
    .developer_ = "森高",
    .description_ = "采用 swap2 规则的无禁手五子棋游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << generic_options_readonly.PlayerNum();
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

enum class State { INIT, SWAP_1, SWAP_2, PLACE };

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")),
                MakeStageCommand(*this, "pass", &MainStage::Pass_, VoidChecker("pass")),
                MakeStageCommand(*this, "假先落三子", &MainStage::InitSet_,
                    AnyArg("黑棋坐标"), AnyArg("白棋坐标"), AnyArg("黑棋坐标")),
                MakeStageCommand(*this, "假后落两子", &MainStage::Swap1Set_, AnyArg("白棋坐标"), AnyArg("黑棋坐标")),
                MakeStageCommand(*this, "落一子", &MainStage::Set_, AnyArg("坐标")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , board_(Global().ResourceDir(), BoardOptions{.to_expand_board_ = false, .is_overline_win_ = false})
        , turn_pid_(rand() % 2)
        , black_pid_(turn_pid_)
        , state_(State::INIT)
        , last_round_passed_(false)
    {
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << Markdown(HtmlHead_() + board_.ToHtml());
        Global().Boardcast() << "请" << At(turn_pid_) << "行动，时限 " << GAME_OPTION(时限) << " 秒，下前 3 手棋（如「J10 H9 E8」）";
        Global().SetReady(1 - turn_pid_);
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        const auto to_continue = HandlePass_();
        return RoundOver_(to_continue);
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(pid) << "强退认负";
        player_scores_[1 - pid] = 1;
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        board_.ClearHighlight();
        const auto set = [&](const AreaType type)
            {
                uint32_t x, y;
                do {
                    x = rand() % Board::k_size_;
                    y = rand() % Board::k_size_;
                } while (!board_.CanBeSet(x, y));
                return board_.Set(x, y, type);
            };
        assert(turn_pid_ == pid);
        if (state_ == State::INIT) {
            set(AreaType::BLACK);
            set(AreaType::WHITE);
            set(AreaType::BLACK);
            state_ = State::SWAP_1;
        } else if (state_ == State::SWAP_1) {
            switch (rand() % 3) {
            case 0:
                set(AreaType::WHITE);
                state_ = State::PLACE;
                break;
            case 1:
                set(AreaType::WHITE);
                set(AreaType::BLACK);
                state_ = State::SWAP_2;
                break;
            default:
                HandlePass_();
                state_ = State::PLACE;
            }
        } else if (state_ == State::SWAP_2) {
            if (rand() % 2) {
                set(AreaType::WHITE);
            } else {
                HandlePass_();
            }
        } else {
            const auto result = set(black_pid_ == pid ? AreaType::BLACK : AreaType::WHITE);
            if (result == Result::WIN_WHITE || result == Result::WIN_BLACK) {
                player_scores_[pid] = 1;
                Global().Boardcast() << "玩家" << At(pid) << "五子连胜";
            }
            return RoundOver_(result == Result::CONTINUE_OK);
        }
        return RoundOver_(true);
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        assert(false); // will continue or checkout directly
        return StageErrCode::CHECKOUT;
    }

  private:
    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (pid != turn_pid_) {
            reply() << "pass 失败：不是您的回合";
            return StageErrCode::FAILED;
        }
        const auto to_continue = HandlePass_();
        board_.ClearHighlight();
        return RoundOver_(to_continue);
    }

    std::optional<std::pair<uint32_t, uint32_t>> DecodePos_(const std::string& str, MsgSenderBase& reply)
    {
        const auto decode_res = DecodePos<Board::k_size_, Board::k_size_>(str);
        if (const auto* const errstr = std::get_if<std::string>(&decode_res)) {
            reply() << "落子失败：" << *errstr;
            return std::nullopt;
        }
        const auto coor = std::get<std::pair<uint32_t, uint32_t>>(decode_res);
        if (!board_.CanBeSet(coor.first, coor.second)) {
            reply() << "落子失败：" << str << " 位置无法落子，请试试其它位置吧";
            return std::nullopt;
        }
        return coor;
    }

    AtomReqErrCode InitSet_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::string& black_1_pos_str, const std::string& white_pos_str, const std::string& black_2_pos_str)
    {
        if (pid != turn_pid_) {
            reply() << "落子失败：不是您的回合";
            return StageErrCode::FAILED;
        }
        if (state_ != State::INIT) {
            reply() << "落子失败：当前并非假先落子阶段";
            return StageErrCode::FAILED;
        }
        const auto black_1_pos = DecodePos_(black_1_pos_str, reply);
        const auto white_pos = DecodePos_(white_pos_str, reply);
        const auto black_2_pos = DecodePos_(black_2_pos_str, reply);
        if (!black_1_pos.has_value() || !white_pos.has_value() || !black_2_pos.has_value()) {
            return StageErrCode::FAILED;
        }
        if (black_1_pos == black_2_pos || black_1_pos == white_pos || black_2_pos == white_pos) {
            reply() << "落子失败：落子点不允许相同";
            return StageErrCode::FAILED;
        }
        board_.ClearHighlight();
        board_.Set(black_1_pos->first, black_1_pos->second, AreaType::BLACK);
        board_.Set(white_pos->first, white_pos->second, AreaType::WHITE);
        board_.Set(black_2_pos->first, black_2_pos->second, AreaType::BLACK);
        state_ = State::SWAP_1;
        return RoundOver_(true);
    }

    AtomReqErrCode Swap1Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
            const std::string& white_pos_str, const std::string& black_pos_str)
    {
        if (pid != turn_pid_) {
            reply() << "落子失败：不是您的回合";
            return StageErrCode::FAILED;
        }
        if (state_ != State::SWAP_1) {
            reply() << "落子失败：当前并非假后落子阶段";
            return StageErrCode::FAILED;
        }
        const auto white_pos = DecodePos_(white_pos_str, reply);
        const auto black_pos = DecodePos_(black_pos_str, reply);
        if (!white_pos.has_value() || !black_pos.has_value()) {
            return StageErrCode::FAILED;
        }
        if (black_pos == white_pos) {
            reply() << "落子失败：落子点不允许相同";
            return StageErrCode::FAILED;
        }
        board_.ClearHighlight();
        board_.Set(white_pos->first, white_pos->second, AreaType::WHITE);
        board_.Set(black_pos->first, black_pos->second, AreaType::BLACK);
        state_ = State::SWAP_2;
        return RoundOver_(true); // the board must not be filled and any color must not achiave line
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& pos_str)
    {
        if (pid != turn_pid_) {
            reply() << "落子失败：不是您的回合";
            return StageErrCode::FAILED;
        }
        if (state_ == State::INIT) {
            reply() << "落子失败：当前为假先落子阶段，需要一次性下前 3 手棋（如「J10 H9 E8」）";
            return StageErrCode::FAILED;
        }
        const auto pos = DecodePos_(pos_str, reply);
        if (!pos.has_value()) {
            return StageErrCode::FAILED;
        }
        if (state_ == State::SWAP_1 || state_ == State::SWAP_2) {
            black_pid_ = 1 - pid;
            state_ = State::PLACE;
            Global().Boardcast() << "玩家" << At(pid) << "决定落子，使用白棋";
        }
        last_round_passed_ = false;
        board_.ClearHighlight();
        const auto result = board_.Set(pos->first, pos->second, black_pid_ == pid ? AreaType::BLACK : AreaType::WHITE);
        if (result == Result::WIN_WHITE || result == Result::WIN_BLACK) {
            player_scores_[pid] = 1;
            Global().Boardcast() << "玩家" << At(pid) << "五子连胜";
        }
        return RoundOver_(result == Result::CONTINUE_OK);
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(HtmlHead_() + board_.ToHtml());
        return StageErrCode::OK;
    }

    CheckoutErrCode RoundOver_(const bool to_continue)
    {
        turn_pid_ = 1 - turn_pid_;
        Global().Boardcast() << Markdown(HtmlHead_() + board_.ToHtml());
        if (!to_continue) {
            return StageErrCode::CHECKOUT;
        }
        ++round_;
        Global().ClearReady();
        Global().SetReady(1 - turn_pid_);
        if (state_ == State::SWAP_1 || state_ == State::SWAP_2) {
            Global().Boardcast() << "请" << At(turn_pid_) << "行动，时限 " << GAME_OPTION(时限) << " 秒\n您有几种选择："
                    "\n1. 回复一个坐标以落子一颗白棋（如「J10」）：若如此行动，您本局执白"
                    "\n2. pass 一手：若如此行动，您本局执黑"
                << (state_ == State::SWAP_1 ? "\n3. 回复两个坐标以分别落子一颗白棋和一颗黑棋（如「J10 G9」），若如此行动，则由对手决定执黑还是执白" : "");
        } else if (state_ == State::PLACE) {
            Global().Boardcast() << "请" << At(turn_pid_) << "回复坐标以落子（如 J10），时限 " << GAME_OPTION(时限) << " 秒";
        } else if (state_ == State::INIT) {
            // the first round player give up the turn
            Global().Boardcast() << "请" << At(turn_pid_) << "行动，时限 " << GAME_OPTION(时限) << " 秒，下前 3 手棋（如「J10 H9 E8」）";
        } else {
            assert(false);
        }
        Global().StartTimer(GAME_OPTION(时限));
        return StageErrCode::CONTINUE;
    }

    // return true if to continue
    bool HandlePass_()
    {
        if (state_ == State::PLACE) {
            if (last_round_passed_) {
                Global().Boardcast() << At(PlayerID{turn_pid_}) << "也 pass 了一手，双方接连 pass，游戏平局";
                return false;
            } else {
                Global().Boardcast() << At(PlayerID{turn_pid_}) << "pass 了一手";
                last_round_passed_ = true;
            }
        } else if (state_ == State::SWAP_1 || state_ == State::SWAP_2) {
            Global().Boardcast() << At(PlayerID{turn_pid_}) << "决定使用黑棋";
            black_pid_ = turn_pid_;
            state_ = State::PLACE;
        } else if (round_ == 0) {
            assert(state_ == State::INIT);
            Global().Boardcast() << At(PlayerID{turn_pid_}) << "放弃了假先行动，由对手担任假先方";
            black_pid_ = 1 - turn_pid_;
        } else {
            assert(state_ == State::INIT);
            assert(round_ == 1);
            Global().Boardcast() << "双方均放弃假先行动，游戏平局";
            return false;
        }
        return true;
    }

    std::string HtmlHead_() const
    {
        std::string str = "## 第 " + std::to_string(round_ + 1) +  " 回合\n\n";
        html::Table player_table(2, 2);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"10\" ");
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(pid, 0).SetContent(
                        std::string("![](file:///") + Global().ResourceDir() + (black_pid_ == pid ? "c_b.bmp)" : "c_w.bmp)"));
                if (turn_pid_ == pid) {
                    player_table.Get(pid, 1).SetContent(HTML_COLOR_FONT_HEADER(red) " **" + Global().PlayerName(pid) + "** " HTML_FONT_TAIL);
                } else {
                    player_table.Get(pid, 1).SetContent("**" + Global().PlayerName(pid) + "**");
                }
            };
        print_player(0);
        print_player(1);
        str += player_table.ToString();
        str += "\n\n";
        return str;
    }

    int round_;
    std::vector<int64_t> player_scores_;
    Board board_;
    PlayerID turn_pid_;
    PlayerID black_pid_;
    State state_;
    bool last_round_passed_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

