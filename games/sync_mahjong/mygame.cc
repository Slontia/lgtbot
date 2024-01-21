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
          , latest_player_points_(main_stage.GetSyncMahjongOption().player_descs_.size())
          , player_public_htmls_(main_stage.GetSyncMahjongOption().player_descs_.size())
          , round_(1)
          , benchang_(benchang)
    {
        for (PlayerID pid = 0; pid < latest_player_points_.size(); ++pid) {
            latest_player_points_[pid] = main_stage.GetSyncMahjongOption().player_descs_[pid].base_point_;
        }
    }

    int64_t LatestPlayerPoint(const uint32_t player_id) const { return latest_player_points_[player_id]; }

    void TellEachPlayerHtml_() {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            Tell(pid) << Markdown(PlayerHtml_(table_.players_[pid]), k_image_width);
            Tell(pid) << AvailableActions_(table_.players_[pid].State());
        }
        Boardcast() << Markdown(BoardcastHtml_(), k_image_width);
    }

    std::string TitleHtml_() const
    {
        return "<style>html,body{background:#c3e4f5;}</style>\n\n" + game_util::mahjong::TitleHtml(name(), round_);
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
        for (auto& player : table_.players_) {
            player.StartRound({}, round_);
            player_public_htmls_[player.PlayerID()] = player.Html(game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PUBLIC, table_.Doras());
        }
        TellEachPlayerHtml_();
    }

    virtual CheckoutErrCode OnTimeout() override
    {
        HookUnreadyPlayers();
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        // TODO: make bot clever
        table_.players_[pid].PerformDefault();
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
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                table_.players_[pid].PerformDefault();
            }
        }
        if (const char* const nagashi_type = CheckToujyuNagashi(table_.players_, round_ == 1)) {
            std::string s = TitleHtml_();
            for (const auto& player : table_.players_) {
                s += "\n\n";
                if (player.State() == game_util::mahjong::ActionState::AFTER_GET_TILE_CAN_NAGASHI) {
                    s += player.Html(game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::OPEN, table_.Doras());
                    s += "\n\n<center> <font size=\"6\">\n\n " HTML_COLOR_FONT_HEADER(green)
                        " **九&nbsp;&nbsp;种&nbsp;&nbsp;九&nbsp;&nbsp;牌** " HTML_FONT_TAIL "\n\n</font> </center>\n\n";
                } else {
                    s += player.Html(game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PUBLIC, table_.Doras());
                }
            }
            Boardcast() << nagashi_type << "，中途流局，本局结束";
            Boardcast() << Markdown(s, k_image_width);
            return true;
        }
        if (HandleFuResults_()) {
            std::string s = TitleHtml_();
            for (const auto& player : table_.players_) {
                const game_util::mahjong::SyncMahjongGamePlayer::HtmlMode html_mode =
                    !player.FuResults().empty() ?
                    game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::OPEN :
                    game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PUBLIC;
                s += "\n\n";
                s += player.Html(html_mode, table_.Doras());
            }
            Boardcast() << "有人和牌，本局结束";
            Boardcast() << Markdown(s, k_image_width);
            return true;
        }
        if (ron_stage_ && CheckNyanapaiNagashi(table_.players_)) {
            // TODO: show image
            std::string s = TitleHtml_();
            for (const auto& player : table_.players_) {
                const game_util::mahjong::SyncMahjongGamePlayer::HtmlMode html_mode =
                    !player.GetListenTiles().empty() ?
                    game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::OPEN :
                    game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PUBLIC;
                s += "\n\n";
                s += player.Html(html_mode, table_.Doras());
            }
            // TODO: 流局满贯
            // TODO: 罚符
            Boardcast() << "荒牌流局，本局结束";
            Boardcast() << Markdown(s, k_image_width);
            return true;
        }
        ron_stage_ = !ron_stage_;
        return ron_stage_ ? RonStageBegin_() : NormalStageBegin_();
    }

    bool NormalStageBegin_()
    {
        Boardcast() << "全员行动完毕，下面公布结果";
        // TODO: show image
        ClearReady();
        std::vector<game_util::mahjong::PlayerKiriInfo> kiri_infos;
        kiri_infos.reserve(table_.players_.size());
        for (const auto& player : table_.players_) {
            kiri_infos.emplace_back(player.CurrentRoundKiriInfo());
        }
        ++round_;
        for (auto& player : table_.players_) {
            if (player.State() == game_util::mahjong::ActionState::RICHII_PREPARE) {
                latest_player_points_[player.PlayerID()] -= 1000;
            }
            player.StartRound(kiri_infos, round_);
            player_public_htmls_[player.PlayerID()] = player.Html(game_util::mahjong::SyncMahjongGamePlayer::HtmlMode::PUBLIC, table_.Doras());
        }
        TellEachPlayerHtml_();
        StartTimer(GET_OPTION_VALUE(option(), 时限));
        return false;
    }

    bool RonStageBegin_()
    {
        bool found = false;
        for (const auto& player : table_.players_) {
            if (player.CanRon()) {
                found = true;
                ClearReady(player.PlayerID());
                Tell(player.PlayerID()) << "您现在可以选择和牌\n" << Markdown(PlayerHtml_(player), k_image_width);
            }
        }
        if (found) {
            StartTimer(GET_OPTION_VALUE(option(), 时限));
            return false;
        }
        if (CheckNyanapaiNagashi(table_.players_)) {
            // TODO: show image
            // TODO: 流局满贯
            // TODO: 罚符
            Boardcast() << "荒牌流局，本局结束";
            return true;
        }
        ron_stage_ = false;
        return NormalStageBegin_();
    }

    void HandleOneFuResult_(const lgtbot::game_util::mahjong::FuResult& fu_result, const uint32_t player_id, const size_t fu_results_size)
    {
        if (fu_result.player_id_ == game_util::mahjong::FuResult::k_tsumo_player_id_) {
            // tsumo
            const int score_each_player = fu_result.counter_.score1 + benchang_ * 100;
            std::ranges::for_each(latest_player_points_, [score_each_player](auto& point) { point -= score_each_player; });
            latest_player_points_[player_id] += score_each_player * table_.players_.size();
        } else {
            // ron
            const int score = fu_result.counter_.score1 + benchang_ * (table_.players_.size() - 1) * 100 / fu_results_size;
            latest_player_points_[fu_result.player_id_] -= score;
            latest_player_points_[player_id] += score;
        }
    }

    bool HandleFuResults_()
    {
        bool found_fu = false;
        for (const auto& player : table_.players_) {
            for (const auto& fu_result : player.FuResults()) {
                found_fu = true;
                HandleOneFuResult_(fu_result, player.PlayerID(), player.FuResults().size());
            }
        }
        return found_fu;
    }

    template <typename Func, typename ...Args>
    AtomReqErrCode HandleAction_(const PlayerID pid, MsgSenderBase& reply, const Func func, const bool ready_if_succ,
            const Args& ...args) {
        auto& player = table_.players_[pid];
        if (!(player.*func)(args...)) {
            reply() << "[错误] 行动失败：" << player.ErrorString();
            return StageErrCode::FAILED;
        }
        if (ready_if_succ) {
            reply() << "行动成功，您本回合行动已结束";
        } else {
            reply() << Markdown(PlayerHtml_(player), k_image_width);
            reply() << "行动成功，" << AvailableActions_(player.State());
        }
        return AtomReqErrCode::Condition(ready_if_succ, StageErrCode::READY, StageErrCode::OK);
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
               state == lgtbot::game_util::mahjong::ActionState::AFTER_GET_TILE ||
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
               state == lgtbot::game_util::mahjong::ActionState::AFTER_GET_TILE_CAN_NAGASHI ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【流局】"
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 立直并切牌（例如：立直 摸切）（例如：立直 1s）"
               "\n- 杠（仅可补杠或暗杠，例如：杠 5m）"
               "\n- 自摸" :
               state == lgtbot::game_util::mahjong::ActionState::AFTER_KIRI ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【结束】"
               "\n- 吃（例如：吃 46m 5m）"
               "\n- 碰（例如：碰 5m）"
               "\n- 杠（仅可直杠，例如：杠 5m）"
               "\n- 荣（鸣牌荣和）" :
               /*
               state == lgtbot::game_util::mahjong::ActionState::RICHII_FIRST_ROUND ||
               state == lgtbot::game_util::mahjong::ActionState::RICHII ?
               "接下来您可以执行的操作有："
               "\n- 自摸"
               "\n- 摸切" :
               state == lgtbot::game_util::mahjong::ActionState::ROUND_OVER ||
               state == lgtbot::game_util::mahjong::ActionState::RICHII_PREPARE ?
               "接下来您可以执行的操作有："
               "\n- 荣"
               "\n- 结束" :
               */
               "您本回合行动已结束";
    }

    AtomReqErrCode GetTile_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::GetTile, false);
    }

    AtomReqErrCode Chi_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& my_tile_str, const std::string& chi_tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Chi, false, my_tile_str, chi_tile_str);
    }

    AtomReqErrCode Pon_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Pon, false, tile_str);
    }

    AtomReqErrCode Kan_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kan, false, tile_str);
    }

    AtomReqErrCode Nagashi_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Nagashi, true);
    }

    AtomReqErrCode Tsumo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Tsumo, true, table_.Doras());
    }

    AtomReqErrCode Ron_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Ron, true, table_.Doras());
    }

    AtomReqErrCode RichiiKiriTsumo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, true, "", true);
    }

    AtomReqErrCode KiriTsumo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (AtomReqErrCode::FAILED == HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, false, "", false)) {
            return StageErrCode::FAILED;
        }
        if (table_.players_[pid].State() == game_util::mahjong::ActionState::AFTER_KIRI) {
            return StageErrCode::OK;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode RichiiKiri_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, true, tile_str, true);
    }

    AtomReqErrCode Kiri_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& tile_str)
    {
        if (AtomReqErrCode::FAILED == HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kiri, false, tile_str, false)) {
            return StageErrCode::FAILED;
        }
        if (table_.players_[pid].State() == game_util::mahjong::ActionState::AFTER_KIRI) {
            return StageErrCode::OK;
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Over_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Over, true);
    }

    lgtbot::game_util::mahjong::SyncMajong table_;
    std::vector<int64_t> latest_player_points_;
    std::vector<std::string> player_public_htmls_;
    int32_t round_{0};
    bool ron_stage_{false};
    int32_t benchang_{0};
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
        option_.player_descs_[pid].base_point_ = sub_stage.LatestPlayerPoint(pid);
    }
    if (game_ < GET_OPTION_VALUE(option(), 局数)) {
        UpateSeed_();
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
