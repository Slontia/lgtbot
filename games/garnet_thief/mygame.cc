// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties {
    .name_ = "石榴石窃贼", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "选择不同的社会身份，通过博弈与欺骗争夺分数",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 1; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const MutableGenericOptions k_default_generic_options{
    .is_formal_{0},
};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < 3) {
        reply() << "该游戏至少 3 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 8;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况"))),
        round_(0),
        player_scores_(Global().PlayerNum(), 0),
        player_chips_(Global().PlayerNum(), 2),
        player_last_chips_(Global().PlayerNum(), 2),
        player_declare_(Global().PlayerNum(), ' '),
        player_select_(Global().PlayerNum(), ' ') {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    int round_;
    std::vector<int64_t> player_scores_;
    
    vector<int64_t> player_chips_;      // 玩家Chips
    vector<int64_t> player_last_chips_; // 上回合Chips
    vector<char> player_declare_;       // 玩家声明
    vector<char> player_select_;        // 玩家选择

    string T_Board = "";   // 表头
    string Board = "";   // 赛况

    const string clip_color = "9CCAF0";     // Clip底色
    const string declare_color = "FFEBA3";  // 声明身份颜色
    const string win_color = "BAFFA8";      // 加分颜色
    const string lose_color = "FFA07A";     // 扣分颜色

    const int image_width = Global().PlayerNum() < 8 ? Global().PlayerNum() * 80 + 100 : (Global().PlayerNum() < 16 ? Global().PlayerNum() * 70 + 50 : Global().PlayerNum() * 60 + 40);

    int Alive_() const { return std::count_if(player_chips_.begin(), player_chips_.end(), [](const auto& chips){ return chips > 0; }); }

    string GetName(string x);
    string GetStatusBoard();

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        string status_Board = GetStatusBoard();
        reply() << Markdown(T_Board + status_Board + Board + "</table>", image_width);
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        srand((unsigned int)time(NULL));
        for (int i = 0; i < Global().PlayerNum(); i++) {
            player_chips_[i] = player_last_chips_[i] = GAME_OPTION(Chips);
        }

        T_Board += "<table><tr>";
        for (int i = 0; i < Global().PlayerNum(); i++) {
            T_Board += "<th>" + to_string(i + 1) + " 号： " + GetName(Global().PlayerName(i)) + "　</th>";
            if (i % 4 == 3) T_Board += "</tr><tr>";
        }
        T_Board += "</tr><br>";

        T_Board += "<table style=\"text-align:center\"><tbody>";
        T_Board += "<tr bgcolor=\"#FFE4C4\"><th style=\"width:70px;\">序号</th>";
        for (int i = 0; i < Global().PlayerNum(); i++) {
            T_Board += "<th style=\"width:60px;\">";
            T_Board += to_string(i + 1) + " 号";
            T_Board += "</th>";
        }
        T_Board += "</tr>";

        string status_Board = GetStatusBoard();

        string PreBoard = "";
        PreBoard += "本局玩家序号如下：\n";
        for (int i = 0; i < Global().PlayerNum(); i++) {
            PreBoard += to_string(i + 1) + " 号：" + Global().PlayerName(i);
            if (i != (int)Global().PlayerNum() - 1) {
                PreBoard += "\n";
            }
        }

        Global().Boardcast() << PreBoard;
        Global().Boardcast() << Markdown(T_Board + status_Board + "</table>", image_width);
        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        bool chips_all_zero = all_of(player_chips_.begin(), player_chips_.end(), [](int64_t c) { return c == 0; });
        if ((++round_) <= GAME_OPTION(回合数) && !chips_all_zero) {
            setter.Emplace<RoundStage>(*this, round_);
            return;
        }
        if (chips_all_zero) {
            Global().Boardcast() << "所有玩家都被淘汰，游戏结束！";
        }
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            player_scores_[pid] = player_chips_[pid];
        }
    }
};

class DeclareStage;
class SelectStage;

class RoundStage : public SubGameStage<DeclareStage, SelectStage>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第" + std::to_string(round) + "回合",
            MakeStageCommand(*this, "【测试功能】向其他玩家发送私信消息", &RoundStage::SendMsg_,
                ArithChecker<uint32_t>(1, main_stage.player_scores_.size(), "序号"), RepeatableChecker<BasicChecker<string>>("私信内容", "私信内容")))
    {}

    void calc();

  private:
    CompReqErrCode SendMsg_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const uint32_t target, const vector<string> messages)
    {
        if (pid == target - 1) {
            reply() << "[错误] 不能向自己发送私信。";
            return StageErrCode::FAILED;
        }
        if (Main().player_chips_[target - 1] <= 0) {
            reply() << "[错误] 不能向淘汰的玩家发送私信。";
            return StageErrCode::FAILED;
        }
        if (is_public) {
            reply() << "[错误] 请私信执行此行动。";
            return StageErrCode::FAILED;
        }
        ostringstream oss;
        for (size_t i = 0; i < messages.size(); ++i) {
            if (i > 0) oss << " ";
            oss << messages[i];
        }
        string msg = oss.str();
        Global().Tell(PlayerID(target - 1)) << "【游戏消息《石榴石窃贼》】\n"
                                            << "收到来自 [" << (pid + 1) << "号]" << Global().PlayerName(pid) << " 的私信，内容：\n"
                                            << msg;
        reply() << "向 [" << target << "号]" << Global().PlayerName(target - 1) << " 发送私信成功";
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        setter.Emplace<DeclareStage>(Main());
    }

    void NextStageFsm(DeclareStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        string status_Board = Main().GetStatusBoard();

        string b = "<tr bgcolor=\"" + Main().declare_color + "\"><td style='border-top:1px solid black;'>R" + to_string(Main().round_) + "声明</td>";
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            b += "<td style='border-top:1px solid black;'>";
            if (Main().player_declare_[pid] == 'M') b += "黑手党";
            else if (Main().player_declare_[pid] == 'C') b += "卡特尔";
            else if (Main().player_declare_[pid] == 'P') b += "警察";
            else if (Main().player_declare_[pid] == 'B') b += "乞丐";
            else b += " ";
            b += "</td>";
        }
        b += "</tr>";
        Main().Board += b;

        Global().Boardcast() << Markdown(Main().T_Board + status_Board + Main().Board + "</table>", Main().image_width);
        setter.Emplace<SelectStage>(Main());
    }

    void NextStageFsm(SelectStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        RoundStage::calc();
    }
};

const map<string, char> role_map = {
    {"黑手党", 'M'}, {"M", 'M'},
	{"卡特尔", 'C'}, {"C", 'C'},
	{"警察", 'P'}, {"P", 'P'},
	{"乞丐", 'B'}, {"B", 'B'},
};

class DeclareStage : public SubGameStage<>
{
   public:
    DeclareStage(MainStage& main_stage)
            : StageFsm(main_stage, "声明阶段" ,
                MakeStageCommand(*this, "选择声明的身份", &DeclareStage::DeclareRole_, AlterChecker<char>(role_map)))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请所有玩家私信选择【声明】的身份，时限 " << GAME_OPTION(时限) << " 秒：\n"
                             << "黑手党(M) / 卡特尔(C) / 警察(P) / 乞丐(B)";
        if (Main().round_ == 1) {
            Global().Boardcast() << "【允许私信】此游戏允许在进行中和其他玩家进行私信沟通，进行合作或协商策略\n"
                                 << "- 测试指令：「<序号> <私信内容>」直接向其他玩家发送私信消息";
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode DeclareRole_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const char role)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行声明。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过声明了。";
            return StageErrCode::FAILED;
        }
        Main().player_declare_[pid] = role;
        reply() << "声明身份成功";
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (!Global().IsReady(pid)) {
                Main().player_chips_[pid] = 0;
                Main().player_declare_[pid] = ' ';
                Main().player_select_[pid] = ' ';
                Global().Eliminate(pid);
            }
        }
        Global().Boardcast() << "有玩家超时仍未行动，已被淘汰";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_chips_[pid] = 0;
        Main().player_declare_[pid] = ' ';
        Main().player_select_[pid] = ' ';
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        char roles[4] = {'M', 'C', 'P', 'B'};
        Main().player_declare_[pid] = roles[rand() % 4];
        return StageErrCode::READY;
    }
};

class SelectStage : public SubGameStage<>
{
   public:
    SelectStage(MainStage& main_stage)
            : StageFsm(main_stage, "提交阶段",
                MakeStageCommand(*this, "选择真实提交的身份", &SelectStage::SelectRole_, AlterChecker<char>(role_map)))
    {}

    virtual void OnStageBegin() override
    {
        Global().Boardcast() << "请所有玩家私信选择【真实】的身份，时限 " << GAME_OPTION(时限) << " 秒：\n"
                             << "黑手党(M) / 卡特尔(C) / 警察(P) / 乞丐(B)";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode SelectRole_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const char role)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行提交。";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您本回合已经进行过提交了。";
            return StageErrCode::FAILED;
        }
        Main().player_select_[pid] = role;
        reply() << "提交身份成功";
        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (int pid = 0; pid < Global().PlayerNum(); pid++) {
            if (!Global().IsReady(pid)) {
                Main().player_chips_[pid] = 0;
                Main().player_declare_[pid] = ' ';
                Main().player_select_[pid] = ' ';
                Global().Eliminate(pid);
            }
        }
        Global().Boardcast() << "有玩家超时仍未行动，已被淘汰";
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().player_chips_[pid] = 0;
        Main().player_declare_[pid] = ' ';
        Main().player_select_[pid] = ' ';
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        char roles[4] = {'M', 'C', 'P', 'B'};
        if (rand() % 3 == 0) {
            Main().player_select_[pid] = roles[rand() % 4];
        } else {
            Main().player_select_[pid] = Main().player_declare_[pid];
        }
        return StageErrCode::READY;
    }
};

string MainStage::GetName(std::string x) {
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

string MainStage::GetStatusBoard() {
    string status_Board = "";
    status_Board += "<tr bgcolor=\"" + clip_color + "\"><th>Chips</th>";
    for (int i = 0; i < Global().PlayerNum(); i++) {
        status_Board += "<td>";
        if (player_chips_[i] > 0) {
            status_Board += to_string(player_chips_[i]);
            if (player_chips_[i] > player_last_chips_[i]) {
                status_Board += Global().PlayerNum() >= 16 ? "<br>" : HTML_ESCAPE_SPACE;
                status_Board += "<font color=\"#1C8A3B\">(+" + to_string(player_chips_[i] - player_last_chips_[i]) + ")</font>";
            } else if (player_chips_[i] < player_last_chips_[i]) {
                status_Board += Global().PlayerNum() >= 16 ? "<br>" : HTML_ESCAPE_SPACE;
                status_Board += "<font color=\"#FF0000\">(-" + to_string(player_last_chips_[i] - player_chips_[i]) + ")</font>";
            }
        } else {
            status_Board += "<font color=\"#FF0000\">" + to_string(player_chips_[i]) + "</font>";
        }
        status_Board += "</td>";
    }
    status_Board += "</tr>";
    return status_Board;
}

void RoundStage::calc() {
    int N = Main().Alive_() > 3 ? Main().Alive_() / 2 : 2;

    int M_count = 0, C_count = 0, P_count = 0, B_count = 0;
    for (int pid = 0; pid < Global().PlayerNum(); pid++) {
        char s = Main().player_select_[pid];
        if (s == 'M') M_count++;
        else if (s == 'C') C_count++;
        else if (s == 'P') P_count++;
        else if (s == 'B') B_count++;
    }

    vector<int64_t> gain(Global().PlayerNum(), 0);
    char primary_role = ' ';
    int primary_count = 0;
    if (M_count > C_count) {
        primary_role = 'M';
        primary_count = M_count;
    } else if (C_count > M_count) {
        primary_role = 'C';
        primary_count = C_count;
    } else {
        primary_role = 'P';
        primary_count = P_count;
    }

    int remaining = N;
    if (primary_count > 0 && primary_role != ' ') {
        int per = N / primary_count;
        if (per > 0) {
            for (int pid = 0; pid < Global().PlayerNum(); pid++) {
                if (Main().player_select_[pid] == primary_role) {
                    gain[pid] += per;
                }
            }
        }
        int used = per * primary_count;
        remaining = N - used;
    } else {
        remaining = N;
    }

    if (remaining > 0 && B_count > 0) {
        int perB = remaining / B_count;
        if (perB > 0) {
            for (int pid = 0; pid < Global().PlayerNum(); pid++) {
                if (Main().player_select_[pid] == 'B') {
                    gain[pid] += perB;
                }
            }
        }
    }

    for (int pid = 0; pid < Global().PlayerNum(); pid++) {
        if (gain[pid] > 0) {
            Main().player_chips_[pid] += gain[pid];
            // 新增：身份不同且获得分数，额外+1
            if (Main().player_declare_[pid] != Main().player_select_[pid]) {
                Main().player_chips_[pid] += 1;
            }
        }
        if (gain[pid] == 0 && Main().player_chips_[pid] > 0 && Main().player_declare_[pid] != Main().player_select_[pid]) {
            Main().player_chips_[pid] -= 1;
        }
    }

    string status_Board = Main().GetStatusBoard();

    string b = "<tr><td>R" + to_string(Main().round_) + "提交</td>";
    for (int pid = 0; pid < Global().PlayerNum(); pid++) {
        if (Main().player_chips_[pid] > Main().player_last_chips_[pid]) {
            b += "<td bgcolor=\"" + Main().win_color + "\">";
        } else if (Main().player_chips_[pid] < Main().player_last_chips_[pid]) {
            b += "<td bgcolor=\"" + Main().lose_color + "\">" ;
        } else {
            b += "<td>";
        }
        if (Main().player_select_[pid] == 'M') b += "黑手党";
        else if (Main().player_select_[pid] == 'C') b += "卡特尔";
        else if (Main().player_select_[pid] == 'P') b += "警察";
        else if (Main().player_select_[pid] == 'B') b += "乞丐";
        else b += " ";
        b += "</td>";
    }
    b += "</tr>";
    Main().Board += b;

    Global().Boardcast() << Markdown(Main().T_Board + status_Board + Main().Board + "</table>", Main().image_width);

    for (int pid = 0; pid < Global().PlayerNum(); pid++) {
        Main().player_last_chips_[pid] = Main().player_chips_[pid];
        if (Main().player_chips_[pid] <= 0) {
            Main().player_declare_[pid] = ' ';
            Main().player_select_[pid] = ' ';
            Global().Eliminate(pid);
        }
    }
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

