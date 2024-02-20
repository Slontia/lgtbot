// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/game_stage.h"
#include "utility/html.h"

#include "game_util/sync_mahjong.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "同步麻将"; // the game name which should be unique among all the games
const uint64_t k_max_player = 4; // 0 indicates no max-player limits
const uint64_t k_multiple = 0; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "森高";
const std::string k_description = "所有玩家同时摸牌和切牌的麻将游戏";
const std::vector<RuleCommand> k_rule_commands = {};

static constexpr uint32_t const k_image_width = 700;

std::string GameOption::StatusInfo() const
{
    return "";
}

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() < 4) {
        reply() << "该游戏至少 4 人参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 4; }

// ========== GAME STAGES ==========

class TableStage;

class MainStage : public MainGameStage<TableStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , sync_mahjong_option_{
            .tiles_option_ = game_util::mahjong::TilesOption{
                .with_red_dora_ = GET_OPTION_VALUE(option, 赤宝牌),
                .with_toumei_ = GET_OPTION_VALUE(option, 透明牌),
            },
            .image_path_ = option.ResourceDir(),
            .player_descs_ = [&]()
                    {
                        std::vector<game_util::mahjong::PlayerDesc> descs;
                        for (PlayerID pid = 0; pid < option.PlayerNum(); ++pid) {
                            descs.emplace_back(PlayerName(pid), PlayerAvatar(pid, 45), PlayerAvatar(pid, 30),
                                    static_cast<Wind>(pid.Get() + 1), 25000);
                        }
                        return descs;
                    }(),
        }
    {
#ifdef TEST_BOT
        if (!GET_OPTION_VALUE(option, 配牌).empty()) {
            sync_mahjong_option_.tiles_option_ = GET_OPTION_VALUE(option, 配牌);
        }
#endif
    }

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(TableStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return sync_mahjong_option_.player_descs_[pid].base_point_; }

    const game_util::mahjong::SyncMahjongOption& GetSyncMahjongOption() const { return sync_mahjong_option_; }

    int32_t GameNo() const { return game_; }

  private:
    void UpateSeed_()
    {
#ifdef TEST_BOT
        game_util::mahjong::TilesOption *const tiles_option = std::get_if<game_util::mahjong::TilesOption>(&sync_mahjong_option_.tiles_option_);
        if (tiles_option == nullptr) {
            return;
        }
        tiles_option->
#else
        sync_mahjong_option_.tiles_option_.
#endif
        seed_ = GET_OPTION_VALUE(option(), 种子).empty() ? "" : GET_OPTION_VALUE(option(), 种子) + std::to_string(sync_mahjong_option_.benchang_);
    }

    game_util::mahjong::SyncMahjongOption sync_mahjong_option_;
    int32_t game_{0};
};

class SingleTileChecker : public AnyArg
{
   public:
    using AnyArg::AnyArg;

    virtual std::optional<std::string> Check(MsgReader& reader) const override
    {
        if (!reader.HasNext()) {
            return std::nullopt;
        }
        const std::string& tile_str = reader.NextArg();
        if ((tile_str.size() == 2 && std::isdigit(tile_str[0]) && std::isalpha(tile_str[1])) ||
                (tile_str.size() == 3 && tile_str[0] == 't' && std::isdigit(tile_str[1]) && std::isalpha(tile_str[2])))  {
            return tile_str;
        }
        return std::nullopt;
    }
};

static constexpr const char* k_chinese_number[] = {"零", "一", "二", "三", "四", "五", "六", "七", "八", "九", "十"};

class TableStage : public SubGameStage<>
{
  public:
    TableStage(MainStage& main_stage)
        : GameStage(main_stage, std::string("第") + k_chinese_number[main_stage.GameNo() + 1] + "局",
                MakeStageCommand("查看场上情况", &TableStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("摸牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::GetTile_, VoidChecker("摸牌")),
                MakeStageCommand("吃牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Chi_, VoidChecker("吃"), AnyArg("手中的牌（两张）", "24m"), SingleTileChecker("要吃的牌（一张）", "3m")),
                MakeStageCommand("碰牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Pon_, VoidChecker("碰"), AnyArg("要碰的牌（可以指明一张或两张，比如手里有055m，可以指明 05m 或 5m，只有前者会碰掉 0m）", "05m")),
                MakeStageCommand("杠牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kan_, VoidChecker("杠"), SingleTileChecker("要杠的牌（一张）", "4m")),
                MakeStageCommand("九种九牌流局", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Nagashi_, VoidChecker("流局")),
                MakeStageCommand("自摸和牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Tsumo_, VoidChecker("自摸")),
                MakeStageCommand("荣和（副露荣和或者荣和）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Ron_, VoidChecker("荣")),
                MakeStageCommand("切掉自己刚刚摸到的牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::KiriTsumo_, VoidChecker("摸切")),
                MakeStageCommand("结束本回合行动", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Over_, VoidChecker("结束")),
                MakeStageCommand("宣告立直，然后切掉自己刚刚摸到的牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::RichiiKiriTsumo_, VoidChecker("立直"), VoidChecker("摸切")),
                MakeStageCommand("切掉一张牌（优先选择手牌，而不是自己刚刚摸到的牌）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kiri_, SingleTileChecker("要切的牌", "3m")),
                MakeStageCommand("宣告立直，然后切掉一张牌（优先选择手牌，而不是自己刚刚摸到的牌）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::RichiiKiri_, VoidChecker("立直"), SingleTileChecker("要切的牌", "3m")),
                MakeStageCommand("当可以和牌的时候（包括自摸、荣和、鸣牌荣和）自动和牌", &TableStage::SetAutoOption_<game_util::mahjong::AutoOption::AUTO_FU>, VoidChecker("自动和了"), OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")),
                MakeStageCommand("每一巡开始的时候不再询问是否鸣牌，而是自动摸牌", &TableStage::SetAutoOption_<game_util::mahjong::AutoOption::AUTO_GET_TILE>, VoidChecker("自动摸牌"), OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")),
                MakeStageCommand("摸到牌后，如果摸到的这张牌无法形成自摸、补杠或暗杠，则自动切出去", &TableStage::SetAutoOption_<game_util::mahjong::AutoOption::AUTO_KIRI>, VoidChecker("自动摸切"), OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")))
          , table_(main_stage.GetSyncMahjongOption())
    {
    }

    int64_t PlayerPointVariation(const uint32_t player_id) const { return table_.Players()[player_id].PointVariation(); }

    int32_t RemainingRichiiPoints() const { return table_.RichiiPoints(); }

    void UpdatePlayerPublicHtmls_() {
        public_dora_html_ = table_.Players()[0].PublicDoraHtml();
    }

    void AllowPlayersToAct_() {
        for (const auto& player : table_.Players()) {
            if (player.State() == game_util::mahjong::ActionState::ROUND_OVER) {
                continue;
            }
            ClearReady(player.PlayerID());
            auto sender = Tell(player.PlayerID());
            sender << Markdown(PlayerHtml_(player), k_image_width);
            if (player.State() == game_util::mahjong::ActionState::AFTER_GET_TILE) {
                sender << "\n自动为您摸了一张牌\n";
            }
            sender << "\n" << AvailableActions_(player.State());
        }
        StartTimer(GET_OPTION_VALUE(option(), 时限));
    }

    void TellAllPlayersHtml_() {
        for (const auto& player : table_.Players()) {
            Tell(player.PlayerID()) << Markdown(PlayerHtml_(player), k_image_width);
        }
    }

    std::string TitleHtml_() const
    {
        std::string s = "<style>html,body{background:#c3e4f5;}</style>\n\n";
        s += game_util::mahjong::TitleHtml(name(), table_.Round());
        s += "\n\n<center>\n\n**<img src=\"file:///";
        s += option().ResourceDir();
        s += "/riichi.png\"/> ";
        if (table_.RichiiPoints() > 0) {
            s += HTML_COLOR_FONT_HEADER(blue);
        }
        s += HTML_ESCAPE_SPACE;
        s += std::to_string(table_.RichiiPoints() / 1000);
        if (table_.RichiiPoints() > 0) {
            s += HTML_FONT_TAIL;
        }
        s += HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE HTML_ESCAPE_SPACE;
        s += "<img src=\"file:///";
        s += option().ResourceDir();
        s += "/benchang.png\"/> ";
        s += HTML_ESCAPE_SPACE;
        s += std::to_string(main_stage().GetSyncMahjongOption().benchang_);
        s += "**\n\n</center>\n\n";
        return s;
    }

    std::string PlayerHtml_(const game_util::mahjong::SyncMahjongGamePlayer &player) const
    {
        std::string s = TitleHtml_();
        s += player.PublicDoraHtml();
        s += "\n\n";
        s += player.PrivateHtml();
        for (PlayerID other_pid = 0; other_pid < option().PlayerNum(); ++other_pid) {
            s += "\n\n";
            if (player.PlayerID() == other_pid) {
                continue;
            }
            s += table_.Players()[other_pid].PublicHtml();
        }
        return s;
    }

    std::string BoardcastHtml_() const
    {
        std::string s = TitleHtml_();
        s += public_dora_html_;
        s += "\n\n";
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            s += "\n\n";
            s += table_.Players()[pid].PublicHtml();
        }
        return s;
    }

    virtual void OnStageBegin() override
    {
        UpdatePlayerPublicHtmls_();
        AllowPlayersToAct_();
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HookUnreadyPlayers();
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        table_.Players()[pid].PerformAi();
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    bool IsValidGame() const { return is_valid_game_; }

  private:
    // return true if game over
    bool OnOver_()
    {
        const game_util::mahjong::SyncMajong::RoundOverResult result = table_.RoundOver();
        UpdatePlayerPublicHtmls_();
        const char* message = nullptr;
        switch (result) {
            case game_util::mahjong::SyncMajong::RoundOverResult::NORMAL_ROUND:
                Group() << "本巡结果如图所示，请各玩家进行下一巡的行动\n" << Markdown(BoardcastHtml_(), k_image_width);
            case game_util::mahjong::SyncMajong::RoundOverResult::RON_ROUND:
                AllowPlayersToAct_();
                return false;
            case game_util::mahjong::SyncMajong::RoundOverResult::FU:
                message = "有人和牌，本局结束";
                is_valid_game_ = true;
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_三家和了:
                message = "三家和了，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_九种九牌:
                message = "九种九牌，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_四风连打:
                message = "四风连打，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_四家立直:
                message = "四家立直，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::NYANPAI_NAGASHI:
                message = "荒牌流局，本局结束";
                is_valid_game_ = true;
                break;
        }
        Group() << message << "\n" << Markdown(BoardcastHtml_(), k_image_width);
        for (const auto& player : table_.Players()) {
            Tell(player.PlayerID()) << message << "\n" << Markdown(PlayerHtml_(player), k_image_width);
        }
        return true;
    }


    template <typename Func, typename ...Args>
    AtomReqErrCode HandleAction_(const PlayerID pid, MsgSenderBase& reply, const Func func, const Args& ...args) {
        auto& player = table_.Players()[pid];
        if (!(player.*func)(args...)) {
            reply() << "[错误] 行动失败：" << player.ErrorString();
            return StageErrCode::FAILED;
        }
        if (player.State() == game_util::mahjong::ActionState::ROUND_OVER) {
            reply() << "行动成功，您本回合行动已结束";
            return StageErrCode::READY;
        } else {
            StartTimer(GET_OPTION_VALUE(option(), 时限));
        }
        reply() << Markdown(PlayerHtml_(player), k_image_width);
        reply() << "行动成功，" << AvailableActions_(player.State());
        return StageErrCode::OK;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(is_public ? BoardcastHtml_() : PlayerHtml_(table_.Players()[pid]));
        return StageErrCode::OK;
    }

    static const char* const AvailableActions_(const lgtbot::game_util::mahjong::ActionState state)
    {
        return state == lgtbot::game_util::mahjong::ActionState::ROUND_BEGIN ?
               "接下来您可选的操作有（【】中的行为是一定可以执行的）："
               "\n- 【摸牌】"
               "\n- 吃（例如：吃 46m 5m）"
               "\n- 碰（例如：碰 5m）"
               "\n- 杠（仅可直杠，例如：杠 5m）" :
               state == lgtbot::game_util::mahjong::ActionState::AFTER_CHI_PON  ?
               "接下来您可选的操作有（【】中的行为是一定可以执行的）："
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 立直并切牌（例如：立直 摸切）（例如：立直 1s）" :
               state == lgtbot::game_util::mahjong::ActionState::AFTER_GET_TILE ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 立直并切牌（例如：立直 摸切）（例如：立直 1s）"
               "\n- 杠（仅可补杠或暗杠，例如：杠 5m）"
               "\n- 自摸"
               "\n- 流局（宣告九种九牌流局，仅首回合可执行）":
               state == lgtbot::game_util::mahjong::ActionState::AFTER_KAN ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 立直并切牌（例如：立直 摸切）（例如：立直 1s）"
               "\n- 杠（仅可补杠或暗杠，例如：杠 5m）"
               "\n- 自摸" :
               state == lgtbot::game_util::mahjong::ActionState::AFTER_KAN_CAN_NARI ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 杠（仅可补杠或暗杠，例如：杠 5m）"
               "\n- 自摸" :
               state == lgtbot::game_util::mahjong::ActionState::AFTER_KIRI ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【结束】"
               "\n- 吃（例如：吃 46m 5m）"
               "\n- 碰（例如：碰 5m）"
               "\n- 杠（仅可直杠，例如：杠 5m）"
               "\n- 荣（鸣牌荣和）" :
               state == lgtbot::game_util::mahjong::ActionState::NOTIFIED_RON ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【荣】"
               "\n- 【结束】" :
               "您不需要做任何行动";
    }

    AtomReqErrCode GetTile_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::GetTile);
    }

    AtomReqErrCode Chi_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& my_tile_str, const std::string& chi_tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Chi, my_tile_str, chi_tile_str);
    }

    AtomReqErrCode Pon_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Pon, tile_str);
    }

    AtomReqErrCode Kan_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kan, tile_str);
    }

    AtomReqErrCode Nagashi_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Nagashi);
    }

    AtomReqErrCode Tsumo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Tsumo);
    }

    AtomReqErrCode Ron_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Ron);
    }

    AtomReqErrCode RichiiKiriTsumo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, "", true);
    }

    AtomReqErrCode KiriTsumo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, "", false);
    }

    AtomReqErrCode RichiiKiri_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, tile_str, true);
    }

    AtomReqErrCode Kiri_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, tile_str, false);
    }

    AtomReqErrCode Over_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Over);
    }

    template <game_util::mahjong::AutoOption OPTION>
    AtomReqErrCode SetAutoOption_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool enable)
    {
        auto& player = table_.Players()[pid];
        player.SetAutoOption(OPTION, enable);
        static const auto bool_to_str = [](const bool enabled) { return enabled ? "开启" : "关闭"; };
        reply() << "设置成功，当前设置为："
            << "\n自动摸牌 " << bool_to_str(player.GetAutoOption(game_util::mahjong::AutoOption::AUTO_GET_TILE))
            << "\n自动和了 " << bool_to_str(player.GetAutoOption(game_util::mahjong::AutoOption::AUTO_FU))
            << "\n自动摸切 " << bool_to_str(player.GetAutoOption(game_util::mahjong::AutoOption::AUTO_KIRI));
        return StageErrCode::OK;
    }

    lgtbot::game_util::mahjong::SyncMajong table_;
    std::string public_dora_html_;
    bool is_valid_game_{false};
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    UpateSeed_();
    return std::make_unique<TableStage>(*this);
}

MainStage::VariantSubStage MainStage::NextSubStage(TableStage& sub_stage, const CheckoutReason reason)
{
    for (uint32_t pid = 0; pid < sync_mahjong_option_.player_descs_.size(); ++pid) {
        sync_mahjong_option_.player_descs_[pid].base_point_ += sub_stage.PlayerPointVariation(pid);
    }
    if (sub_stage.IsValidGame()) {
        ++game_;
    }
    if (game_ < GET_OPTION_VALUE(option(), 局数)) {
        sync_mahjong_option_.benchang_ += 2;
        sync_mahjong_option_.richii_points_ = sub_stage.RemainingRichiiPoints();
        UpateSeed_();
        return std::make_unique<TableStage>(*this);
    }
    return {};
}

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"

// TEST:
// 只有摸完牌的玩家打出的牌才是河底牌
// 次巡和牌玩家平分前巡立直供托
