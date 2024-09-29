// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

#include "game_util/sync_mahjong.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "同步麻将",
    .developer_ = "森高",
    .description_ = "所有玩家同时摸牌和切牌的麻将游戏",
};
const MutableGenericOptions k_default_generic_options;

const std::vector<RuleCommand> k_rule_commands = {};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [](CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 4;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (const auto player_num = generic_options_readonly.PlayerNum(); player_num < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << player_num;
        return false;
    }
    return true;
}

uint64_t MaxPlayerNum(const CustomOptions& options) { return 4; }

uint32_t Multiple(const CustomOptions& options)
{
    if (!GET_OPTION_VALUE(options, 种子).empty()) {
        return 0;
    }
    return GET_OPTION_VALUE(options, 局数);
}

static constexpr uint32_t const k_image_width = 700;

// ========== GAME STAGES ==========

class TableStage;

static const int k_three_players_init_point = 35000;
static const int k_four_players_init_point = 25000;

class MainStage : public MainGameStage<TableStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , sync_mahjong_option_{
            .tiles_option_ = game_util::mahjong::TilesOption{
                .with_red_dora_ = GAME_OPTION(赤宝牌),
                .with_toumei_ = GAME_OPTION(透明牌),
            },
            .image_path_ = Global().ResourceDir(),
            .player_descs_ = [&]()
                    {
                        std::vector<game_util::mahjong::PlayerDesc> descs;
                        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                            descs.emplace_back(Global().PlayerName(pid), Global().PlayerAvatar(pid, 45), Global().PlayerAvatar(pid, 30),
                                    static_cast<Wind>(pid.Get() + 1), Global().PlayerNum() == 4 ? k_four_players_init_point
                                                                                                : k_three_players_init_point);
                        }
                        return descs;
                    }(),
        }
    {
#ifdef TEST_BOT
        if (!GAME_OPTION(配牌).empty()) {
            sync_mahjong_option_.tiles_option_ = GAME_OPTION(配牌);
        }
#endif
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(TableStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

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
        seed_ = GAME_OPTION(种子).empty() ? "" : GAME_OPTION(种子) + std::to_string(sync_mahjong_option_.benchang_);
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

static constexpr std::array<std::optional<Achievement>, game_util::mahjong::k_max_yaku> YakuToAchievementThreePlayers()
{
    std::array<std::optional<Achievement>, game_util::mahjong::k_max_yaku> result;

    result[static_cast<int>(Yaku::天和)] = Achievement::天和;
    result[static_cast<int>(Yaku::流局满贯)] = Achievement::流局满贯;
    result[static_cast<int>(Yaku::河底捞鱼)] = Achievement::河底捞鱼;
    result[static_cast<int>(Yaku::海底捞月)] = Achievement::海底捞月;
    result[static_cast<int>(Yaku::岭上开花)] = Achievement::岭上开花;
    result[static_cast<int>(Yaku::抢杠)] = Achievement::抢杠;

    return result;
}

static constexpr std::array<std::optional<Achievement>, game_util::mahjong::k_max_yaku> YakuToAchievementFourPlayers()
{
    std::array<std::optional<Achievement>, game_util::mahjong::k_max_yaku> result;

    result[static_cast<int>(Yaku::国士无双)] = Achievement::国士无双;
    result[static_cast<int>(Yaku::国士无双十三面)] = Achievement::国士无双十三面;
    result[static_cast<int>(Yaku::九莲宝灯)] = Achievement::九莲宝灯;
    result[static_cast<int>(Yaku::纯正九莲宝灯)] = Achievement::纯正九莲宝灯;
    result[static_cast<int>(Yaku::字一色)] = Achievement::字一色;
    result[static_cast<int>(Yaku::小四喜)] = Achievement::小四喜;
    result[static_cast<int>(Yaku::大四喜)] = Achievement::大四喜;
    result[static_cast<int>(Yaku::大三元)] = Achievement::大三元;
    result[static_cast<int>(Yaku::清老头)] = Achievement::清老头;
    result[static_cast<int>(Yaku::四暗刻)] = Achievement::四暗刻;
    result[static_cast<int>(Yaku::四暗刻单骑)] = Achievement::四暗刻单骑;
    result[static_cast<int>(Yaku::绿一色)] = Achievement::绿一色;
    result[static_cast<int>(Yaku::天和)] = Achievement::天和;

    result[static_cast<int>(Yaku::役满)] = Achievement::累计役满;

    result[static_cast<int>(Yaku::流局满贯)] = Achievement::流局满贯;

    result[static_cast<int>(Yaku::清一色)] = Achievement::清一色;
    result[static_cast<int>(Yaku::清一色副露版)] = Achievement::清一色;

    result[static_cast<int>(Yaku::二杯口)] = Achievement::二杯口;
    result[static_cast<int>(Yaku::纯全带幺九)] = Achievement::纯全带幺九;
    result[static_cast<int>(Yaku::纯全带幺九副露版)] = Achievement::纯全带幺九;

    result[static_cast<int>(Yaku::七对子)] = Achievement::七对子;
    result[static_cast<int>(Yaku::混全带幺九)] = Achievement::混全带幺九;
    result[static_cast<int>(Yaku::混全带幺九副露版)] = Achievement::混全带幺九;
    result[static_cast<int>(Yaku::三色同刻)] = Achievement::三色同刻;
    result[static_cast<int>(Yaku::三色同顺)] = Achievement::三色同顺;
    result[static_cast<int>(Yaku::三色同顺副露版)] = Achievement::三色同顺;
    result[static_cast<int>(Yaku::三暗刻)] = Achievement::三暗刻;
    result[static_cast<int>(Yaku::一气通贯)] = Achievement::一气通贯;
    result[static_cast<int>(Yaku::一气通贯副露版)] = Achievement::一气通贯;
    result[static_cast<int>(Yaku::混老头)] = Achievement::混老头;
    result[static_cast<int>(Yaku::三杠子)] = Achievement::三杠子;
    result[static_cast<int>(Yaku::小三元)] = Achievement::小三元;
    result[static_cast<int>(Yaku::两立直)] = Achievement::两立直;

    result[static_cast<int>(Yaku::河底捞鱼)] = Achievement::河底捞鱼;
    result[static_cast<int>(Yaku::海底捞月)] = Achievement::海底捞月;
    result[static_cast<int>(Yaku::岭上开花)] = Achievement::岭上开花;
    result[static_cast<int>(Yaku::抢杠)] = Achievement::抢杠;

    return result;
}

class TableStage : public SubGameStage<>
{
  public:
    TableStage(MainStage& main_stage)
        : StageFsm(main_stage, std::string("第") + k_chinese_number[main_stage.GameNo() + 1] + "局",
                MakeStageCommand(*this, "查看场上情况", &TableStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "摸牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::GetTile_, VoidChecker("摸牌")),
                MakeStageCommand(*this, "吃牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Chi_, VoidChecker("吃"), AnyArg("手中的牌（两张）", "24m"), SingleTileChecker("要吃的牌（一张）", "3m")),
                MakeStageCommand(*this, "碰牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Pon_, VoidChecker("碰"), AnyArg("要碰的牌（可以指明一张或两张，比如手里有055m，可以指明 05m 或 5m，只有前者会碰掉 0m）", "05m")),
                MakeStageCommand(*this, "杠牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kan_, VoidChecker("杠"), SingleTileChecker("要杠的牌（一张）", "4m")),
                MakeStageCommand(*this, "拔北（仅限三麻）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kita_, VoidChecker("拔北"), OptionalDefaultChecker<BoolChecker>(false, "优先手牌", "自摸牌")),
                MakeStageCommand(*this, "九种九牌流局", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Nagashi_, VoidChecker("流局")),
                MakeStageCommand(*this, "自摸和牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Tsumo_, VoidChecker("自摸")),
                MakeStageCommand(*this, "荣和（副露荣和或者荣和）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Ron_, VoidChecker("荣")),
                MakeStageCommand(*this, "切掉自己刚刚摸到的牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::KiriTsumo_, VoidChecker("摸切")),
                MakeStageCommand(*this, "结束本回合行动", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Over_, VoidChecker("结束")),
                MakeStageCommand(*this, "宣告立直，然后切掉自己刚刚摸到的牌", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::RichiiKiriTsumo_, VoidChecker("立直"), VoidChecker("摸切")),
                MakeStageCommand(*this, "切掉一张牌（优先选择手牌，而不是自己刚刚摸到的牌）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::Kiri_, SingleTileChecker("要切的牌", "3m")),
                MakeStageCommand(*this, "宣告立直，然后切掉一张牌（优先选择手牌，而不是自己刚刚摸到的牌）", CommandFlag::PRIVATE_ONLY | CommandFlag::UNREADY_ONLY, &TableStage::RichiiKiri_, VoidChecker("立直"), SingleTileChecker("要切的牌", "3m")),
                MakeStageCommand(*this, "当可以和牌的时候（包括自摸、荣和、鸣牌荣和）自动和牌", &TableStage::SetAutoOption_<game_util::mahjong::AutoOption::AUTO_FU>, VoidChecker("自动和了"), OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")),
                MakeStageCommand(*this, "每一巡开始的时候不再询问是否鸣牌，而是自动摸牌", &TableStage::SetAutoOption_<game_util::mahjong::AutoOption::AUTO_GET_TILE>, VoidChecker("自动摸牌"), OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")),
                MakeStageCommand(*this, "摸到牌后，如果摸到的这张牌无法形成自摸、补杠或暗杠，则自动切出去", &TableStage::SetAutoOption_<game_util::mahjong::AutoOption::AUTO_KIRI>, VoidChecker("自动摸切"), OptionalDefaultChecker<BoolChecker>(true, "开启", "关闭")))
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
            Global().ClearReady(player.PlayerID());
            auto sender = Global().Tell(player.PlayerID());
            sender << Markdown(PlayerHtml_(player), k_image_width);
            if (player.State() == game_util::mahjong::ActionState::AFTER_GET_TILE) {
                sender << "\n自动为您摸了一张牌\n";
            }
            sender << "\n" << AvailableActions_(player.State());
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    void TellAllPlayersHtml_() {
        for (const auto& player : table_.Players()) {
            Global().Tell(player.PlayerID()) << Markdown(PlayerHtml_(player), k_image_width);
        }
    }

    std::string TitleHtml_() const
    {
        std::string s = "<style>html,body{background:#c3e4f5;}</style>\n\n";
        s += game_util::mahjong::TitleHtml(Name(), table_.Round());
        s += "\n\n<center>\n\n**<img src=\"file:///";
        s += Global().ResourceDir();
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
        s += Global().ResourceDir();
        s += "/benchang.png\"/> ";
        s += HTML_ESCAPE_SPACE;
        s += std::to_string(Main().GetSyncMahjongOption().benchang_);
        s += "**\n\n</center>\n\n";
        return s;
    }

    std::string PlayerHtml_(const game_util::mahjong::SyncMahjongGamePlayer &player) const
    {
        std::string s = TitleHtml_();
        s += player.PublicDoraHtml();
        s += "\n\n";
        s += player.PrivateHtml();
        for (PlayerID other_pid = 0; other_pid < Global().PlayerNum(); ++other_pid) {
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
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
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
        Global().HookUnreadyPlayers();
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
                Global().Group() << "本巡结果如图所示，请各玩家进行下一巡的行动\n" << Markdown(BoardcastHtml_(), k_image_width);
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
        if (GAME_OPTION(种子).empty()) {
            std::ranges::for_each(table_.Players(), [this, result](const auto& player) { Achieve_(player, result); });
        }
        Global().Group() << message << "\n" << Markdown(BoardcastHtml_(), k_image_width);
        for (const auto& player : table_.Players()) {
            Global().Tell(player.PlayerID()) << message << "\n" << Markdown(PlayerHtml_(player), k_image_width);
        }
        return true;
    }

    void Achieve_(const game_util::mahjong::SyncMahjongGamePlayer& player, const game_util::mahjong::SyncMajong::RoundOverResult result)
    {
        const auto yakus = player.Yakus();
        if (result == game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_四风连打) {
            Global().Achieve(player.PlayerID(), Achievement::四风连打);
        } else if (!yakus.none() && result == game_util::mahjong::SyncMajong::RoundOverResult::CHUTO_NAGASHI_三家和了) {
            Global().Achieve(player.PlayerID(), Achievement::三家和了);
        }
        const auto update_achievement = [this, pid = player.PlayerID()](const int yaku_id) {
            const std::optional<Achievement> achievement = [&]()
                {
                    if (Global().PlayerNum() == 4) {
                        constexpr auto yaku_to_achievement = YakuToAchievementFourPlayers();
                        return yaku_to_achievement[yaku_id];
                    }
                    assert(Global().PlayerNum() == 3);
                    constexpr auto yaku_to_achievement = YakuToAchievementThreePlayers();
                    return yaku_to_achievement[yaku_id];
                }();
            if (achievement.has_value()) {
                Global().Achieve(pid, *achievement);
            }
        };
        for (int i = 0; i < game_util::mahjong::k_max_yaku; ++i) {
            if (yakus[i]) {
                update_achievement(i);
            }
        }
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
            Global().StartTimer(GAME_OPTION(时限));
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
               "\n- 拔北（仅限三麻）"
               "\n- 自摸"
               "\n- 流局（宣告九种九牌流局，仅首回合可执行）":
               state == lgtbot::game_util::mahjong::ActionState::AFTER_KAN ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 立直并切牌（例如：立直 摸切）（例如：立直 1s）"
               "\n- 杠（仅可补杠或暗杠，例如：杠 5m）"
               "\n- 拔北（仅限三麻）"
               "\n- 自摸" :
               state == lgtbot::game_util::mahjong::ActionState::AFTER_KAN_CAN_NARI ?
               "接下来您可以执行的操作有（【】中的行为是一定可以执行的）："
               "\n- 【切牌】（例如：摸切）（例如：1s）"
               "\n- 杠（仅可补杠或暗杠，例如：杠 5m）"
               "\n- 拔北（仅限三麻）"
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

    AtomReqErrCode Kita_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool use_tsumo)
    {
        return HandleAction_(pid, reply, &lgtbot::game_util::mahjong::SyncMahjongGamePlayer::Kita, use_tsumo);
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

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    UpateSeed_();
    setter.Emplace<TableStage>(*this);
    return;
}

void MainStage::NextStageFsm(TableStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    for (uint32_t pid = 0; pid < sync_mahjong_option_.player_descs_.size(); ++pid) {
        sync_mahjong_option_.player_descs_[pid].base_point_ += sub_stage.PlayerPointVariation(pid);
    }
    if (sub_stage.IsValidGame()) {
        ++game_;
    }
    if (game_ < GAME_OPTION(局数)) {
        sync_mahjong_option_.benchang_ += 2;
        sync_mahjong_option_.richii_points_ = sub_stage.RemainingRichiiPoints();
        UpateSeed_();
        setter.Emplace<TableStage>(*this);
        return;
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

