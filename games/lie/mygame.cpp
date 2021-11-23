#include <array>
#include <functional>
#include <memory>

#include "game_framework/game_main.h"
#include "game_framework/game_stage.h"
#include "game_framework/game_options.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "LIE";
const uint64_t k_max_player = 2; /* 0 means no max-player limits */

std::string GameOption::StatusInfo() const
{
    std::stringstream ss;
    ss << "集齐全部" << GET_VALUE(数字种类) << "种数字，或持有单个数字数量达到" << GET_VALUE(失败数量) << "时玩家失败";
    return ss.str();
}

bool GameOption::IsValid(MsgSenderBase& reply) const
{
    if (PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

class MyTable
{
  public:
    struct Result
    {
        PlayerID loser_;
        PlayerID questioner_;
        int actual_number_;
        bool is_lie_;
    };

    MyTable(const GameOption& option)
        : option_(option)
        , table_(option.GET_VALUE(失败数量) * 2 + 3, option.GET_VALUE(数字种类))
        , player_nums_{std::vector<int>(option.GET_VALUE(数字种类), 0),
                        std::vector<int>(option.GET_VALUE(数字种类), 0)}
    {
        table_.SetTableStyle(" align=\"center\" cellpadding=\"1\" cellspacing=\"1\" ");
        table_.MergeRight(0, 0, option.GET_VALUE(数字种类));
        table_.MergeRight(table_.Row() - 1, 0, option.GET_VALUE(数字种类));
        const auto mid_row = option.GET_VALUE(失败数量) + 1;
        for (uint32_t col = 0; col < option.GET_VALUE(数字种类); ++col) {
            for (uint32_t i = 1; i <= option.GET_VALUE(失败数量); ++i) {
                table_.Get(mid_row + i, col).SetContent(Image_("blank"));
                table_.Get(mid_row - i, col).SetContent(Image_("blank"));
            }
        }
    }

    void SetName(std::string p1_name, std::string p2_name)
    {
        table_.Get(0, 0).SetContent(HTML_COLOR_FONT_HEADER(#345a88) " **" + p1_name + "** " HTML_FONT_TAIL);
        table_.Get(table_.Row() - 1, 0).SetContent(HTML_COLOR_FONT_HEADER(#880015) " **" + p2_name + "** " HTML_FONT_TAIL);
    }

    void Lose(const Result result)
    {
        if (last_result_.has_value()) {
            DrawLose_(false);
        }
        last_result_.emplace(result);
        ++player_nums_[result.loser_][result.actual_number_ - 1];
        DrawLose_(true);
    }

    bool CheckOver(const PlayerID questioner)
    {
        bool has_all_num = true;
        for (uint32_t num_idx = 0; num_idx < option_.GET_VALUE(数字种类); ++ num_idx) {
            const int count = player_nums_[static_cast<uint64_t>(questioner)][num_idx];
            if (count >= option_.GET_VALUE(失败数量)) {
                for (int i = 0; i < count; ++i) {
                    table_.Get(option_.GET_VALUE(失败数量) + 1 + (questioner == 1 ? 1 : -1) * (i + 1), num_idx)
                          .SetContent(Image_("death"));
                }
                return true;
            } else if (count == 0) {
                has_all_num = false;
            }
        }
        if (has_all_num) {
            for (uint32_t num_idx = 0; num_idx < option_.GET_VALUE(数字种类); ++ num_idx) {
                table_.Get(option_.GET_VALUE(失败数量) + 1 + (questioner == 1 ? 1 : -1), num_idx)
                      .SetContent(Image_("death"));
            }
        }
        return has_all_num;
    }

    std::string ToHtml(const PlayerID pid)
    {
        if (pid == 0) {
            table_.Get(0, 0).SetColor("#7092be"); // light blue
            table_.Get(table_.Row() - 1, 0).SetColor("white");
        } else {
            table_.Get(0, 0).SetColor("white");
            table_.Get(table_.Row() - 1, 0).SetColor("#b97a57"); // light red
        }
        return table_.ToString();
    }

  private:
    std::string Image_(std::string name) const
    {
        return std::string("![](file://") + option_.ResourceDir() + "/" + std::move(name) + ".png)";
    }

    void DrawLose_(const bool with_light)
    {
        const int32_t direct = last_result_->loser_ == 1 ? 1 : -1;
        const int32_t offset = player_nums_[last_result_->loser_][last_result_->actual_number_ - 1];
        const char color = last_result_->questioner_ == 1 ? 'r' : 'b';
        const char* const lie = last_result_->is_lie_ ? "lie_" : "truth_";
        table_.Get(option_.GET_VALUE(失败数量) + 1 + direct * offset, last_result_->actual_number_ - 1)
              .SetContent(Image_(std::string(with_light ? "light_" : "") + lie + color));
        for (uint32_t col = 0; col < option_.GET_VALUE(数字种类); ++col) {
            table_.Get(option_.GET_VALUE(失败数量) + 1, col)
                  .SetContent(Image_("point_" + std::to_string(col + 1) + (last_result_->loser_ == 1 ? "_r" : "_b")));
        }
    }

    const GameOption& option_;
    Table table_;
    std::array<std::vector<int>, 2> player_nums_;
    std::optional<Result> last_result_;
};

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
   public:
    MainStage(const GameOption& option, MatchBase& match);
    virtual VariantSubStage OnStageBegin() override;
    virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;
    int64_t PlayerScore(const PlayerID pid) const;
    MyTable& table() { return table_; }

   private:
    bool JudgeOver();
    virtual void OnPlayerLeave(const PlayerID pid) override;
    void Info_()
    {
        Boardcast() << Markdown("## 第" + std::to_string(round_) + "回合\n\n" + table_.ToHtml(questioner_));
    }

    MyTable table_;
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
    RoundStage(MainStage& main_stage, const uint64_t round, const uint64_t questioner)
            : GameStage(main_stage, "第" + std::to_string(round) + "回合"),
              questioner_(questioner),
              actual_number_(1),
              lie_number_(1),
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
        // add(loser_, questioner_, actual_number_, is_lie)
        main_stage().table().Lose(MyTable::Result{loser_, questioner_, actual_number_, actual_number_ != lie_number_});
        auto boardcast = Boardcast();
        boardcast << "实际数字为" << actual_number_ << "，"
                  << (doubt ? "怀疑" : "相信") << (suc ? "成功" : "失败") << "，"
                  << "玩家" << At(loser_) << "获得数字" << actual_number_ << "\n数字获得情况：\n"
                  << At(PlayerID(0)) << "：" << At(PlayerID(1));
        return {};
    }

   private:
    const PlayerID questioner_;
    int actual_number_;
    int lie_number_;
    PlayerID loser_;
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , table_(option)
        , questioner_(0)
        , round_(1)
{
}

MainStage::VariantSubStage MainStage::OnStageBegin()
{
    table_.SetName(PlayerName(0), PlayerName(1));
    return std::make_unique<RoundStage>(*this, 1, std::rand() % 2);
}

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage, const CheckoutReason reason)
{
    questioner_ = sub_stage.loser();
    if (JudgeOver()) {
        return {};
    }
    return std::make_unique<RoundStage>(*this, ++round_, questioner_);
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
    const bool is_over = table_.CheckOver(questioner_);
    Info_();
    return is_over;
}

void MainStage::OnPlayerLeave(const PlayerID pid) { leaver_ = pid; }

MainStageBase* MakeMainStage(MsgSenderBase& reply, const GameOption& options, MatchBase& match)
{
    if (!options.IsValid(reply)) {
        return nullptr;
    }
    return new MainStage(options, match);
}
