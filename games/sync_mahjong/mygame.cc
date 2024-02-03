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
        : GameStage(option, match, MakeStageCommand("查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , game_(0)
        , option_{
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
            //std::cout << "配牌xxxxxxxxxxxxxxx " << GET_OPTION_VALUE(option, 配牌) << std::endl;
            option_.tiles_option_ = GET_OPTION_VALUE(option, 配牌);
        }
#endif
    }

    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(TableStage& sub_stage, const CheckoutReason reason) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return option_.player_descs_[pid].base_point_; }

    const game_util::mahjong::SyncMahjongOption& GetSyncMahjongOption() const { return option_; }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << "这里输出当前游戏情况";
        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

    void UpateSeed_()
    {
#ifdef TEST_BOT
        game_util::mahjong::TilesOption *const tiles_option = std::get_if<game_util::mahjong::TilesOption>(&option_.tiles_option_);
        if (tiles_option == nullptr) {
            return;
        }
        tiles_option->
#else
        option_.tiles_option_.
#endif
        seed_ = GET_OPTION_VALUE(option(), 种子).empty() ? "" : GET_OPTION_VALUE(option(), 种子) + std::to_string(game_);
    }

    int game_;
    game_util::mahjong::SyncMahjongOption option_;
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

class TableStage : public SubGameStage<>
{
  public:
    TableStage(MainStage& main_stage, const uint64_t benchang)
        : GameStage(main_stage, std::to_string(benchang) + " 本场",
                MakeStageCommand("查看场上情况", &TableStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("摸牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::GetTile_, VoidChecker("摸牌")),
                MakeStageCommand("吃牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Chi_, VoidChecker("吃"), AnyArg("手中的牌（两张）", "24m"), SingleTileChecker("要吃的牌（一张）", "3m")),
                MakeStageCommand("碰牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Pon_, VoidChecker("碰"), AnyArg("要碰的牌（可以指明一张或两张，比如手里有055m，可以指明 05m 或 5m，只有前者会碰掉 0m）", "05m")),
                MakeStageCommand("杠牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kan_, VoidChecker("杠"), SingleTileChecker("要杠的牌（一张）", "4m")),
                MakeStageCommand("九种九牌流局", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Nagashi_, VoidChecker("流局")),
                MakeStageCommand("自摸和牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Tsumo_, VoidChecker("自摸")),
                MakeStageCommand("荣和（副露荣和或者荣和）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Ron_, VoidChecker("荣")),
                MakeStageCommand("切掉自己刚刚摸到的牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::KiriTsumo_, VoidChecker("摸切")),
                MakeStageCommand("鸣牌并切牌后结束本回合行动", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Over_, VoidChecker("结束")),
                MakeStageCommand("宣告立直，然后切掉自己刚刚摸到的牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::RichiiKiriTsumo_, VoidChecker("立直"), VoidChecker("摸切")),
                MakeStageCommand("切掉一张牌（优先选择手牌，而不是自己刚刚摸到的牌）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kiri_, SingleTileChecker("要切的牌", "3m")),
                MakeStageCommand("宣告立直，然后切掉一张牌（优先选择手牌，而不是自己刚刚摸到的牌）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::RichiiKiri_, VoidChecker("立直"), SingleTileChecker("要切的牌", "3m")))
          , table_(main_stage.GetSyncMahjongOption())
          , player_public_htmls_(main_stage.GetSyncMahjongOption().player_descs_.size())
    {
    }

    int64_t PlayerPointVariation(const uint32_t player_id) const { return table_.players_[player_id].point_variation_; }

    void UpdatePlayerPublicHtmls_() {
        for (const auto& player : table_.players_) {
            player_public_htmls_[player.PlayerID()] = player.Html(game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PUBLIC, table_.Doras());
        }
    }

    void AllowPlayersToAct_() {
        for (const auto& player : table_.players_) {
            if (player.State() == game_util::mahjong::ActionState::ROUND_OVER) {
                continue;
            }
            ClearReady(player.PlayerID());
            Tell(player.PlayerID()) << Markdown(PlayerHtml_(player), k_image_width);
            Tell(player.PlayerID()) << AvailableActions_(player.State());
        }
    }

    void TellAllPlayersHtml_() {
        for (const auto& player : table_.players_) {
            Tell(player.PlayerID()) << Markdown(PlayerHtml_(player), k_image_width);
        }
    }

    std::string TitleHtml_() const
    {
        return "<style>html,body{background:#c3e4f5;}</style>\n\n" + game_util::mahjong::TitleHtml(name(), table_.Round());
    }

    std::string PlayerHtml_(const game_util::mahjong::SyncMahjongGamePlayer &player) const
    {
        std::string s = TitleHtml_();
        s += "\n\n";
        s += player.Html(game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PRIVATE, table_.Doras());
        for (PlayerID other_pid = 0; other_pid < option().PlayerNum(); ++other_pid) {
            s += "\n\n";
            if (player.PlayerID() == other_pid) {
                continue;
            }
            s += player_public_htmls_[other_pid];
        }
        return s;
    }

    std::string BoardcastHtml_() const
    {
        std::string s = TitleHtml_();
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            s += "\n\n";
            s += player_public_htmls_[pid];
        }
        return s;
    }

    virtual void OnStageBegin() override
    {
        StartTimer(GET_OPTION_VALUE(option(), 时限));
        Boardcast() << "牌局开始"; // TODO: show image
        UpdatePlayerPublicHtmls_();
        AllowPlayersToAct_();
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        HookUnreadyPlayers();
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        // TODO: make bot clever
        table_.players_[pid].PerformDefault(table_.Doras());
        return StageErrCode::READY;
    }

    virtual void OnAllPlayerReady() override
    {
        Boardcast() << "全员行动完毕，公布当前赛况";
        OnOver_();
    }

  private:
    // return true if game over
    bool OnOver_()
    {
        const game_util::mahjong::SyncMajong::RoundOverResult result = table_.RoundOver();
        UpdatePlayerPublicHtmls_();
        switch (result) {
            case game_util::mahjong::SyncMajong::RoundOverResult::NORMAL_ROUND:
                Boardcast() << "本巡结果如图所示，请各玩家进行下一巡的行动" << Markdown(BoardcastHtml_(), k_image_width);
            case game_util::mahjong::SyncMajong::RoundOverResult::RON_ROUND:
                AllowPlayersToAct_();
                return false;
            case game_util::mahjong::SyncMajong::RoundOverResult::FU:
                Boardcast() << "有人和牌，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_三家和了:
                Boardcast() << "三家和了，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_九种九牌:
                Boardcast() << "九种九牌，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_四风连打:
                Boardcast() << "四风连打，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_四家立直:
                Boardcast() << "四家立直，中途流局，本局结束";
                break;
            case game_util::mahjong::SyncMajong::RoundOverResult::NYANPAI_NAGASHI:
                Boardcast() << "荒牌流局，本局结束";
                break;
        }
        return true;
    }


    template <typename Func, typename ...Args>
    AtomReqErrCode HandleAction_(const PlayerID pid, MsgSenderBase& reply, const Func func, const Args& ...args) {
        auto& player = table_.players_[pid];
        if (!(player.*func)(args...)) {
            reply() << "[错误] 行动失败：" << player.ErrorString();
            return StageErrCode::FAILED;
        }
        if (player.State() == game_util::mahjong::ActionState::ROUND_OVER) {
            reply() << "行动成功，您本回合行动已结束";
            return StageErrCode::READY;
        }
        reply() << Markdown(PlayerHtml_(player), k_image_width);
        reply() << "行动成功，" << AvailableActions_(player.State());
        return StageErrCode::OK;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
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
               "您本回合行动已结束";
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
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Tsumo, table_.Doras());
    }

    AtomReqErrCode Ron_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Ron, table_.Doras());
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

    lgtbot::game_util::mahjong::SyncMajong table_;
    std::vector<std::string> player_public_htmls_;
};

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    UpateSeed_();
    return std::make_unique<TableStage>(*this, game_);
}

MainStage::VariantSubStage MainStage::NextSubStage(TableStage& sub_stage, const CheckoutReason reason)
{
    ++game_;
    for (uint32_t pid = 0; pid < option_.player_descs_.size(); ++pid) {
        option_.player_descs_[pid].base_point_ += sub_stage.PlayerPointVariation(pid);
    }
    if (game_ < GET_OPTION_VALUE(option(), 局数)) {
        UpateSeed_();
        option_.benchang_ += 2;
        return std::make_unique<TableStage>(*this, game_ * 2);
    }
    Boardcast() << "游戏结束";
    // Returning empty variant means the game will be over.
    return {};
}

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"

// TEST:
// 只有摸完牌的玩家打出的牌才是河底牌
// 次巡和牌玩家平分前巡立直供托
