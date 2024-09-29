// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <ranges>
#include <algorithm>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "utility/coding.h"
#include "game_util/chinese_chess.h"

using namespace lgtbot::game_util::chinese_chess;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "群雄象棋",
    .developer_ = "森高",
    .description_ = "多个帝国共同参与，定时重组棋盘的象棋游戏",
};
constexpr uint64_t k_max_player = 6;
uint64_t MaxPlayerNum(const CustomOptions& options) { return k_max_player; }
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 最小回合限制) / 6; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() == 5) {
        reply() << "不好意思，该游戏允许 2、3、4、6 人参加，但唯独不允许 5 人参加";
        return false;
    }
    const auto kingdoms_num = generic_options_readonly.PlayerNum() * GET_OPTION_VALUE(game_options, 阵营);
    auto sender = reply();
    if (kingdoms_num % 2) {
        GET_OPTION_VALUE(game_options, 阵营) = 2;
        sender << "[警告] 当前王国数无法被 2 整除，已将每名玩家控制王国数调整至 2";
    }
    if (kingdoms_num > k_max_player) {
        sender << "每名玩家控制阵营数过多，请保证王国数量小于或等于 6";
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 3;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

static std::ostream& operator<<(std::ostream& os, const Coor& coor) { return os << ('A' + coor.m_) << coor.n_; }

class MainStage : public MainGameStage<>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
                MakeStageCommand(*this, "查看盘面情况，可用于图片重发", &MainStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "查看拼接后的棋盘", &MainStage::ShowImage_,
                    VoidChecker("棋盘"),
                    AlterChecker(KingdomId::ParseMap()),
                    AlterChecker(KingdomId::ParseMap())),
                MakeStageCommand(*this, "跳过某个王国的行动", &MainStage::Pass_,
                    AlterChecker(KingdomId::ParseMap()),
                    VoidChecker("pass")),
                MakeStageCommand(*this, "移动棋子", &MainStage::Move_,
                    ArithChecker<uint32_t>(0, utility.PlayerNum() * GET_OPTION_VALUE(utility.Options(), 阵营), "棋盘编号"),
                    AnyArg("移动前位置", "A1"), AnyArg("移动后位置", "B1")))
        , board_(Global().PlayerNum(), GAME_OPTION(阵营))
        , round_(0)
    {}

    virtual void OnStageBegin()
    {
        board_.SetImagePath(Global().ResourceDir());
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board_.SetPlayerName(pid, Global().PlayerName(pid));
        }
        BoardcastBoardToAllPlayers_();
        ResetTimer_(Global().Boardcast());
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply)
    {
        return StageErrCode::READY;
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return board_.GetScore(pid);
    }

  private:
    std::string RoundTitleHtml_() const { return "## 第 " + std::to_string(round_) + " 回合\n\n"; }

    AtomReqErrCode ShowImage_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const KingdomId kingdom_id_1,
            const KingdomId kingdom_id_2)
    {
        const auto html = RoundTitleHtml_() + board_.ToHtml(kingdom_id_1, kingdom_id_2);
        if (html.empty()) {
            reply() << "查看失败：找不到对应的国家";
            return StageErrCode::FAILED;
        } else {
            reply() << Markdown(html);
            return StageErrCode::OK;
        }
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const KingdomId kingdom_id)
    {
        if (is_public) {
            reply() << "行动失败：请私信裁判行动";
            return StageErrCode::FAILED;
        }
        if (const auto errstr = board_.Pass(pid, kingdom_id); !errstr.empty()) {
            reply() << "行动失败：" << errstr;
            return StageErrCode::FAILED;
        }
        auto sender = reply();
        sender << "跳过成功\n\n";
        return CheckoutErrCode::Condition(TellUnreadyKingdoms_(pid, sender), StageErrCode::READY, StageErrCode::OK);
    }

    AtomReqErrCode Move_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t board_id,
            const std::string& src_coor_str, const std::string& dst_coor_str)
    {
        if (is_public) {
            reply() << "行动失败：请私信裁判行动";
            return StageErrCode::FAILED;
        }
        const auto decode_coor = [&](const auto& coor_str)
            {
                const auto coor_ret = DecodePos(coor_str);
                if (const auto* const s = std::get_if<std::string>(&coor_ret)) {
                    reply() << "行动失败：" << *s;
                    return Coor{-1, -1};
                }
                const auto coor_pair = std::get<std::pair<uint32_t, uint32_t>>(coor_ret);
                return Coor{static_cast<int32_t>(coor_pair.first), static_cast<int32_t>(coor_pair.second)};
            };
        const auto src_coor = decode_coor(src_coor_str);
        const auto dst_coor = decode_coor(dst_coor_str);
        if (src_coor.m_ == -1 || dst_coor.m_ == -1) {
            return StageErrCode::FAILED;
        }
        if (const auto errstr = board_.Move(pid, board_id, src_coor, dst_coor); !errstr.empty()) {
            reply() << "行动失败：" << errstr;
            return StageErrCode::FAILED;
        }
        auto sender = reply();
        sender << "行动成功，您将 " << board_id << " 号棋盘 " << src_coor.ToString() << " 位置的棋子移动至 "
            << dst_coor.ToString() << " 位置\n\n";
        return CheckoutErrCode::Condition(TellUnreadyKingdoms_(pid, sender), StageErrCode::READY, StageErrCode::OK);
    }

    template <typename Sender>
    bool TellUnreadyKingdoms_(const uint32_t pid, Sender&& sender)
    {
        const auto unready_kingdoms = board_.GetUnreadyKingdomIds(pid);
        if (unready_kingdoms.empty()) {
            sender << "您所控制的所有阵营都已经完成行动";
        } else {
            sender << "您还剩余 " << unready_kingdoms.size() << " 个未完成行动的阵营：";
            for (const KingdomId id : unready_kingdoms) {
                sender << id.ToString() << "色帝国 ";
            }
        }
        return unready_kingdoms.empty();
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(cur_html_);
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageOver()
    {
        return CheckoutErrCode::Condition(Settle_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    bool Settle_()
    {
        // do settle and switch
        const auto ret = board_.Settle();
        ++round_;
        if (round_ % GAME_OPTION(切换回合) == 0) {
            Global().Boardcast() << "棋盘发生切换，" << GAME_OPTION(切换回合) << " 回合后再次切换";
            board_.Switch();
        }

        // output html
        BoardcastBoardToAllPlayers_();
        auto sender = Global().Boardcast();

        // fill settle report
        std::string settle_report;
        const auto send_chess_info = [&](const auto& chess)
            {
                sender << "由" << At(PlayerID(board_.GetControllerPlayerID(chess.kingdom_id_))) << "控制的"
                    << chess.kingdom_id_.ToString() << "色帝国的「" << chess.chess_rule_->ChineseName() << "」";
            };
        for (const auto& info : ret.eat_results_) {
            sender << "- ";
            send_chess_info(info.eating_chess_);
            sender << "吃掉了";
            send_chess_info(info.ate_chess_);
            sender << "\n";
        }
        for (const auto& chess : ret.crashed_chesses_) {
            sender << "- ";
            send_chess_info(chess);
            sender << "发生了碰撞\n";
        }

        // check peace round
        bool game_over = false;
        if (!ret.eat_results_.empty() || !ret.crashed_chesses_.empty()) {
            peace_round_count_ = 0;
            sender << "\n";
        } else if (round_ <= GAME_OPTION(最小回合限制)) {
            // do nothing
        } else if (++peace_round_count_ >= GAME_OPTION(和平回合限制)) {
            sender << "已经连续 " << GAME_OPTION(和平回合限制) << " 回合没有发生吃子或碰撞，游戏结束";
            return true;
        } else {
            sender << "[注意] 若 " << (GAME_OPTION(和平回合限制) - peace_round_count_)
                << " 回合结束前仍未发生吃子或碰撞，游戏将结束\n\n";
        }
        if (round_ == GAME_OPTION(最小回合限制)) {
            sender << "[注意] 从下一回合开始，若连续 " << (GAME_OPTION(和平回合限制) - peace_round_count_)
                << " 回合未发生吃子或碰撞，游戏将结束\n\n";
        }

        // reset ready for players
        uint32_t alive_player_count = 0;
        for (uint32_t i = 0; i < Global().PlayerNum(); ++i) {
            if (board_.GetUnreadyKingdomIds(i).empty()) {
                Global().SetReady(i);
            } else {
                ++alive_player_count;
                Global().ClearReady(i);
            }
        }
        if (alive_player_count <= 1) {
            sender << "存活玩家不足一人，游戏结束";
            return true;
        } else {
            ResetTimer_(sender);
            return false;
        }
    }

    void BoardcastBoardToAllPlayers_()
    {
        cur_html_ = RoundTitleHtml_() + board_.ToHtml();
        Global().Group() << Markdown(cur_html_);
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << Markdown(cur_html_);
        }
    }

    template <typename Sender>
    void ResetTimer_(Sender&& sender)
    {
        Global().StartTimer(GAME_OPTION(时限));
        sender << "请所有玩家行动，" << GAME_OPTION(时限)
               << " 秒未行动自动 pass\n格式：棋盘编号 移动前位置 移动后位置\n\n"
               << (GAME_OPTION(切换回合) - round_ % GAME_OPTION(切换回合))
               << " 回合后将切换棋盘";
    }

    CheckoutErrCode OnStageTimeout()
    {
        return CheckoutErrCode::Condition(Settle_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    BoardMgr board_;
    uint32_t round_;
    uint32_t peace_round_count_;
    std::string cur_html_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

