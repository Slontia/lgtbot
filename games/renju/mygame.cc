// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>

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
    .name_ = "决胜五子",
    .developer_ = "森高",
    .description_ = "双方一起落子的五子棋游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options) { return 2; }
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

// Player 1 use fork, player 0 use circle
class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看盘面情况，可用于图片重发", &MainStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "跳过本次落子", &MainStage::Pass_, VoidChecker("pass")),
                MakeStageCommand(*this, "落子", &MainStage::Set_, RepeatableChecker<AnyArg>("坐标")))
        , board_(Global().ResourceDir(), BoardOptions{.to_expand_board_ = true, .is_overline_win_ = true})
        , round_(0)
        , crash_count_(0)
        , extended_(false)
        , last_round_passed_{false, false}
        , last_round_both_passed_(false)
        , last_last_round_both_passed_(false)
        , pass_count_{0, 0}
    {
    }

    virtual void OnStageBegin()
    {
        if (GAME_OPTION(pass胜利)) {
            Global().Boardcast() << "[注意] 本局和棋时 pass 次数较多的玩家取得胜利\n\n但是因为首回合 pass 不计 pass 次数，所以第一手还请正常落子";
        }
        Global().StartTimer(GAME_OPTION(时限));
        Global().Boardcast() << Markdown(HtmlHead_() + board_.ToHtml());
        Global().Boardcast() << "请私信裁判落子位置";
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        uint32_t x, y;
        do {
            x = rand() % Board::k_size_;
            y = rand() % Board::k_size_;
        } while (!board_.CanBeSet(x, y));
        player_pos_[pid].emplace_back(x, y);
        return StageErrCode::READY;
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return winner_ == pid;
    }

  private:
    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "跳过失败：请私信裁判决定跳过";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "跳过失败：您已经完成落子，无法跳过";
            return StageErrCode::FAILED;
        }
        player_pos_[pid].clear();
        reply() << "跳过成功，本回合您不进行落子";
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::vector<std::string>& pos_strs)
    {
        if (is_public) {
            reply() << "落子失败：请私信裁判选择落子位置";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "落子失败：您已经完成落子，无法变更落子位置";
            return StageErrCode::FAILED;
        }
        if (pos_strs.empty()) {
            reply() << "落子失败：请至少选择一个落子点";
            return StageErrCode::FAILED;
        }
        if (!GAME_OPTION(多选点) && pos_strs.size() != 1) {
            reply() << "落子失败：您只能选择一个落子点";
            return StageErrCode::FAILED;
        }
        auto& player_pos = player_pos_[pid];
        for (const auto& str : pos_strs) {
            const auto decode_res = DecodePos<Board::k_size_, Board::k_size_>(str);
            if (const auto* const errstr = std::get_if<std::string>(&decode_res)) {
                player_pos.clear();
                reply() << "落子失败：" << *errstr;
                return StageErrCode::FAILED;
            }
            const auto [x, y] = std::get<std::pair<uint32_t, uint32_t>>(decode_res);
            if (!board_.CanBeSet(x, y)) {
                player_pos.clear();
                reply() << "落子失败：" << str << " 位置无法落子，请试试其它位置吧";
                return StageErrCode::FAILED;
            }
            if (round_ == 0 && x == Board::k_size_ / 2 && y == Board::k_size_ / 2) {
                player_pos.clear();
                reply() << "落子失败：首回合不允许落子天元";
                return StageErrCode::FAILED;
            }
            if (std::ranges::find(player_pos, std::pair{x, y}) != std::end(player_pos)) {
                player_pos.clear();
                reply() << "落子失败：不允许有重复的选点";
                return StageErrCode::FAILED;
            }
            player_pos.emplace_back(x, y);
        }
        reply() << "落子成功";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(HtmlHead_() + board_.ToHtml());
        return StageErrCode::OK;
    }

    std::string HtmlHead_() const
    {
        std::string str = "## 第 " + std::to_string(round_ + 1);
        if (GAME_OPTION(回合上限) > 0) {
            str += " / " + std::to_string(GAME_OPTION(回合上限));
        }
        str +=  " 回合\n\n";
        html::Table player_table(3, 4);
        player_table.SetTableStyle(" align=\"center\" cellpadding=\"0\" cellspacing=\"10\" ");
        player_table.Get(0, 0).SetContent(
                "**扩展后碰撞次数：" + std::string(last_round_crashed_ && extended_ ? HTML_COLOR_FONT_HEADER(red) : "") +
                std::to_string(crash_count_) + std::string(last_round_crashed_ && extended_ ? HTML_FONT_TAIL : "") +
                " / " + std::to_string(GAME_OPTION(碰撞上限)) + "**");
        player_table.MergeRight(0, 0, 4);
        const auto print_player = [&](const PlayerID pid)
            {
                player_table.Get(1, 0 + pid * 2).SetContent(
                        std::string("![](file:///") + Global().ResourceDir() + (pid == 0 ? "c_b.bmp)" : "c_w.bmp)"));
                player_table.MergeDown(1, 0 + pid * 2, 2);
                player_table.Get(1, 1 + pid * 2).SetContent("**" + Global().PlayerName(pid) + "**");
                if (!GAME_OPTION(pass胜利)) {
                    // do nothing
                } else if (last_round_passed_[pid] && !last_last_round_both_passed_) {
                    player_table.Get(2, 1 + pid * 2).SetContent(
                            HTML_COLOR_FONT_HEADER(red) "pass 次数：" + std::to_string(pass_count_[pid]) + HTML_FONT_TAIL);
                } else {
                    player_table.Get(2, 1 + pid * 2).SetContent("pass 次数：" + std::to_string(pass_count_[pid]));
                }
            };
        print_player(0);
        print_player(1);
        str += player_table.ToString();
        str += "\n\n";
        return str;
    }

    virtual CheckoutErrCode OnStageOver()
    {
        if (!SetToBoard_(Global().BoardcastMsgSender())) {
            Global().ClearReady();
            Global().StartTimer(GAME_OPTION(时限));
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    CheckoutErrCode OnStageTimeout()
    {
        Global().HookUnreadyPlayers();
        if (!SetToBoard_(Global().BoardcastMsgSender())) {
            Global().ClearReady();
            Global().StartTimer(GAME_OPTION(时限));
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    Result SetProperPieceToBoard_(MsgSenderBase& reply, const size_t idx = 0)
    {
        const std::array<bool, 2> no_more_choise { player_pos_[0].size() == idx + 1, player_pos_[1].size() == idx + 1 };
        if (player_pos_[0][idx] != player_pos_[1][idx] || (round_ > 0 && no_more_choise[0] && no_more_choise[1])) {
            return board_.Set(player_pos_[0][idx].first, player_pos_[0][idx].second, player_pos_[1][idx].first,
                    player_pos_[1][idx].second);
        } else if (round_ == 0 && no_more_choise[0] && no_more_choise[1]) {
            reply() << "[提示] 已通过中心对称调整某一方落子，防止首回合碰撞";
            return board_.Set(player_pos_[0][idx].first, player_pos_[0][idx].second,
                    Board::k_size_ - 1 - player_pos_[1][idx].first, Board::k_size_ - 1 - player_pos_[1][idx].second);
        } else if (no_more_choise[0]) {
            return board_.Set(player_pos_[0][idx].first, player_pos_[0][idx].second, player_pos_[1][idx + 1].first,
                    player_pos_[1][idx + 1].second);
        } else if (no_more_choise[1]) {
            return board_.Set(player_pos_[0][idx + 1].first, player_pos_[0][idx + 1].second, player_pos_[1][idx].first,
                    player_pos_[1][idx].second);
        } else {
            return SetProperPieceToBoard_(reply, idx + 1);
        }
    }

    bool SetToBoard_(MsgSenderBase& reply)
    {
        board_.ClearHighlight();
        const auto ret =
            player_pos_[0].empty() && player_pos_[1].empty() ?
                Result::CONTINUE_OK :
            player_pos_[1].empty() ?
                board_.Set(player_pos_[0].front().first, player_pos_[0].front().second, Pid2Type_(0)) :
            player_pos_[0].empty() ?
                board_.Set(player_pos_[1].front().first, player_pos_[1].front().second, Pid2Type_(1)) :
            // both not empty
                SetProperPieceToBoard_(reply);
        for (int i = 0; i < 2; ++i) {
            pass_count_[i] += (last_round_passed_[i] = player_pos_[i].empty()) && !last_round_both_passed_ && round_ > 0;
        }
        last_last_round_both_passed_ = last_round_both_passed_;
        last_round_both_passed_ = last_round_passed_[0] && last_round_passed_[1];
        crash_count_ += (last_round_crashed_ = (ret == Result::CONTINUE_CRASH || last_round_both_passed_)) && extended_;
        reply() << Markdown(HtmlHead_() + board_.ToHtml());
        auto sender = reply();
        bool is_over = true;
        if (ret == Result::WIN_BLACK) {
            sender << "黑方连成五子，宣告胜利！";
            winner_ = 0;
        } else if (ret == Result::WIN_WHITE) {
            sender << "白方连成五子，宣告胜利！";
            winner_ = 1;
        } else if (ret == Result::TIE_DOUBLE_WIN) {
            sender << "双方均连成五子，满足了和棋条件";
        } else if (ret == Result::TIE_FULL_BOARD) {
            sender << "棋盘上没有可落子位置了，满足了和棋条件";
        } else if (GAME_OPTION(pass上限) > 0 &&
                pass_count_[0] + pass_count_[1] >= GAME_OPTION(pass上限)) {
            sender << "双方 pass 总次数达到上限，满足了和棋条件";
        } else {
            if (last_round_crashed_) {
                sender << "双方行动相同，发生碰撞";
                if (GAME_OPTION(碰撞上限) > 0 && extended_) {
                    sender << "，目前已发生 " << crash_count_ << " 次碰撞（当发生 " << GAME_OPTION(碰撞上限)
                           << " 次碰撞时，将满足和棋条件）";
                }
                if (last_round_both_passed_ && GAME_OPTION(pass胜利)) {
                    sender << "\n\n下一回合允许继续 pass，但不推荐，理由是下回合的 pass 不会记在个人的 pass 数上";
                }
            } else if (ret == Result::CONTINUE_OK) {
                sender << "双方落子成功";
            } else if (ret == Result::CONTINUE_EXTEND) {
                sender << "双方落子成功，我们的战场得到了扩展！";
                extended_ = true;
            } else {
                abort();
            }
            if (GAME_OPTION(pass上限) > 0 &&
                    (player_pos_[0].empty() || player_pos_[1].empty())) {
                sender << "\n\n本回合有玩家选择了 pass（当双方 pass 次数总和达到 "
                    << GAME_OPTION(pass上限) << " 次时，将满足和棋条件）";
            }
            sender << "\n\n";
            if (++round_ == GAME_OPTION(回合上限)) {
                sender << "达到回合上限，满足了和棋条件";
            } else if (crash_count_ > 0 && crash_count_ == GAME_OPTION(碰撞上限)) {
                sender << "达到碰撞上限，满足了和棋条件";
            } else {
                sender << "游戏继续，请双方继续落子";
                is_over = false;
            }
        }
        if (is_over && !winner_.has_value() && GAME_OPTION(pass胜利)) {
            if (pass_count_[0] > pass_count_[1]) {
                sender << "，但由于黑方 pass 次数多于白方，故黑方取得胜利";
                winner_ = 0;
            } else if (pass_count_[1] > pass_count_[0]) {
                sender << "，但由于白方 pass 次数多于黑方，故白方取得胜利";
                winner_ = 1;
            } else {
                sender << "，由于双方 pass 次数相同，游戏平局";
            }
        }
        player_pos_[0].clear();
        player_pos_[1].clear();
        return is_over;
    }

    static AreaType Pid2Type_(const PlayerID pid) { return pid == 0 ? AreaType::BLACK : AreaType::WHITE; }

    Board board_;
    uint32_t round_;
    uint32_t crash_count_;
    bool extended_;
    std::array<std::vector<std::pair<uint32_t, uint32_t>>, 2> player_pos_;
    std::array<bool, 2> last_round_passed_;
    bool last_round_both_passed_;
    bool last_last_round_both_passed_;
    bool last_round_crashed_;
    std::array<int32_t, 2> pass_count_;
    std::optional<PlayerID> winner_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

