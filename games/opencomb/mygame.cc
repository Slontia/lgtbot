// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <array>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <regex>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/random.h"
#include "utility/html.h"

#include "opencomb.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "开放蜂巢",
    .developer_ = "铁蛋",
    .description_ = "使用特殊道具改变经典地图，体验不一样的数字蜂巢玩法",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 10; }
uint32_t Multiple(const CustomOptions& options) { return GET_OPTION_VALUE(options, 种子).empty() ? 2 : 0; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options) {
    if (GET_OPTION_VALUE(game_options, 模式) == 1 && generic_options_readonly.PlayerNum() < 2) {
        reply() << "「云顶」对战模式至少需要 2 人参加游戏";
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 1;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
    InitOptionsCommand("修改游戏配置：卡池、地图、游戏模式、道具、连线奖励",
            [] (CustomOptions& game_options,
                MutableGenericOptions& generic_options,
                const int32_t& card,
                const std::string map,
                const std::string mode,
                const std::string special,
                const std::string line_rewards
            ) {
                GET_OPTION_VALUE(game_options, 卡池) = card;
                for (size_t i = 0; i < map_string.size(); ++i) {
                    if (map_string[i].first == map) {
                        GET_OPTION_VALUE(game_options, 地图) = i;
                        break;
                    }
                }
                if (mode == "云顶") GET_OPTION_VALUE(game_options, 模式) = 1;
                if (special == "关闭") GET_OPTION_VALUE(game_options, 道具) = false;
                if (line_rewards == "关闭") GET_OPTION_VALUE(game_options, 连线奖励) = false;
                return NewGameMode::MULTIPLE_USERS;
            },
            AlterChecker<uint32_t>({{"经典", 0}, {"癞子", 1}, {"空气", 2}, {"混乱", 3}}),
            OptionalDefaultChecker<BasicChecker<std::string>>("经典", "地图", "随机"),
            OptionalDefaultChecker<BasicChecker<std::string>>("传统", "模式", "云顶"),
            OptionalDefaultChecker<BasicChecker<std::string>>("开启", "道具", "[开启/关闭]"),
            OptionalDefaultChecker<BasicChecker<std::string>>("开启", "连线奖励", "[开启/关闭]")),
};

// ========== GAME STAGES ==========

static const std::array<std::vector<int32_t>, k_direct_max> all_points{
    std::vector<int32_t>{3, 4, 8, 10, 0},
    std::vector<int32_t>{1, 5, 9, 10, 0},
    std::vector<int32_t>{2, 6, 7, 10, 0}
};

struct Player
{
    Player(std::string resource_path, const uint32_t map) : comb_(new OpenComb(std::move(resource_path), map)) {}
    Player(Player&&) = default;
    int hp = 200;
    std::unique_ptr<OpenComb> comb_;
};

class RoundStage;
class SelectStage;

class MainStage : public MainGameStage<RoundStage, SelectStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看蜂巢初始状态，用于预览序号", &MainStage::InitInfo_, AlterChecker<uint32_t>({{"地图", 0}, {"盘面", 0}})))
        , round_(-1)
        , alive_(Global().PlayerNum())
        , player_out_(Global().PlayerNum(), 0)
        , player_leave_(Global().PlayerNum(), false)
    {
        srand((unsigned int)time(NULL));
        const uint32_t map = GAME_OPTION(地图) == 0 ? rand() % k_map_size : GAME_OPTION(地图) - 1;
        for (uint64_t i = 0; i < Global().PlayerNum(); ++i) {
            players_.emplace_back(Global().ResourceDir(), map);
        }

        // seed
        seed_str_ = GAME_OPTION(种子);
        if (seed_str_.empty()) {
            std::random_device rd;
            std::uniform_int_distribution<unsigned long long> dis;
            seed_str_ = std::to_string(dis(rd));
        }
        g = MakeRng(seed_str_);

        // 卡池模板生成
        std::vector<AreaCard> classical_cards_;
        std::vector<AreaCard> wild_cards_;
        std::vector<AreaCard> air_cards_;
        std::vector<AreaCard> chaos_cards_;
        for (const int32_t point_0 : all_points[0]) {
            for (const int32_t point_1 : all_points[1]) {
                for (const int32_t point_2 : all_points[2]) {
                    int32_t count0 = (point_0 == 0) + (point_1 == 0) + (point_2 == 0);
                    int32_t count10 = (point_0 == 10) + (point_1 == 10) + (point_2 == 10);
                    if (count0 == 0 && count10 == 0) {
                        classical_cards_.emplace_back(point_0, point_1, point_2);
                    }
                    if (count0 == 0 && count10 == 1) {
                        wild_cards_.emplace_back(point_0, point_1, point_2);
                    }
                    if (count0 == 1 && count10 == 0) {
                        air_cards_.emplace_back(point_0, point_1, point_2);
                    }
                    if (count0 > 0 && count10 > 0) {
                        chaos_cards_.emplace_back(point_0, point_1, point_2);
                    }
                }
            }
        }
        
        if (GAME_OPTION(卡池) == 0) {
            cards_.insert(cards_.end(), classical_cards_.begin(), classical_cards_.end());
            cards_.insert(cards_.end(), classical_cards_.begin(), classical_cards_.end());
        } else if (GAME_OPTION(卡池) == 1) {
            cards_.insert(cards_.end(), classical_cards_.begin(), classical_cards_.end());
            cards_.insert(cards_.end(), wild_cards_.begin(), wild_cards_.end());
        } else if (GAME_OPTION(卡池) == 2) {
            cards_.insert(cards_.end(), classical_cards_.begin(), classical_cards_.end());
            cards_.insert(cards_.end(), air_cards_.begin(), air_cards_.end());
        } else if (GAME_OPTION(卡池) == 3) {
            cards_.insert(cards_.end(), chaos_cards_.begin(), chaos_cards_.end());
            SeededShuffle(classical_cards_.begin(), classical_cards_.end(), g);
            SeededShuffle(wild_cards_.begin(), wild_cards_.end(), g);
            SeededShuffle(air_cards_.begin(), air_cards_.end(), g);
            cards_.insert(cards_.end(), classical_cards_.begin(), classical_cards_.begin() + 10);
            cards_.insert(cards_.end(), wild_cards_.begin(), wild_cards_.begin() + 10);
            cards_.insert(cards_.end(), air_cards_.begin(), air_cards_.begin() + 10);
            cards_.emplace_back(0, 0, 0);
            cards_.emplace_back(0, 0, 0);
        }

        std::vector<AreaCard> tmp_cards = cards_;

        for (uint32_t i = 0; i < 2; ++i) cards_.emplace_back(10, 10, 10);

        if (GAME_OPTION(道具)) {
            for (uint32_t i = 0; i < 2; ++i) cards_.emplace_back("wall");
            for (uint32_t i = 0; i < 2; ++i) cards_.emplace_back("wall_broken");
            for (uint32_t i = 0; i < 4; ++i) cards_.emplace_back("erase");
            for (uint32_t i = 0; i < 3; ++i) cards_.emplace_back("move");
            for (uint32_t i = 0; i < 2; ++i) cards_.emplace_back("reshape");
        }

        SeededShuffle(cards_.begin(), cards_.end(), g);

        if (GAME_OPTION(道具)) {
            // first round
            const std::string special_cards[5] = {"wall", "wall_broken", "erase", "move", "reshape"};
            std::uniform_int_distribution<int32_t> dist(0, 4);
            int32_t start = dist(g);
            cards_.insert(cards_.begin(), special_cards[start]);
        } else {
            round_ = 0;
        }

        // 云顶模式
        if (GAME_OPTION(模式) == 1) {
            if (!GAME_OPTION(连线奖励)) {
                for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                    players_[pid].hp = 150;
                }
            }
            dRate_ = std::pow(M_E, Global().PlayerNum() / 6.0) / M_E;
            iRate_ = dRate_;
            SeededShuffle(tmp_cards.begin(), tmp_cards.end(), g);
            cards2_.insert(cards2_.end(), tmp_cards.begin(), tmp_cards.end());
            SeededShuffle(tmp_cards.begin(), tmp_cards.end(), g);
            cards2_.insert(cards2_.end(), tmp_cards.begin(), tmp_cards.end());
        }

        it_ = cards_.begin();
        it2_ = cards2_.begin();
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;

    virtual void NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    bool hasBots();

    int64_t PlayerScore(const PlayerID pid) const
    {
        if (GAME_OPTION(连线奖励)) {
            return players_[pid].comb_->Score() + players_[pid].comb_->ExtraScore();
        } else {
            return players_[pid].comb_->Score();
        }
    }

    void SetPlayerBoard(html::Table& table, const int pos, const PlayerID pid, const bool isEliminated) {
        std::string board = "### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid) +
                            "\n\n### " HTML_COLOR_FONT_HEADER(green) "当前积分：" + std::to_string(PlayerScore(pid)) + HTML_FONT_TAIL +
                            (GAME_OPTION(连线奖励) ? (HTML_COLOR_FONT_HEADER(blue) "（" + std::to_string(players_[pid].comb_->Score()) + "+" + 
                            std::to_string(players_[pid].comb_->ExtraScore()) + "）" + HTML_FONT_TAIL) : "") +
                            (GAME_OPTION(模式) == 1 ? (HTML_COLOR_FONT_HEADER(#8B0000) "　血量：" + std::to_string(players_[pid].hp) + HTML_FONT_TAIL) : "") +
                            "\n\n" +
                            players_[pid].comb_->ToHtml();
        if (isEliminated) {
            table.Get(pos / 2, pos % 2).SetColor("#C0C0C0").SetContent(board);
        } else {
            table.Get(pos / 2, pos % 2).SetContent(board);
        }
    }

    std::string CombHtml(const std::string& str)
    {
        html::Table table(players_.size() / 2 + 1, 2);
        table.SetTableStyle("align=\"center\" cellpadding=\"20\" cellspacing=\"0\"");
        int pos = 0;
        // 传统模式或未淘汰玩家
        for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
            if (player_out_[pid] == 0) {
                SetPlayerBoard(table, pos++, pid, false);
            }
        }
        if (players_.size() % 2 && GAME_OPTION(模式) == 0) {
            table.MergeRight(table.Row() - 1, 0, 2);
        }
        if (GAME_OPTION(模式) == 1) {
            // 云顶模式已淘汰玩家
            for (PlayerID pid = 0; pid.Get() < players_.size(); ++pid) {
                if (player_out_[pid] > 0) {
                    SetPlayerBoard(table, pos++, pid, true);
                }
            }
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << dRate_;
            std::string rate = ss.str();
            return str + "（伤害倍率：" + rate + "）" + GetStyle(Global().ResourceDir()) + table.ToString();
        }
        return str + GetStyle(Global().ResourceDir()) + table.ToString();
    }

    std::string GetName(std::string x) {
        std::string ret = "";
        int n = x.length();
        if (n == 0) return ret;

        int l = 0;
        int r = n - 1;

        if (x[0] == '<') l++;
        if (x[r] == '>') {
            while (r >= 0 && x[r] != '(') r--;
            r--;
        }

        for (int i = l; i <= r; i++) {
            ret += x[i];
        }
        return ret;
    }

    bool CheckGameOver_(const std::vector<Player>& players_) const
    {
        if (GAME_OPTION(模式) == 0) {
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (players_[pid].comb_->IsAllFilled()) {
                    return true;
                }
            }
        } else {
            if (alive_ <= 1) {
                return true;
            }
        }
        return false;
    }

    int32_t round_;
    std::vector<Player> players_;
    std::string seed_str_;
    std::mt19937 g;

    int32_t alive_;
    std::vector<int32_t> player_out_;
    std::vector<bool> player_leave_;
    std::vector<std::unordered_map<int32_t, int32_t>> fought_list_;
    double dRate_, iRate_;
    PlayerID last_mirror_;
    std::vector<AreaCard> cards2_;
    decltype(cards2_)::iterator it2_;

  private:
    CompReqErrCode InitInfo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t null)
    {
        html::Table table(2, 2);
        table.SetTableStyle("align=\"center\" cellpadding=\"10\" cellspacing=\"0\"");
        table.Get(0, 0).SetContent("&nbsp;\n### " + Global().PlayerAvatar(pid, 40) + "&nbsp;&nbsp; " + Global().PlayerName(pid));
        table.Get(0, 1).SetContent("&nbsp;\n### 初始地图");
        table.Get(1, 0).SetContent(players_[pid].comb_->ToHtml());
        table.Get(1, 1).SetContent(players_[pid].comb_->GetInitTable_());
        reply() << Markdown("<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir()) + table.ToString());
        return StageErrCode::OK;
    }

    void NewStage_(SubStageFsmSetter& setter);

    std::vector<AreaCard> cards_;
    decltype(cards_)::iterator it_;
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round, const AreaCard& card)
            : StageFsm(main_stage, (round ? "第" + std::to_string(round) + "回合" : "道具回合"),
                MakeStageCommand(*this, "放置砖块（或使用道具）", &RoundStage::Set_, ArithChecker<uint32_t>(1, 73, "位置")),
                MakeStageCommand(*this, "使用移动道具", &RoundStage::Move_, ArithChecker<uint32_t>(1, 73, "起始"), ArithChecker<uint32_t>(1, 73, "结束")),
                MakeStageCommand(*this, "跳过本回合（仅限特殊道具）", &RoundStage::Pass_, VoidChecker("pass")),
                MakeStageCommand(*this, "查看本回合开始时蜂巢情况，可用于图片重发", &RoundStage::Info_, VoidChecker("赛况")))
            , card_(card)
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合"))
    {}

    virtual void OnStageBegin() override
    {
        if (card_.Type() == "") {
            Global().Boardcast() << "本回合砖块为 " << card_.CardName() << "，请公屏或私信裁判设置数字：";
        } else {
            Global().Boardcast() << "本回合为特殊道具 [" << card_.CardName() << "]，请公屏或私信裁判使用道具，或「pass」跳过本回合：";
        }
        SendInfo(Global().BoardcastMsgSender());
        Global().StartTimer(GAME_OPTION(局时));
    }

  private:
    void HandleUnreadyPlayers_()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid) && Main().player_out_[pid] == 0) {
                auto& player = Main().players_[pid];
                auto sender = Global().Boardcast();
                
                if (card_.Type() == "") {
                    if (player.comb_->IsAllFilled()) {
                        // 云顶模式盘面已满，放入位置1
                        const auto [point, extra_point] = player.comb_->Fill(1, card_);
                        sender << At(pid) << "因为超时未做选择，盘面已满自动填入位置 1";
                        if (point > 0) sender << "，意外收获 " + std::to_string(point) + " 点积分";
                        else if (point < 0) sender << "，损失 " + std::to_string(-point) + " 点积分";
                        if (GAME_OPTION(连线奖励)) {
                            if (extra_point > 0) sender << "和连线额外奖励 " + std::to_string(extra_point) + " 点积分";
                            else if (extra_point < 0) sender << "，损失奖励得分 " + std::to_string(-extra_point) + " 点积分";
                        }
                    } else {
                        // 超时自动放入第一个空位
                        const auto [num, result] = player.comb_->SeqFill(card_);
                        sender << At(pid) << "因为超时未做选择，自动填入空位置 " << num;
                        if (result.score_ > 0) {
                            sender << "，意外收获 " << result.score_ << " 点积分";
                        }
                        if (GAME_OPTION(连线奖励) && result.extra_score_ > 0) {
                            sender << "和连线奖励分数 " << result.extra_score_ << " 点积分";
                        }
                    }
                } else {
                    sender << At(pid) << "因为超时未做选择，自动跳过特殊道具";
                }
            }
        }
        Global().HookUnreadyPlayers();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayers_();
        if (GAME_OPTION(模式) == 0) {
            return StageErrCode::CHECKOUT;
        } else {
            return Battle();
        }
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayers_();
        if (GAME_OPTION(模式) == 0) {
            return StageErrCode::CHECKOUT;
        } else {
            return Battle();
        }
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = Main().players_[pid];
        if (card_.Type() == "") {
            if (player.comb_->IsAllFilled()) {
                player.comb_->Fill(1, card_);
            } else {
                player.comb_->SeqFill(card_);
            }
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t num)
    {
        if (Global().IsReady(pid)) {
            reply() << "您本回合已经行动完成，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (card_.Type() == "move") {
            reply() << "[错误] 移动道具需要输入两个位置，包含起始和结束位置";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        if (player.comb_->IsWall(num) && !player.comb_->CanReplace(num) && card_.Type() != "erase" && card_.Type() != "reshape") {
            reply() << "[错误] 该位置为普通墙块，无法放置";
            return StageErrCode::FAILED;
        }
        if (player.comb_->IsFilled(num) && !player.comb_->CanReplace(num) && card_.Type() != "erase" && card_.Type() != "reshape" && GAME_OPTION(模式) == 0) {
            reply() << "[错误] 该位置已经有砖块了，当前位置不可被覆盖";
            return StageErrCode::FAILED;
        }
        if (card_.Type() == "erase" && !player.comb_->IsWall(num) && !player.comb_->IsFilled(num)) {
            reply() << "[错误] 消除失败，该位置为空，试试其它位置吧";
            return StageErrCode::FAILED;
        }
        const auto [point, extra_point] = player.comb_->Fill(num, card_);
        auto sender = reply();
        if (card_.Type() == "erase") {
            sender << "消除位置 " << num << " 成功";
        } else if (card_.Type() == "reshape") {
            if (player.comb_->IsFilled(num)) {
                sender << "将位置 " << num << " 转化为癞子";
            } else {
                sender << "重塑位置 " << num << " 成功";
            }
        } else {
            sender << "设置数字 " << num << " 成功";
        }
        if (point > 0) sender << "，本次操作收获 " + std::to_string(point) + " 点积分";
        else if (point < 0) sender << "，本次操作损失 " + std::to_string(-point) + " 点积分";
        if (GAME_OPTION(连线奖励)) {
            if (extra_point > 0) sender << "，连线额外奖励 " + std::to_string(extra_point) + " 点积分";
            else if (extra_point < 0) sender << "，损失奖励得分 " + std::to_string(-extra_point) + " 点积分";
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Move_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t from, const uint32_t to)
    {
        if (Global().IsReady(pid)) {
            reply() << "您本回合已经行动完成，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (card_.Type() != "move") {
            reply() << "[错误] 本回合并非移动道具，不能执行移动操作";
            return StageErrCode::FAILED;
        }

        auto& player = Main().players_[pid];
        if (!player.comb_->IsWall(from) && !player.comb_->IsFilled(from)) {
            reply() << "[错误] 移动的起始位置为空，仅能移动墙块和砖块";
            return StageErrCode::FAILED;
        }
        if (player.comb_->IsWall(to) && !player.comb_->CanReplace(to)) {
            reply() << "[错误] 移动的结束位置为墙块且不可覆盖";
            return StageErrCode::FAILED;
        }
        if (player.comb_->IsFilled(to) && !player.comb_->CanReplace(to) && GAME_OPTION(模式) == 0) {
            reply() << "[错误] 移动的结束位置为砖块且不可覆盖";
            return StageErrCode::FAILED;
        }
        const auto [point, extra_point] = player.comb_->Move(from, to);
        auto sender = reply();
        sender << "成功将位置 " << from << " 的砖块移动至 " << to;
        if (point > 0) sender << "，本次操作收获 " + std::to_string(point) + " 点积分";
        else if (point < 0) sender << "，本次操作损失 " + std::to_string(-point) + " 点积分";
        if (GAME_OPTION(连线奖励)) {
            if (extra_point > 0) sender << "，连线额外奖励 " + std::to_string(extra_point) + " 点积分";
            else if (extra_point < 0) sender << "，损失奖励得分 " + std::to_string(-extra_point) + " 点积分";
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (Global().IsReady(pid)) {
            reply() << "您本回合已经行动完成，无法重复设置";
            return StageErrCode::FAILED;
        }
        if (card_.Type() == "") {
            reply() << "[错误] 本回合为普通砖块，仅特殊道具可以pass";
            return StageErrCode::FAILED;
        }
        reply() << "您选择跳过本回合";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        SendInfo(reply);
        return StageErrCode::OK;
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
        if (card_.Type() == "") {
            sender() << Markdown(style + card_.ToHtml(Global().ResourceDir()), 64);
        } else {
            sender() << Image((Global().ResourceDir() / std::filesystem::path(card_.Type() + ".png")).string());
        }
    }

    // 对战逻辑参(fu)考(zhi)星星姬
    AtomReqErrCode Battle()
    {
        if (Main().alive_ <= 1) {
            return StageErrCode::CHECKOUT;
        }

        std::string result;
        std::vector<PlayerID> alive_players;

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0) {
                alive_players.push_back(pid);
            }
        }

        int32_t min_round_ = (Main().alive_ + 1) / 2 - 1;
        while (Main().fought_list_.size() > min_round_) {
            Main().fought_list_.erase(Main().fought_list_.begin());
        }

        if (Main().round_ >= 3) {
            std::unordered_map<int32_t, int32_t> fight_map;
            std::vector<PlayerID> list = alive_players;
            bool b;
            
            do {
                b = false;
                fight_map.clear();
                SeededShuffle(list.begin(), list.end(), Main().g);

                for (size_t i = 0; i < list.size() - 1; i += 2) {
                    for (const auto& hist : Main().fought_list_) {
                        if (hist.find(list[i]) != hist.end() && hist.at(list[i]) == list[i + 1]) {
                            b = true;
                            break;
                        }
                        if (hist.find(list[i + 1]) != hist.end() && hist.at(list[i + 1]) == list[i]) {
                            b = true;
                            break;
                        }
                    }
                    if (!b) {
                        fight_map[list[i]] = list[i + 1];
                    }
                }
            } while (b);

            Main().fought_list_.push_back(fight_map);

            while (Main().fought_list_.size() > min_round_) {
                Main().fought_list_.erase(Main().fought_list_.begin());
            }

            for (const auto& entry : fight_map) {
                const int32_t pid1 = entry.first;
                const int32_t pid2 = entry.second;
                const int64_t score1 = Main().PlayerScore(pid1);
                const int64_t score2 = Main().PlayerScore(pid2);
                const std::string name1 = Main().GetName(Global().PlayerName(pid1));
                const std::string name2 = Main().GetName(Global().PlayerName(pid2));

                int damage = (int)((score2 - score1) * Main().dRate_);
                if (score1 > score2) {
                    result += name1 + " vs " + name2 + " (" + std::to_string(damage) + ")\n";
                    Main().players_[pid2].hp += damage;
                } else if (score1 < score2) {
                    result += name1 + " (" + std::to_string(-damage) + ") vs " + name2 + "\n";
                    Main().players_[pid1].hp -= damage;
                } else {
                    result += name1 + " vs " + name2 + " (0)\n";
                }
            }

            if (list.size() % 2 == 1) {
                int32_t player0 = list.back();
                int32_t mirror;
                do {
                    mirror = list[rand() % (list.size() - 1)];
                } while (mirror == Main().last_mirror_);
                Main().last_mirror_ = mirror;

                const int64_t score0 = Main().PlayerScore(player0);
                const int64_t scoreM = Main().PlayerScore(mirror);
                if (score0 >= scoreM) {
                    result += Main().GetName(Global().PlayerName(player0)) + " vs " + Main().GetName(Global().PlayerName(mirror)) + "(镜像)\n";
                } else {
                    int damage = (int)((score0 - scoreM) * Main().dRate_);
                    result += Main().GetName(Global().PlayerName(player0)) + " (" + std::to_string(damage) + ") vs " + Main().GetName(Global().PlayerName(mirror)) + "(镜像)\n";
                    Main().players_[player0].hp += damage;
                }
            }
            int32_t fought_round = (Main().round_ - 2 - (Main().round_ - 1) / 7);
            if (Main().dRate_ < 1) {
                Main().dRate_ = std::min(std::exp(fought_round * 0.22 / Global().PlayerNum()) * Main().iRate_, 1.0);
            }
            if (Main().dRate_ > 1) {
                Main().dRate_ = std::max(std::exp(fought_round * -0.44 / Global().PlayerNum()) * Main().iRate_, 1.0);
            }
        } else {
            result += "本轮不进行玩家对战";
        }

        std::vector<PlayerID> list = alive_players;
        for (PlayerID pid : list) {
            if (Main().players_[pid].hp <= 0) {
                Main().alive_--;
                Main().player_out_[pid] = 1;
                Global().Boardcast() << At(pid) << "已被淘汰！";
                Global().Eliminate(pid);
            }
        }

        Global().Boardcast() << result;
        return StageErrCode::CHECKOUT;
    }

    const AreaCard card_;
    const std::string comb_html_;
};

class SelectStage : public SubGameStage<>
{
  public:
    SelectStage(MainStage& main_stage, const uint64_t round, std::vector<AreaCard>::iterator& it2_)
            : StageFsm(main_stage, "公共配牌阶段",
                MakeStageCommand(*this, "选择并放置砖块", &SelectStage::Select_, ArithChecker<uint32_t>(1, 11, "选卡"), ArithChecker<uint32_t>(1, 73, "位置")),
                MakeStageCommand(*this, "查看当前蜂巢情况和选卡卡池，可用于图片重发", &SelectStage::Info_, VoidChecker("赛况")))
            , comb_html_(main_stage.CombHtml("## 第 " + std::to_string(round) + " 回合[公共配牌阶段]"))
    {
        current_players.clear();
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (Main().player_out_[pid] == 0) {
                current_players.push_back(pid);
            }
        }
        for (int i = 0; i < current_players.size() + 1; i++) {
            if (it2_ == Main().cards2_.end()) {
                it2_ = Main().cards2_.begin();
            }
            const auto& card = *(it2_++);
            tmp_cards_.push_back(card);
        }
        std::seed_seq seed(Main().seed_str_.begin(), Main().seed_str_.end());
        std::mt19937 g(seed);
        SeededShuffle(current_players.begin(), current_players.end(), Main().g);

        std::sort(current_players.begin(), current_players.end(), [this](const PlayerID &p1, const PlayerID &p2) {
            if (Main().players_[p1].hp != Main().players_[p2].hp) {
                return Main().players_[p1].hp < Main().players_[p2].hp;
            }
            return Main().PlayerScore(p1) < Main().PlayerScore(p2);
        });
    }

    virtual void OnStageBegin() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (pid != current_players[0]) {
                Global().SetReady(pid);
            }
        }
        SendInfo(Global().BoardcastMsgSender());
        Global().Boardcast() << "请 " << At(current_players[0]) << " 选择";
        Global().StartTimer(GAME_OPTION(局时));
    }
  private:
    void HandleUnreadyPlayer_()
    {
        PlayerID pid = current_players[0];
        if (!Global().IsReady(pid) || Main().player_leave_[pid]) {
            auto& player = Main().players_[pid];
            auto sender = Global().Boardcast();
            const AreaCard card_ = tmp_cards_[0];
            Selected(1);
            if (card_.Type() == "") {
                if (player.comb_->IsAllFilled()) {
                    // 云顶模式盘面已满，放入位置1
                    const auto [point, extra_point] = player.comb_->Fill(1, card_);
                    sender << At(pid) << "因为超时未做选择，自动选择 1 号卡牌，因盘面已满填入位置 1";
                    if (point > 0) sender << "，意外收获 " + std::to_string(point) + " 点积分";
                    else if (point < 0) sender << "，损失 " + std::to_string(-point) + " 点积分";
                    if (GAME_OPTION(连线奖励)) {
                        if (extra_point > 0) sender << "和连线额外奖励 " + std::to_string(extra_point) + " 点积分";
                        else if (extra_point < 0) sender << "，损失奖励得分 " + std::to_string(-extra_point) + " 点积分";
                    }
                } else {
                    // 超时自动放入第一个空位
                    const auto [num, result] = player.comb_->SeqFill(card_);
                    sender << At(pid) << "因为超时未做选择，自动选择 1 号卡牌，并填入空位置 " << num;
                    if (result.score_ > 0) {
                        sender << "，意外收获 " << result.score_ << " 点积分";
                    }
                    if (GAME_OPTION(连线奖励) && result.extra_score_ > 0) {
                        sender << "和连线奖励分数 " << result.extra_score_ << " 点积分";
                    }
                }
            } else {
                sender << At(pid) << "因为超时未做选择，自动选择并跳过特殊道具";
            }
            Global().HookUnreadyPlayers();
        }
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_leave_[pid] = true;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        HandleUnreadyPlayer_();
        return StageOver();
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        HandleUnreadyPlayer_();
        return StageOver();
    }

    AtomReqErrCode StageOver()
    {
        if (current_players.empty()) {
            return StageErrCode::CHECKOUT;
        }
        comb_html_ = Main().CombHtml("## 第 " + std::to_string(Main().round_) + " 回合[公共配牌阶段]");
        SendInfo(Global().BoardcastMsgSender());
        Global().Boardcast() << "请 " << At(current_players[0]) << " 选择";
        Global().ClearReady(current_players[0]);
        Global().StartTimer(GAME_OPTION(局时));
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        auto& player = Main().players_[pid];
        const AreaCard card_ = tmp_cards_[0];
        Selected(1);
        if (card_.Type() == "") {
            if (player.comb_->IsAllFilled()) {
                player.comb_->Fill(1, card_);
            } else {
                player.comb_->SeqFill(card_);
            }
        }
        return StageErrCode::READY;
    }

    AtomReqErrCode Select_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t id, const uint32_t num)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 当前并非您的选卡回合";
            return StageErrCode::FAILED;
        }
        if (id < 1 || id > tmp_cards_.size()) {
            reply() << "[错误] 无效的选牌ID，请输入 1-" << tmp_cards_.size() << " 之间的数字";
            return StageErrCode::FAILED;
        }
        const AreaCard card_ = tmp_cards_[id - 1];
        auto& player = Main().players_[pid];
        if (player.comb_->IsWall(num) && !player.comb_->CanReplace(num) && card_.Type() != "erase" && card_.Type() != "reshape") {
            reply() << "[错误] 该位置为普通墙块，无法放置";
            return StageErrCode::FAILED;
        }
        if (card_.Type() == "erase" && !player.comb_->IsWall(num) && !player.comb_->IsFilled(num)) {
            reply() << "[错误] 消除失败，该位置为空，试试其它位置吧";
            return StageErrCode::FAILED;
        }
        Selected(id);
        const auto [point, extra_point] = player.comb_->Fill(num, card_);
        auto sender = reply();
        if (card_.Type() == "erase") {
            sender << "消除位置 " << num << " 成功";
        } else if (card_.Type() == "reshape") {
            if (player.comb_->IsFilled(num)) {
                sender << "将位置 " << num << " 转化为癞子";
            } else {
                sender << "重塑位置 " << num << " 成功";
            }
        } else {
            sender << "选择 " << id << " 号砖块，设置数字 " << num << " 成功";
        }
        if (point > 0) sender << "，本次操作收获 " + std::to_string(point) + " 点积分";
        else if (point < 0) sender << "，本次操作损失 " + std::to_string(-point) + " 点积分";
        if (GAME_OPTION(连线奖励)) {
            if (extra_point > 0) sender << "，连线额外奖励 " + std::to_string(extra_point) + " 点积分";
            else if (extra_point < 0) sender << "，损失奖励得分 " + std::to_string(-extra_point) + " 点积分";
        }
        return StageErrCode::READY;
    }

    void Selected(const uint32_t id)
    {
        tmp_cards_.erase(tmp_cards_.begin() + id - 1);
        current_players.erase(current_players.begin());
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        SendInfo(reply);
        return StageErrCode::OK;
    }

    std::string SelectCardHtml_()
    {
        html::Table avatar_table(1, current_players.size() + 1);
        avatar_table.SetTableStyle("cellpadding=\"0\" cellspacing=\"5\"");
        avatar_table.Get(0, 0).SetContent("　<b>选卡顺序：</b>");
        for (int i = 0; i < current_players.size(); i++) {
            avatar_table.Get(0, i + 1).SetContent(Global().PlayerAvatar(current_players[i], 40));
        }
        html::Table card_table(1, tmp_cards_.size() * 2);
        card_table.SetTableStyle("align=\"center\" cellpadding=\"0\" cellspacing=\"0\" ");
        for (int i = 0; i < tmp_cards_.size(); i++) {
            card_table.Get(0, i * 2).SetContent(std::to_string(i + 1) + ".");
            card_table.Get(0, i * 2 + 1).SetContent(tmp_cards_[i].ToHtml(Global().ResourceDir()));
        }
        const std::string style = "<style>body{margin:0;}</style>" + GetStyle(Global().ResourceDir());
        return style + avatar_table.ToString() + card_table.ToString();
    }

    void SendInfo(MsgSenderBase& sender)
    {
        sender() << Markdown{comb_html_};
        sender() << Markdown(SelectCardHtml_(), 300);
    }

    std::vector<PlayerID> current_players;
    std::vector<AreaCard> tmp_cards_;
    std::string comb_html_;
};

void MainStage::NewStage_(SubStageFsmSetter& setter)
{
    if (GAME_OPTION(模式) == 1 && round_ % 7 == 0 && round_ != 0) {
        setter.Emplace<SelectStage>(*this, ++round_, it2_);
    } else {
        const auto& card = *(it_++);
        setter.Emplace<RoundStage>(*this, ++round_, card);
    }
    return;
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    if (GAME_OPTION(模式) == 1) {
        Global().Boardcast() << "[提示] 本局为「云顶」对战模式：砖块可被覆盖，第3回合起进行玩家对战，血量归零淘汰，剩余1人时游戏结束。"
                             << (GAME_OPTION(连线奖励) ? "" : "由于未开启连线奖励，血量调整为150。");
    }
    if (!GAME_OPTION(道具)) {
        Global().Boardcast() << "[提示] 本局未开启道具，整局游戏不会出现任何特殊道具";
    }
    NewStage_(setter);
}

void MainStage::NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    NewStage_(setter);
}

bool MainStage::hasBots()
{
    std::regex pattern(R"(机器人\d+号)");
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        if (std::regex_match(Global().PlayerName(pid), pattern)) {
            return true;
        }
    }
    return false;
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (!CheckGameOver_(players_) && it_ != cards_.end()) {
        NewStage_(setter);
        return;
    }
    
    if (it_ == cards_.end()) Global().Boardcast() << "卡池耗尽，游戏结束";

    if (GAME_OPTION(模式) == 1 && alive_ == 1) {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (player_out_[pid] == 0) {
                Global().Boardcast() << "游戏结束，恭喜胜者：" << At(pid) << "！";
            }
        }
    }

    Global().Boardcast() << Markdown(CombHtml("## 终局"));
    const std::string game_card[4] = {"经典", "癞子", "空气", "混乱"};
    if (GAME_OPTION(种子).empty()) {
        Global().Boardcast() << "【本局卡池】" + game_card[GAME_OPTION(卡池)] + (GAME_OPTION(道具) ? "" : "\n【特殊道具】关闭") + "\n随机数种子：" + seed_str_;
        // achievements
        auto max_player = std::max_element(players_.begin(), players_.end(), [](const Player& a, const Player& b) { return a.comb_->Score() < b.comb_->Score(); });
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (GAME_OPTION(模式) == 0) {
                if (GAME_OPTION(卡池) == 0) {
                    if (players_[pid].comb_->Score() >= 250) Global().Achieve(pid, Achievement::经典开篇);
                    if (players_[pid].comb_->Score() >= 300) Global().Achieve(pid, Achievement::不凡经典);
                    if (players_[pid].comb_->Score() >= 350) Global().Achieve(pid, Achievement::传世经典);
                } else if (GAME_OPTION(卡池) == 1) {
                    if (players_[pid].comb_->Score() >= 350) Global().Achieve(pid, Achievement::万象归一);
                } else if (GAME_OPTION(卡池) == 2) {
                    if (players_[pid].comb_->Score() >= 250) Global().Achieve(pid, Achievement::空灵之境);
                } else if (GAME_OPTION(卡池) == 3) {
                    if (players_[pid].comb_->Score() >= 250) Global().Achieve(pid, Achievement::乱象初现);
                    if (players_[pid].comb_->Score() >= 300) Global().Achieve(pid, Achievement::破碎旅程);
                    if (players_[pid].comb_->Score() >= 350) Global().Achieve(pid, Achievement::混乱主宰);
                }
            }
            if ((GAME_OPTION(卡池) == 2 || GAME_OPTION(卡池) == 3) && players_[pid].comb_->AirAllLine()) {
                Global().Achieve(pid, Achievement::空中楼阁);
            }
            if ((GAME_OPTION(卡池) == 1 || GAME_OPTION(卡池) == 3) && players_[pid].comb_->WildAll10() && players_[pid].comb_->Score() == max_player->comb_->Score()) {
                Global().Achieve(pid, Achievement::十全十美);
            }
            if (players_[pid].comb_->BreakLine() == 2) Global().Achieve(pid, Achievement::蜂巢开拓者);
            if (players_[pid].comb_->BreakLine() == 1) Global().Achieve(pid, Achievement::蜂巢编织者);
            if (players_[pid].comb_->BreakLine() == 0) Global().Achieve(pid, Achievement::蜂巢完美者);
            if (players_[pid].comb_->Length9()) Global().Achieve(pid, Achievement::贯穿星河);
        }
    } else {
        Global().Boardcast() << "【本局卡池】" + game_card[GAME_OPTION(卡池)] + (GAME_OPTION(道具) ? "" : "\n【特殊道具】关闭") + "\n自定义种子：" + seed_str_;
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

