#include <array>
#include <functional>
#include <memory>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "LIE";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

const std::string GameOption::StatusInfo() const
{
    std::stringstream ss;
    ss << "集齐全部" << GET_VALUE(数字种类) << "种数字，或持有单个数字数量达到" << GET_VALUE(失败数量) << "时玩家失败";
    return ss.str();
}

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option, MatchBase& match);
    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;
    int64_t PlayerScore(const PlayerID pid) const;

   private:
    bool JudgeOver();
    virtual void OnPlayerLeave(const PlayerID pid) override;

    PlayerID questioner_;
    uint64_t round_;
    std::array<std::vector<int>, 2> player_nums_;
    std::optional<PlayerID> leaver_;
};

class NumberStage : public SubGameStage<>
{
   public:
    NumberStage(MainStage& main_stage, const PlayerID questioner, int& actual_number, int& lie_number)
            : GameStage(main_stage, "设置数字阶段",
                    MakeStageCommand("设置数字", &NumberStage::Number_,
                        ArithChecker<int>(1, main_stage.option().GET_VALUE(数字种类), "实际数字"),
                        ArithChecker<int>(1, main_stage.option().GET_VALUE(数字种类), "提问数字")))
            , questioner_(questioner)
            , actual_number_(actual_number)
            , lie_number_(lie_number)
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "请" << At(questioner_) << "设置数字";
        SetReady(1 - questioner_);
        StartTimer(60);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (questioner_ == pid) {
            actual_number_ = std::rand() % option().GET_VALUE(数字种类) + 1;
            lie_number_ = std::rand() % 5 >= 2 ? std::rand() % option().GET_VALUE(数字种类) + 1
                                               : actual_number_; // 50% same
            return StageErrCode::READY;
        }
        return StageErrCode::OK;
    }

   private:
    AtomReqErrCode Number_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int actual_number,
            const int lie_number)
    {
        if (pid != questioner_) {
            reply() << "[错误] 本回合您为猜测者，无法设置数字";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信裁判选择数字，公开选择无效";
            return StageErrCode::FAILED;
        }
        actual_number_ = actual_number;
        lie_number_ = lie_number;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override { return StageErrCode::CHECKOUT; }

    const PlayerID questioner_;
    int& actual_number_;
    int& lie_number_;
};

class GuessStage : public SubGameStage<>
{
   public:
    GuessStage(MainStage& main_stage, const PlayerID guesser)
            : GameStage(main_stage, "猜测阶段",
                    MakeStageCommand("猜测", &GuessStage::Guess_, BoolChecker("质疑", "相信"))),
              guesser_(guesser),
              doubt_(false)
    {}

    virtual void OnStageBegin() override
    {
        Boardcast() << "请" << At(guesser_) << "选择「相信」或「质疑」";
        SetReady(1 - guesser_);
        StartTimer(60);
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (guesser_ == pid) {
            doubt_ = std::rand() % 2;
            return StageErrCode::CHECKOUT;
        }
        return StageErrCode::OK;
    }

    bool doubt() const { return doubt_; }

   private:
    AtomReqErrCode Guess_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool doubt)
    {
        if (pid != guesser_) {
            reply() << "[错误] 本回合您为提问者，无法猜测";
            return StageErrCode::FAILED;
        }
        doubt_ = doubt;
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override { return StageErrCode::CHECKOUT; }

    const PlayerID guesser_;
    bool doubt_;
};

class RoundStage : public SubGameStage<NumberStage, GuessStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round, const uint64_t questioner, std::array<std::vector<int>, 2>& player_nums)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合"),
              questioner_(questioner),
              actual_number_(1),
              lie_number_(1),
              player_nums_(player_nums),
              loser_(0)
    {}

    PlayerID loser() const { return loser_; }

    virtual VariantSubStage OnStageBegin() override
    {
        return std::make_unique<NumberStage>(main_stage(), questioner_, actual_number_, lie_number_);
    }

    virtual VariantSubStage NextSubStage(NumberStage& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        Tell(questioner_) << (reason == CheckoutReason::BY_TIMEOUT ? "设置超时，视为" : "设置成功")
                          << "\n实际数字：" << actual_number_ << "\n提问数字：" << lie_number_;
        Boardcast() << At(questioner_) << "提问数字" << lie_number_;
        return std::make_unique<GuessStage>(main_stage(), 1 - questioner_);
    }

    virtual VariantSubStage NextSubStage(GuessStage& sub_stage, const CheckoutReason reason) override
    {
        if (reason == CheckoutReason::BY_LEAVE) {
            return {};
        }
        const bool doubt = reason == CheckoutReason::BY_TIMEOUT ? false : sub_stage.doubt();
        if (reason == CheckoutReason::BY_TIMEOUT) {
            Tell(questioner_) << "选择超时，默认为相信";
        }
        const bool suc = doubt ^ (actual_number_ == lie_number_);
        loser_ = suc ? questioner_ : PlayerID{1 - questioner_};
        ++player_nums_[loser_][actual_number_ - 1];
        Table table(option().GET_VALUE(数字种类) + 1, 3);
        {
            auto boardcast = Boardcast();
            boardcast << "实际数字为" << actual_number_ << "，"
                      << (doubt ? "怀疑" : "相信") << (suc ? "成功" : "失败") << "，"
                      << "玩家" << At(loser_) << "获得数字" << actual_number_ << "\n数字获得情况：\n"
                      << At(PlayerID(0)) << "：" << At(PlayerID(1));
            table.Get(0, 0).content_ = PlayerName(PlayerID(0));
            table.Get(0, 1).content_ = "数字";
            table.Get(0, 2).content_ = PlayerName(PlayerID(1));
            for (int num = 1; num <= option().GET_VALUE(数字种类); ++num) {
                boardcast << "\n" << player_nums_[0][num - 1] << " [" << num << "] " << player_nums_[1][num - 1];
                table.Get(num, 0).content_ = std::to_string(player_nums_[0][num - 1]);
                table.Get(num, 1).content_ = "[" + std::to_string(num) + "]";
                table.Get(num, 2).content_ = std::to_string(player_nums_[1][num - 1]);
                table.Get(num, 1).color_ = "Aquamarine";
            }
            table.Get(actual_number_, loser_ * 2).color_ = "AntiqueWhite";
        }
        Boardcast() << Markdown(table.ToString());
        return {};
    }

   private:
    const PlayerID questioner_;
    int actual_number_;
    int lie_number_;
    std::array<std::vector<int>, 2>& player_nums_;
    PlayerID loser_;
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , questioner_(0)
        , round_(1)
        , player_nums_{std::vector<int>(option.GET_VALUE(数字种类), 0),
                        std::vector<int>(option.GET_VALUE(数字种类), 0)}
{}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    return std::make_unique<RoundStage>(*this, 1, std::rand() % 2, player_nums_);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    questioner_ = sub_stage.loser();
    if (JudgeOver()) {
        return {};
    }
    return std::make_unique<RoundStage>(*this, ++round_, questioner_, player_nums_);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const
{
    const PlayerID loser_pid = leaver_.has_value() ? *leaver_ : questioner_;
    return pid == loser_pid ? 0 : 1;
}

bool MainStage::JudgeOver()
{
    if (leaver_.has_value()) {
        return true;
    }
    bool has_all_num = true;
    for (const int count : player_nums_[questioner_]) {
        if (count >= option().GET_VALUE(失败数量)) {
            return true;
        } else if (count == 0) {
            has_all_num = false;
        }
    }
    return has_all_num;
}

void MainStage::OnPlayerLeave(const PlayerID pid) { leaver_ = pid; }

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (options.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << options.PlayerNum();
        return nullptr;
    }
    return new MainStage(options, match);
}
