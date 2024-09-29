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
#include "game_util/mahjong_17_steps.h"

using namespace lgtbot::game_util::mahjong;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "十七步",
    .developer_ = "森高",
    .description_ = "组成满贯听牌牌型，并经历 17 轮切牌的麻将游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 4; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options)
{
    return GET_OPTION_VALUE(options, 种子).empty() ? 3 : 0;
}
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() == 1) {
        reply() << "该游戏至少 2 名玩家参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    if (generic_options_readonly.PlayerNum() == 4 && GET_OPTION_VALUE(game_options, 宝牌) > 0) {
        GET_OPTION_VALUE(game_options, 宝牌) = 0;
        reply() << "[警告] 四人游戏不允许有宝牌，已经将宝牌数量自动调整为 0";
        return true;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [](CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 3;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

static constexpr const std::array<Wind, 4> k_winds = { Wind::East, Wind::West, Wind::South, Wind::North };

class TableStage;

class MainStage : public MainGameStage<TableStage>
{
  private:
    struct Player
    {
        Player(const uint32_t wind_idx)
            : wind_idx_(wind_idx)
            , score_(25000)
        {}

        uint32_t wind_idx_;
        int32_t score_;
    };

  public:
    MainStage(StageUtility&& utility) : StageFsm(std::move(utility)), table_idx_(0)
    {
        std::variant<std::random_device, std::seed_seq> rd;
        std::mt19937 g([&]
            {
                if (GAME_OPTION(种子).empty()) {
                    auto& real_rd = rd.emplace<std::random_device>();
                    return std::mt19937(real_rd());
                } else {
                    auto& real_rd = rd.emplace<std::seed_seq>(GAME_OPTION(种子).begin(), GAME_OPTION(种子).end());
                    return std::mt19937(real_rd);
                }
            }());
        const auto offset = std::uniform_int_distribution<uint32_t>(1, Global().PlayerNum())(g);
        for (uint64_t pid = 0; pid < Global().PlayerNum(); ++pid) {
            players_.emplace_back((pid + offset) % Global().PlayerNum());
        }
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<TableStage>(*this, "东一局");
    }

    virtual void NextStageFsm(TableStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        const uint32_t max_round = 4;
        ++table_idx_;
        if (table_idx_ < max_round) {
            for (auto& player : players_) {
                player.wind_idx_ = (player.wind_idx_ + 1) % max_round % 4;
            }
            setter.Emplace<TableStage>(*this, table_idx_ == 1 ? "东二局" : table_idx_ == 2 ? "东三局" : "东四局");
            return;
        }
    }

    int64_t PlayerScore(const PlayerID pid) const
    {
        return players_[pid].score_;
    }

    std::vector<Player> players_;

  private:
    uint32_t table_idx_;
};

class PrepareStage : public SubGameStage<>
{
   public:
    PrepareStage(MainStage& main_stage, Mahjong17Steps& game_table)
            : StageFsm(main_stage, "配牌阶段",
                    MakeStageCommand(*this, "从牌山中添加手牌", &PrepareStage::Add_,
                        VoidChecker("添加"), AnyArg("要添加的牌（无空格）", "123p445566m789s2z")),
                    MakeStageCommand(*this, "将手牌放回牌山", &PrepareStage::Remove_,
                        VoidChecker("移除"), AnyArg("要移除的牌（无空格）", "1p44m")),
                    MakeStageCommand(*this, "完成配牌，宣布立直", &PrepareStage::Finish_, VoidChecker("立直")),
                    MakeStageCommand(*this, "查看当前手牌配置情况", &PrepareStage::Info_, VoidChecker("赛况"),
                        OptionalDefaultChecker<BoolChecker>(true, "图片", "文字")))
            , game_table_(game_table)
    {}

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(配牌时限));
        Global().Boardcast() << "请私信裁判进行配牌，时限 " << GAME_OPTION(配牌时限) << " 秒";
        Global().SaveMarkdown(game_table_.SpecHtml());
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << Markdown(game_table_.PrepareHtml(pid));
            Global().Tell(pid) << "请配牌，您可通过「帮助」命令查看命令格式";
        }
    }

   private:
    // PrepareStage timeout do not hook players

    AtomReqErrCode Add_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (is_public) {
            reply() << "[错误] 添加失败，请私信裁判进行添加";
            return StageErrCode::FAILED;
        }
        if (!game_table_.AddToHand(pid, str)) {
            reply() << "[错误] 添加失败：" << game_table_.ErrorStr();
            return StageErrCode::FAILED;
        }
        reply() << Markdown(game_table_.PrepareHtml(pid));
        reply() << "添加成功！若您已凑齐所有 13 张手牌，请使用「立直」命令结束行动";
        return StageErrCode::OK;
    }

    AtomReqErrCode Remove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (is_public) {
            reply() << "[错误] 移除失败，请私信裁判进行移除";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 喂，立都立了还带换牌的啊！";
            return StageErrCode::FAILED;
        }
        if (!game_table_.RemoveFromHand(pid, str)) {
            reply() << "[错误] 移除失败：" << game_table_.ErrorStr();
            return StageErrCode::FAILED;
        }
        reply() << "移除成功！";
        reply() << Markdown(game_table_.PrepareHtml(pid));
        return StageErrCode::OK;
    }

    AtomReqErrCode Finish_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经立直过一次了，即便再立直一次，番数也不会增加哦~";
            return StageErrCode::FAILED;
        }
        if (!game_table_.CheckHandValid(pid)) {
            reply() << "[错误] 您未完成配牌，无法立直";
            return StageErrCode::FAILED;
        }
        reply() << "宣布立直成功！";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool show_image)
    {
        if (is_public) {
            reply() << "请私信裁判查看手牌和牌山情况";
            return StageErrCode::FAILED;
        }
        if (show_image) {
            reply() << Markdown(game_table_.PrepareHtml(pid));
        } else {
            reply() << game_table_.PrepareString(pid);
        }
        return StageErrCode::OK;
    }

    Mahjong17Steps& game_table_;
};

class KiriStage : public SubGameStage<>
{
   public:
    KiriStage(MainStage& main_stage, Mahjong17Steps& game_table)
            : StageFsm(main_stage, "切牌阶段",
                    MakeStageCommand(*this, "查看各个玩家舍牌情况", &KiriStage::Info_, VoidChecker("赛况"),
                        OptionalDefaultChecker<BoolChecker>(true, "图片", "文字")),
                    MakeStageCommand(*this, "从牌山中切出该牌", &KiriStage::Kiri_, AnyArg("舍牌", "2s")))
            , game_table_(game_table)
    {}

    virtual void OnStageBegin() override
    {
        Global().StartTimer(GAME_OPTION(切牌时限));
        SendInfo_();
        Global().Boardcast() << "请从牌山中选择一张切出去，时限 " << GAME_OPTION(切牌时限) << " 秒";
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << "请从牌山中选择一张切出去，时限 " << GAME_OPTION(切牌时限) << " 秒";
        }
    }

   private:
    CheckoutErrCode OnStageTimeout()
    {
        Global().HookUnreadyPlayers();
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    AtomReqErrCode Kiri_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经切过牌了";
            return StageErrCode::FAILED;
        }
        if (!game_table_.Kiri(pid, str)) {
            reply() << "[错误] 切牌失败：" << game_table_.ErrorStr();
            return StageErrCode::FAILED;
        }
        reply() << "切出 " << str << " 成功";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool show_image)
    {
        if (is_public && show_image) {
            reply() << Markdown(game_table_.PublicHtml());
        } else if (!is_public && show_image) {
            reply() << Markdown(game_table_.KiriHtml(pid));
        } else if (is_public && !show_image) {
            reply() << game_table_.PublicString();
        } else {
            reply() << game_table_.KiriString(pid);
        }
        return StageErrCode::OK;
    }

    void SendInfo_()
    {
        Global().Group() << Markdown(game_table_.PublicHtml());
        Global().SaveMarkdown(game_table_.SpecHtml());
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << Markdown(game_table_.KiriHtml(pid));
        }
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << "全员切牌完毕，公布当前赛况";
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    Mahjong17Steps& game_table_;

  private:
    void Achieve_(const PlayerID pid)
    {
        if (!GAME_OPTION(种子).empty()) {
            return; // no achievements for game with a seed
        }
        for (const Yaku yaku : game_table_.GetYakumanYakus(pid)) {
            if (yaku == Yaku::国士无双) {
                Global().Achieve(pid, Achievement::国士无双);
            } else if (yaku == Yaku::九莲宝灯) {
                Global().Achieve(pid, Achievement::九莲宝灯);
            } else if (yaku == Yaku::字一色) {
                Global().Achieve(pid, Achievement::字一色);
            } else if (yaku == Yaku::小四喜) {
                Global().Achieve(pid, Achievement::小四喜);
            } else if (yaku == Yaku::大四喜) {
                Global().Achieve(pid, Achievement::大四喜);
            } else if (yaku == Yaku::大三元) {
                Global().Achieve(pid, Achievement::大三元);
            } else if (yaku == Yaku::清老头) {
                Global().Achieve(pid, Achievement::清老头);
            } else if (yaku == Yaku::四暗刻) {
                Global().Achieve(pid, Achievement::四暗刻);
            } else if (yaku == Yaku::绿一色) {
                Global().Achieve(pid, Achievement::绿一色);
            } else if (yaku == Yaku::役满) {
                Global().Achieve(pid, Achievement::累计役满);
            }
        }

    }

    bool OnOver_()
    {
        const auto state = game_table_.RoundOver();
        SendInfo_();
        switch (state) {
        case Mahjong17Steps::GameState::CONTINUE:
            Global().Boardcast() << "全员安全，请继续私信裁判切牌，时限 " << GAME_OPTION(切牌时限) << " 秒";
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                Global().Tell(pid) << "请继续切牌，时限 " << GAME_OPTION(切牌时限) << " 秒";
            }
            Global().ClearReady();
            Global().StartTimer(GAME_OPTION(切牌时限));
            return false;
        case Mahjong17Steps::GameState::HAS_RON: {
            // FIXME: if timeout, the ready set any_ready is not be set, then the round will not be over
            std::vector<std::pair<PlayerID, int32_t>> player_scores;
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                Main().players_[pid].score_ += game_table_.PointChange(pid);
                player_scores.emplace_back(pid, Main().players_[pid].score_);
                Achieve_(pid);
            }
            std::ranges::sort(player_scores, [](const auto& _1, const auto& _2) { return _1.second > _2.second; });
            auto sender = Global().Boardcast();
            sender << "有玩家和牌，本局结束，当前各个玩家分数：";
            for (uint32_t i = 0; i < player_scores.size(); ++i) {
                sender << "\n" << (i + 1) << " 位：" << At(player_scores[i].first) << "【" << player_scores[i].second << " 分】";
            }
            return true;
        }
        case Mahjong17Steps::GameState::FLOW:
            Global().Boardcast() << "十七巡结束，流局";
            return true;
        }
        return true;
    }
};

class TableStage : public SubGameStage<PrepareStage, KiriStage>
{
  public:
    TableStage(MainStage& main_stage, const std::string& stage_name)
        : StageFsm(main_stage, stage_name)
        , game_table_(Mahjong17Steps(Mahjong17StepsOption{
                    .tile_option_{
                        .with_red_dora_ = GAME_OPTION(赤宝牌),
                        .with_toumei_ = GAME_OPTION(透明牌),
                        .seed_ = GAME_OPTION(种子).empty() ? "" : GAME_OPTION(种子) + stage_name,
                    },
                    .name_ = stage_name,
                    .with_inner_dora_ = GAME_OPTION(里宝牌),
                    .dora_num_ = GAME_OPTION(宝牌),
                    .ron_required_point_ = GAME_OPTION(起和点),
                    .image_path_ = main_stage.Global().ResourceDir(),
                    .player_descs_ = [&]()
                            {
                                std::vector<PlayerDesc> descs;
                                for (PlayerID pid = 0; pid < main_stage.Global().PlayerNum(); ++pid) {
                                    descs.emplace_back(Global().PlayerName(pid), Global().PlayerAvatar(pid, 45), Global().PlayerAvatar(pid, 30),
                                            k_winds[main_stage.players_[pid].wind_idx_],
                                            main_stage.players_[pid].score_);
                                }
                                return descs;
                            }()
                }))
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<PrepareStage>(Main(), game_table_);
    }

    virtual void NextStageFsm(PrepareStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            game_table_.AddToHand(pid); // for those not finish preparing
        }
        setter.Emplace<KiriStage>(Main(), game_table_);
    }

    virtual void NextStageFsm(KiriStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
    }

    Mahjong17Steps game_table_;
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

