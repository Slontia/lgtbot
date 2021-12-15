#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"
#include "utility/html.h"
#include "game_util/mahjong_17_steps.h"

const std::string k_game_name = "十七步";
const uint64_t k_max_player = 4; /* 0 means no max-player limits */

std::string GameOption::StatusInfo() const
{
    return "配牌时限 " + std::to_string(GET_VALUE(配牌时限)) + " 秒，切牌时限 " + std::to_string(GET_VALUE(切牌时限)) +
        " 秒，和牌牌型最少需要 " + std::to_string(GET_VALUE(起和点)) + " 点，场上有 " + std::to_string(GET_VALUE(宝牌)) +
        " 枚宝牌，" + (GET_VALUE(赤宝牌) ? "有" : "无") + "赤宝牌";
}

bool GameOption::IsValid(MsgSenderBase& reply) const
{
    if (PlayerNum() == 1) {
        reply() << "该游戏至少 2 名玩家参加，当前玩家数为 " << PlayerNum();
        return false;
    }
    if (PlayerNum() == 4 && GET_VALUE(宝牌) > 0) {
        reply() << "四人游戏不允许有宝牌，请将宝牌数量设置为 0，当前数量为 " << GET_VALUE(宝牌);
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 3; }

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
    MainStage(const GameOption& option, MatchBase& match) : GameStage(option, match), table_idx_(0)
    {
        std::variant<std::random_device, std::seed_seq> rd;
        std::mt19937 g([&]
            {
                if (option.GET_VALUE(种子).empty()) {
                    auto& real_rd = rd.emplace<std::random_device>();
                    return std::mt19937(real_rd());
                } else {
                    auto& real_rd = rd.emplace<std::seed_seq>(option.GET_VALUE(种子).begin(), option.GET_VALUE(种子).end());
                    return std::mt19937(real_rd);
                }
            }());
        const auto offset = std::uniform_int_distribution<uint32_t>(1, option.PlayerNum())(g);
        for (uint64_t pid = 0; pid < option.PlayerNum(); ++pid) {
            players_.emplace_back((pid + offset) % option.PlayerNum());
        }
    }

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<TableStage>(*this, "东一局");
    }

    virtual VariantSubStage NextSubStage(TableStage& sub_stage, const CheckoutReason reason) override
    {
        const uint32_t max_round = 4;
        ++table_idx_;
        if (table_idx_ < max_round) {
            for (auto& player : players_) {
                player.wind_idx_ = (player.wind_idx_ + 1) % max_round % 4;
            }
            return std::make_unique<TableStage>(*this, table_idx_ == 1 ? "东二局" :
                                                       table_idx_ == 2 ? "东三局" : "东四局");
        }
        return {};
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
            : GameStage(main_stage, "配牌阶段",
                    MakeStageCommand("从牌山中添加手牌", &PrepareStage::Add_,
                        VoidChecker("添加"), AnyArg("要添加的牌（无空格）", "123p445566m789s2z")),
                    MakeStageCommand("将手牌放回牌山", &PrepareStage::Remove_,
                        VoidChecker("移除"), AnyArg("要移除的牌（无空格）", "1p44m")),
                    MakeStageCommand("完成配牌，宣布立直", &PrepareStage::Finish_, VoidChecker("立直")),
                    MakeStageCommand("查看当前手牌配置情况", &PrepareStage::Info_, VoidChecker("赛况")))
            , game_table_(game_table)
    {}

    virtual void OnStageBegin() override
    {
        StartTimer(option().GET_VALUE(配牌时限));
        Boardcast() << "请私信裁判进行配牌，时限 " << option().GET_VALUE(配牌时限) << " 秒";
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            Tell(pid) << Markdown(game_table_.PrepareHtml(pid));
            Tell(pid) << "请配牌，您可通过「帮助」命令查看命令格式";
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
        if (IsReady(pid)) {
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
        if (IsReady(pid)) {
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

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "请私信裁判查看手牌和牌山情况";
            return StageErrCode::FAILED;
        }
        reply() << Markdown(game_table_.PrepareHtml(pid));
        return StageErrCode::OK;
    }

    Mahjong17Steps& game_table_;
};

class KiriStage : public SubGameStage<>
{
   public:
    KiriStage(MainStage& main_stage, Mahjong17Steps& game_table)
            : GameStage(main_stage, "切牌阶段",
                    MakeStageCommand("查看各个玩家舍牌情况", &KiriStage::Info_, VoidChecker("赛况")),
                    MakeStageCommand("从牌山中切出该牌", &KiriStage::Kiri_, AnyArg("舍牌", "2s")))
            , game_table_(game_table)
    {}

    virtual void OnStageBegin() override
    {
        StartTimer(option().GET_VALUE(切牌时限));
        Boardcast() << "请从牌山中选择一张切出去，时限 " << option().GET_VALUE(切牌时限) << " 秒";
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            Boardcast() << "请从牌山中选择一张切出去，时限 " << option().GET_VALUE(切牌时限) << " 秒";
        }
    }

   private:
    CheckoutErrCode OnTimeout()
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                Hook(pid);
            }
        }
        return CheckoutErrCode::Condition(OnOver_(), StageErrCode::CHECKOUT, StageErrCode::CONTINUE);
    }

    AtomReqErrCode Kiri_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (IsReady(pid)) {
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

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown(game_table_.PublicHtml());
        } else {
            reply() << Markdown(game_table_.KiriHtml(pid));
        }
        return StageErrCode::OK;
    }

    void SendInfo_()
    {
        Boardcast() << Markdown(game_table_.PublicHtml());
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            Tell(pid) << Markdown(game_table_.KiriHtml(pid));
        }
    }

    virtual void OnAllPlayerReady() override
    {
        Boardcast() << "全员切牌完毕，公布当前赛况";
        OnOver_();
    }

    Mahjong17Steps& game_table_;

  private:
    bool OnOver_()
    {
        const auto state = game_table_.RoundOver();
        SendInfo_();
        switch (state) {
        case Mahjong17Steps::GameState::CONTINUE:
            Boardcast() << "全员安全，请继续私信裁判切牌，时限 " << option().GET_VALUE(切牌时限) << " 秒";
            for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
                Tell(pid) << "请继续切牌，时限 " << option().GET_VALUE(切牌时限) << " 秒";
            }
            ClearReady();
            StartTimer(option().GET_VALUE(切牌时限));
            return false;
        case Mahjong17Steps::GameState::HAS_RON: {
            // FIXME: if timeout, the ready set any_ready is not be set, then the round will not be over
            std::vector<std::pair<PlayerID, int32_t>> player_scores;
            for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
                main_stage().players_[pid].score_ += game_table_.PointChange(pid);
                player_scores.emplace_back(pid, main_stage().players_[pid].score_);
            }
            std::ranges::sort(player_scores, [](const auto& _1, const auto& _2) { return _1.second > _2.second; });
            auto sender = Boardcast();
            sender << "有玩家和牌，本局结束，当前各个玩家分数：";
            for (uint32_t i = 0; i < player_scores.size(); ++i) {
                sender << "\n" << (i + 1) << " 位：" << At(player_scores[i].first) << "【" << player_scores[i].second << " 分】";
            }
            return true;
        }
        case Mahjong17Steps::GameState::FLOW:
            Boardcast() << "十七巡结束，流局";
            return true;
        }
        return true;
    }
};

class TableStage : public SubGameStage<PrepareStage, KiriStage>
{
  public:
    TableStage(MainStage& main_stage, const std::string& stage_name)
        : GameStage(main_stage, stage_name)
        , game_table_(Mahjong17Steps(Mahjong17StepsOption{
                    .name_ = stage_name,
                    .with_red_dora_ = main_stage.option().GET_VALUE(赤宝牌),
                    .dora_num_ = main_stage.option().GET_VALUE(宝牌),
                    .ron_required_point_ = main_stage.option().GET_VALUE(起和点),
                    .seed_ = main_stage.option().GET_VALUE(种子) + stage_name,
                    .image_path_ = main_stage.option().ResourceDir(),
                    .player_descs_ = [&]()
                            {
                                std::vector<PlayerDesc> descs;
                                for (PlayerID pid = 0; pid < main_stage.option().PlayerNum(); ++pid) {
                                    descs.emplace_back(PlayerName(pid), k_winds[main_stage.players_[pid].wind_idx_],
                                            main_stage.players_[pid].score_);
                                }
                                return descs;
                            }()
                }))
    {}

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<PrepareStage>(main_stage(), game_table_);
    }

    virtual VariantSubStage NextSubStage(PrepareStage& sub_stage, const CheckoutReason reason) override
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            game_table_.AddToHand(pid); // for those not finish preparing
        }
        return std::make_unique<KiriStage>(main_stage(), game_table_);
    }

    virtual VariantSubStage NextSubStage(KiriStage& sub_stage, const CheckoutReason reason) override
    {
        return {};
    }

    Mahjong17Steps game_table_;
};

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (!options.IsValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}

