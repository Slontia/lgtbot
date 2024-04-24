// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"
#include "board.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const std::string k_game_name = "大海战"; // the game name which should be unique among all the games
uint64_t MaxPlayerNum(const MyGameOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const MyGameOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "铁蛋";
const std::string k_description = "在地图上布置飞机，并击落对手的飞机";
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, MyGameOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << generic_options_readonly.PlayerNum();
        return false;
    }
    const int planeLimit[8] = {3, 3, 4, 5, 6, 6, 7, 8};
    if (GET_OPTION_VALUE(game_options, 飞机) > planeLimit[GET_OPTION_VALUE(game_options, 边长) - 8] && !GET_OPTION_VALUE(game_options, 重叠)) {
        GET_OPTION_VALUE(game_options, 飞机) = planeLimit[GET_OPTION_VALUE(game_options, 边长) - 8];
        reply() << "[警告] 边长为 " + to_string(GET_OPTION_VALUE(game_options, 边长)) + " 的地图最多可设置 " + to_string(planeLimit[GET_OPTION_VALUE(game_options, 边长) - 8]) + " 架不重叠的飞机，飞机数已自动调整为 " + to_string(planeLimit[GET_OPTION_VALUE(game_options, 边长) - 8]) + "！";
        return true;
    }
    if (GET_OPTION_VALUE(game_options, 飞机) >= 5 && GET_OPTION_VALUE(game_options, 放置时限) == 300 && GET_OPTION_VALUE(game_options, 进攻时限) == 120) {
        GET_OPTION_VALUE(game_options, 放置时限) = 600;
        GET_OPTION_VALUE(game_options, 进攻时限) = 150;
        reply() << "[提示] 本局飞机数为 " + to_string(GET_OPTION_VALUE(game_options, 飞机)) + "，已增加长考时间。";
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (MyGameOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class PrepareStage;
class AttackStage;

class MainStage : public MainGameStage<PrepareStage, AttackStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {
    }

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 地图
    Board board[2];
    // 回合数 
	int round_;
    // 回合攻击次数
    int attack_count[2];
    // 判定是否是超时淘汰
    int timeout[2];

	string GetAllMap(int show_0, int show_1, int crucial_mode)
	{
		string allmap = "<table style=\"text-align:center;margin:auto;\"><tr>";
		allmap += "<td>" + board[0].Getmap(show_0, crucial_mode) + "</td>";
		allmap += "<td>" + board[1].Getmap(show_1, crucial_mode) + "</td>";
		allmap += "</tr></table>";
        return allmap;
	}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override
    {
        // 初始化玩家配置参数
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board[pid].sizeX = board[pid].sizeY = GAME_OPTION(边长);
            board[pid].planeNum = GAME_OPTION(飞机);
            board[pid].MapName = Global().PlayerName(pid);
            if (board[pid].MapName[0] == '<') {
                board[pid].MapName = board[pid].MapName.substr(1, board[pid].MapName.size() - 2);
            }
            board[pid].alive = 0;
            board[pid].prepare = 1;
            board[pid].firstX = board[pid].firstY = -1;
            attack_count[pid] = timeout[pid] = 0;
        }
        // 初始化BOSS战配置
        if (Global().PlayerName(1) == "机器人0号") {
            board[0].MapName = "<div>挑战者</div>" + board[0].MapName;
            if (GAME_OPTION(BOSS挑战) == 1) {
                board[0].sizeX = board[0].sizeY = board[1].sizeX = board[1].sizeY = 14;
                board[0].planeNum = 3;
                board[1].planeNum = 6;
                Global().Boardcast() << "[提示] 初始化预设BOSS配置成功";
            }
        }
        board[0].InitializeMap();
        board[1].InitializeMap();
        
        // 随机生成侦察点
        srand((unsigned int)time(NULL));
        int count = 0, X, Y;
        int investigate = GAME_OPTION(侦察);
        if (investigate == 100) {
            investigate = rand() % 5 + (GAME_OPTION(边长) - 7);
        }
        while (count < investigate) {
            X = rand() % board[0].sizeX + 1;
            Y = rand() % board[0].sizeY + 1;
            if (board[0].map[X][Y][0] == 0) {
                board[0].map[X][Y][0] = board[1].map[X][Y][0] = 2;
                count++;
            }
        }
        setter.Emplace<PrepareStage>(*this);
    }

    virtual void NextStageFsm(PrepareStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        // 超时直接结束游戏
        if (timeout[0] || timeout[1]) {
            if (timeout[0]) player_scores_[0] = -1;
            if (timeout[1]) player_scores_[1] = -1;
            Global().Boardcast() << Markdown(GetAllMap(1, 1, 0));
            return;
        }
        // UI从转为要害显示
        board[0].prepare = 0;
        board[1].prepare = 0;

        // 展示初始地图
        Global().Boardcast() << Markdown(GetAllMap(0, 0, GAME_OPTION(要害)));

        setter.Emplace<AttackStage>(*this, ++round_);
    }

    virtual void NextStageFsm(AttackStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (board[0].alive && board[1].alive && !timeout[0] && !timeout[1]) {
            attack_count[0] = attack_count[1] = 0;
            setter.Emplace<AttackStage>(*this, ++round_);
            return;
        }

        if (timeout[0]) player_scores_[0] = -1;
        if (timeout[1]) player_scores_[1] = -1;

        if (board[0].alive == 0 && board[1].alive == 0) {
            Global().Boardcast() << "游戏结束，所有飞机都被击落";
            if (attack_count[0] < attack_count[1]) {
                Global().Boardcast() << "根据双方最后一回合的行动次数，判定 "<< At(PlayerID(0)) << " 取得胜利！";
                player_scores_[0] = 1;
            } else if (attack_count[0] > attack_count[1]) {
                Global().Boardcast() << "根据双方最后一回合的行动次数，判定 "<< At(PlayerID(1)) << " 取得胜利！";
                player_scores_[1] = 1;
            } else {
                Global().Boardcast() << "根据双方最后一回合的行动次数，游戏平局！";
                player_scores_[0] = player_scores_[1] = 1;
            }
        } else if (board[0].alive == 0) {
            Global().Boardcast() << "游戏结束，"<< At(PlayerID(1)) << " 取得胜利！";
            player_scores_[1] = 1;
        } else if (board[1].alive == 0) {
            Global().Boardcast() << "游戏结束，"<< At(PlayerID(0)) << " 取得胜利！";
            player_scores_[0] = 1;
        }

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            for(int i = 1; i <= board[pid].sizeX; i++) {
                for(int j = 1; j <= board[pid].sizeY; j++) {
                    board[pid].this_turn[i][j] = 0;
                }
            }
        }
        Global().Boardcast() << Markdown(GetAllMap(1, 1, 0));

        if (round_ == 1 && GAME_OPTION(飞机) >= 3) {
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (player_scores_[pid] == 1 && player_scores_[!pid] == 0) {
                    if (attack_count[pid] == GAME_OPTION(飞机)) {
                        Global().Achieve(pid, Achievement::超级一回杀);
                    } else {
                        Global().Achieve(pid, Achievement::一回杀);
                    }
                }
            }
        }
    }

};

class PrepareStage : public SubGameStage<>
{
   public:
    PrepareStage(MainStage& main_stage)
            : StageFsm(main_stage, "放置阶段",
                    MakeStageCommand(*this, "查看当前飞机布置情况", &PrepareStage::Info_, VoidChecker("赛况")),
                    MakeStageCommand(*this, "放置飞机（飞机头坐标+方向）", &PrepareStage::Add_,
                        AnyArg("飞机头坐标", "C5"), AlterChecker<int>({{"上", 1}, {"下", 2}, {"左", 3}, {"右", 4}})),
                    MakeStageCommand(*this, "移除飞机（移除+飞机头坐标）", &PrepareStage::Remove_,
                        VoidChecker("移除"), AnyArg("飞机头坐标", "C5")),
					MakeStageCommand(*this, "清空地图", &PrepareStage::RemoveALL_, VoidChecker("清空")),
                    MakeStageCommand(*this, "布置完成，结束准备阶段", &PrepareStage::Finish_, VoidChecker("确认")))
    {}

    virtual void OnStageBegin() override
    {
		Global().Boardcast() << Markdown(Main().GetAllMap(0, 0, GAME_OPTION(要害)));
        Global().Boardcast() << "请私信裁判放置飞机，时限 " << GAME_OPTION(放置时限) << " 秒";
        
        // 游戏开始时展示特殊规则
        if (GAME_OPTION(重叠)) {
            Global().Boardcast() << "[特殊规则] 本局允许飞机重叠：飞机的机身可任意数量重叠，机头不可与机身重叠。";
        }
        if (GAME_OPTION(要害) == 1) {
            Global().Boardcast() << "[特殊规则] 本局无“要害”提示：命中机头时，仅告知命中，不再提示命中要害，且不具有额外一回合。";
        }
        if (GAME_OPTION(要害) == 2) {
            Global().Boardcast() << "[特殊规则] 本局仅首“要害”公开：每个玩家命中过1次机头以后，之后再次命中其他机头时，仅告知命中，不提示命中要害，且不具有额外一回合。";
        }

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Global().Tell(pid) << Markdown(Main().board[pid].Getmap(1, GAME_OPTION(要害)));
            Global().Tell(pid) << "请放置飞机，指令为「坐标 方向」，如：C5 上\n可通过「帮助」命令查看全部命令格式";
        }
        Global().StartTimer(GAME_OPTION(放置时限));
    }

   private:
    AtomReqErrCode Add_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str, const int64_t direction)
    {
        if (is_public) {
            reply() << "[错误] 放置失败，请私信裁判";
            return StageErrCode::FAILED;
        }
		if (Global().IsReady(pid)) {
            reply() << "[错误] 放置失败：您已经确认过了，请等待对手行动";
            return StageErrCode::FAILED;
        }
		if (Main().board[pid].alive == Main().board[pid].planeNum) {
			reply() << "[错误] 放置失败：飞机数量已达本局上限，请先使用「移除 坐标」命令移除飞机，或使用「清空」命令清除全部飞机";
			return StageErrCode::FAILED;
		}

        string result = Main().board[pid].AddPlane(str, direction, GAME_OPTION(重叠));
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }

        reply() << Markdown(Main().board[pid].Getmap(1, GAME_OPTION(要害)));
		if (Main().board[pid].alive < Main().board[pid].planeNum) {
			reply() << "放置成功！您还有 " + to_string(Main().board[pid].planeNum - Main().board[pid].alive) + " 架飞机等待放置，请继续行动";
		} else {
			reply() << "放置成功！若您已准备完成，请使用「确认」命令结束行动";
		}
        return StageErrCode::OK;
    }

    AtomReqErrCode Remove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (is_public) {
            reply() << "[错误] 移除失败，请私信裁判";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 已经确认了就不能反悔了哦~";
            return StageErrCode::FAILED;
        }

        string result = Main().board[pid].RemovePlane(str);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }

        reply() << Markdown(Main().board[pid].Getmap(1, GAME_OPTION(要害)));
        reply() << "移除成功！";
        return StageErrCode::OK;
    }

	AtomReqErrCode RemoveALL_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 清空失败，请私信裁判";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 已经确认了就不能反悔了哦~";
            return StageErrCode::FAILED;
        }
        
        Main().board[pid].RemoveAllPlanes();
        
        reply() << Markdown(Main().board[pid].Getmap(1, GAME_OPTION(要害)));
        reply() << "清空成功！";
        return StageErrCode::OK;
    }

    AtomReqErrCode Finish_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (Global().IsReady(pid)) {
            reply() << "[错误] 您已经确认过了，请等待对手确认行动";
            return StageErrCode::FAILED;
        }
		if (Main().board[pid].alive < Main().board[pid].planeNum) {
			reply() << "[错误] 您还有 " + to_string(Main().board[pid].planeNum - Main().board[pid].alive) + " 架飞机等待放置";
            return StageErrCode::FAILED;
        }
        reply() << "确认成功！";
        return StageErrCode::READY;
    }

    AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "请私信裁判查看当前布置的地图";
            return StageErrCode::FAILED;
        }
        reply() << Markdown(Main().board[pid].Getmap(1, GAME_OPTION(要害)));
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                if (Main().board[pid].alive < GAME_OPTION(飞机)) {
                    Global().Boardcast() << At(PlayerID(pid)) << " 超时未完成布置。";
                    Main().timeout[pid] = 1;
                } else {
                    Global().Tell(pid) << "您超时未行动，已自动确认布置";
                }
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << At(PlayerID(pid)) << " 退出游戏，游戏结束。";
        Main().timeout[pid] = 1;
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (!Global().IsReady(pid)) {
            int X, Y;
            string str;
            int direction, try_count = 0;
            while (Main().board[pid].alive < Main().board[pid].planeNum) {
                X = rand() % Main().board[pid].sizeX;
                Y = rand() % Main().board[pid].sizeY + 1;
                str = string(1, 'A' + X) + to_string(Y);
                direction = rand() % 4 + 1;
                string result = Main().board[pid].AddPlane(str, direction, GAME_OPTION(重叠));
                if (result == "OK") {
                    try_count = 0;
                }
                if (try_count++ > 1000) {
                    Main().board[pid].RemoveAllPlanes();
                }
            }
            Global().Boardcast() << "BOSS已抵达战场，请根据BOSS技能选择合适的部署和战术！";
            Global().Boardcast() << Markdown(BossIntro());
            return StageErrCode::READY;
        }
	    return StageErrCode::OK;
    }

};

class AttackStage : public SubGameStage<>
{
  public:
    AttackStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
				MakeStageCommand(*this, "查看当前游戏进展情况", &AttackStage::Info_, VoidChecker("赛况")),
                MakeStageCommand(*this, "标记飞机（飞机头坐标+方向）", &AttackStage::AddMark_,
                        AnyArg("飞机头坐标", "C5"), AlterChecker<int>({{"上", 1}, {"下", 2}, {"左", 3}, {"右", 4}})),
				MakeStageCommand(*this, "清空地图上的所有标记", &AttackStage::RemoveALLMark_, VoidChecker("清空")),
                MakeStageCommand(*this, "攻击坐标", &AttackStage::Attack_, AnyArg("坐标", "A1")))
    {
    }

    // 剩余连发次数
    int repeated[2];

    virtual void OnStageBegin() override
    {
        if (Main().round_ == 1) {
            Global().Boardcast() << "请选择坐标发射导弹，每次行动时限 " << GAME_OPTION(进攻时限) << " 秒";
		    Global().Boardcast() << "可通过私信裁判行动或「赛况」命令来查看己方地图部署。\n可使用指令「坐标 方向」来标记飞机，如：C5 上\n用「清空」来清除全部标记";
        } else {
            Global().Boardcast() << "请继续行动，每次行动时限 " << GAME_OPTION(进攻时限) << " 秒";
        }
        repeated[0] = repeated[1] = GAME_OPTION(连发);
        Global().StartTimer(GAME_OPTION(进攻时限));
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Global().Boardcast() << Markdown(Main().GetAllMap(0, 0, GAME_OPTION(要害)));
        // 重置上回合打击位置
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            for(int i = 1; i <= Main().board[pid].sizeX; i++) {
                for(int j = 1; j <= Main().board[pid].sizeY; j++) {
                    Main().board[pid].this_turn[i][j] = 0;
                }
            }
        }

        return StageErrCode::CHECKOUT;
    }

  private:
    AtomReqErrCode Attack_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (Global().IsReady(pid)) {
            reply() << "您本回合已行动完成，请等待对手操作";
            return StageErrCode::FAILED;
        }

        string result = Main().board[!pid].Attack(str);
        if (result != "0" && result != "1" && result != "2" ) {
            reply() << result;
            return StageErrCode::FAILED;
        }

        // 首要害坐标添加
        bool first_crucial = false;
        if (result == "2" && GAME_OPTION(要害) == 2 && Main().board[!pid].firstX == -1) {
            string coordinate = str;
            Main().board[!pid].CheckCoordinate(coordinate);
            Main().board[!pid].firstX = coordinate[0] - 'A' + 1;
            Main().board[!pid].firstY = coordinate[1] - '0'; 
            if (coordinate.length() == 3) {
                Main().board[!pid].firstY = (coordinate[1] - '0') * 10 + coordinate[2] - '0';
            }
            first_crucial = true;
        }

        reply() << Markdown(Main().GetAllMap(!pid && !is_public, pid && !is_public, GAME_OPTION(要害)));
        Main().attack_count[pid]++;
        if (result == "0")
        {
            repeated[pid]--;
            if (repeated[pid] == 0) {
                reply() << "导弹未击中任何飞机。\n您已经没有导弹了，本回合结束。";
                return StageErrCode::READY;
            } else {
                reply() << "导弹未击中任何飞机。\n您还有 " + to_string(repeated[pid]) + " 发导弹。";
                Global().StartTimer(GAME_OPTION(进攻时限));
                return StageErrCode::OK;
            }
        }
        else if (result == "1" ||
                (result == "2" && GAME_OPTION(要害) == 1) ||
                (result == "2" && GAME_OPTION(要害) == 2 && !first_crucial))
        {
            if (Main().board[!pid].alive == 0 && GAME_OPTION(要害) > 0) {
                reply() << "导弹命中了飞机！\n对方的所有飞机都已被击落！";
                return StageErrCode::READY;
            }
            reply() << "导弹命中了飞机！\n本回合结束。";
            return StageErrCode::READY;
        }
        else if (result == "2")
        {
            if (Main().board[!pid].alive == 0) {
                reply() << "导弹直击飞机要害！！\n对方的所有飞机都已被击落！";
                return StageErrCode::READY;
            } else {
                if (GAME_OPTION(要害) == 2) {
                    reply() << "导弹直击飞机要害！！\n获得额外一回合行动机会，再次命中要害时将不会进行提示。";
                } else {
                    reply() << "导弹直击飞机要害！！\n获得额外一回合行动机会，导弹已重新填充。";
                }
                repeated[pid] = GAME_OPTION(连发);
                Global().StartTimer(GAME_OPTION(进攻时限));
                return StageErrCode::OK;
            }
        }
        Global().Boardcast() << "[错误] Empty Return";
        return StageErrCode::FAILED;
    }

    AtomReqErrCode AddMark_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str, const int64_t direction)
    {
        string result = Main().board[!pid].AddMark(str, direction);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }
        reply() << Markdown(Main().GetAllMap(!pid && !is_public, pid && !is_public, GAME_OPTION(要害))) << "标记成功！";
        return StageErrCode::OK;
    }

	AtomReqErrCode RemoveALLMark_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        Main().board[!pid].RemoveAllMark();
        reply() << Markdown(Main().GetAllMap(!pid && !is_public, pid && !is_public, GAME_OPTION(要害))) << "清空标记成功！";
        return StageErrCode::OK;
    }

	AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown(Main().GetAllMap(0, 0, GAME_OPTION(要害)));
        } else {
            reply() << Markdown(Main().GetAllMap(!pid, pid, GAME_OPTION(要害)));
        }
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Global().IsReady(pid)) {
                Global().Boardcast() << At(PlayerID(pid)) << " 行动超时。";
                Main().timeout[pid] = 1;
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().timeout[pid] = 1;
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (!Global().IsReady(pid)) {
            if (Main().round_ == 18) {
                Global().Boardcast() << "【BOSS】普通打击已升级，每回合最多发射 8 枚导弹";
            }
            Global().Boardcast() << BossNormalAttack(Main().board, Main().round_, Main().attack_count);
            string skillinfo = BossSkillAttack(Main().board, GAME_OPTION(重叠), Main().timeout);
            if (skillinfo != "") {
                Global().Boardcast() << skillinfo;
            }
            if (Main().timeout[1] == 1) {
                Global().SetReady(0);
            }
            Global().Boardcast() << Markdown(Main().GetAllMap(0, 0, GAME_OPTION(要害)));
            return StageErrCode::READY;
        }
	    return StageErrCode::OK;
    }

};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

