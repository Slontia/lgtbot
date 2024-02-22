// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/game_stage.h"
#include "utility/html.h"
#include "board.h"

using namespace std;

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = GameStage<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = GameStage<void, SubStages...>;

const std::string k_game_name = "大海战"; // the game name which should be unique among all the games
const uint64_t k_max_player = 2; // 0 indicates no max-player limits
const uint64_t k_multiple = 1; // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
const std::string k_developer = "铁蛋";
const std::string k_description = "在地图上布置飞机，并击落对手的飞机";
const std::vector<RuleCommand> k_rule_commands = {};

std::string GameOption::StatusInfo() const { return ""; }

bool GameOption::ToValid(MsgSenderBase& reply)
{
    if (PlayerNum() != 2) {
        reply() << "该游戏为双人游戏，必须为2人参加，当前玩家数为" << PlayerNum();
        return false;
    }
    const int planeLimit[8] = {3, 3, 4, 5, 6, 6, 7, 8};
    if (GET_VALUE(飞机) > planeLimit[GET_VALUE(边长) - 8] && !GET_VALUE(重叠)) {
        GET_VALUE(飞机) = planeLimit[GET_VALUE(边长) - 8];
        reply() << "[警告] 边长为 " + to_string(GET_VALUE(边长)) + " 的地图最多可设置 " + to_string(planeLimit[GET_VALUE(边长) - 8]) + " 架不重叠的飞机，飞机数已自动调整为 " + to_string(planeLimit[GET_VALUE(边长) - 8]) + "！";
        return true;
    }
    if (GET_VALUE(飞机) >= 5 && GET_VALUE(放置时限) == 300 && GET_VALUE(进攻时限) == 120) {
        GET_VALUE(放置时限) = 600;
        GET_VALUE(进攻时限) = 150;
        reply() << "[提示] 本局飞机数为 " + to_string(GET_VALUE(飞机)) + "，已增加长考时间。";
    }
    return true;
}

uint64_t GameOption::BestPlayerNum() const { return 2; }

// ========== GAME STAGES ==========

class PrepareStage;
class AttackStage;

class MainStage : public MainGameStage<PrepareStage, AttackStage>
{
  public:
    MainStage(const GameOption& option, MatchBase& match)
        : GameStage(option, match)
        , round_(0)
        , player_scores_(option.PlayerNum(), 0)
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

    virtual VariantSubStage OnStageBegin() override
    {
        // 初始化玩家配置参数
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            board[pid].sizeX = board[pid].sizeY = GET_OPTION_VALUE(option(), 边长);
            board[pid].planeNum = GET_OPTION_VALUE(option(), 飞机);
            board[pid].MapName = PlayerName(pid);
            if (board[pid].MapName[0] == '<') {
                board[pid].MapName = board[pid].MapName.substr(1, board[pid].MapName.size() - 2);
            }
            board[pid].alive = 0;
            board[pid].prepare = 1;
            board[pid].firstX = board[pid].firstY = -1;
            attack_count[pid] = timeout[pid] = 0;
        }
        // 初始化BOSS战配置
        if (PlayerName(1) == "机器人0号") {
            board[0].MapName = "<div>挑战者</div>" + board[0].MapName;
            if (GET_OPTION_VALUE(option(), BOSS挑战) == 1) {
                board[0].sizeX = board[0].sizeY = board[1].sizeX = board[1].sizeY = 14;
                board[0].planeNum = 3;
                board[1].planeNum = 6;
                Boardcast() << "[提示] 初始化预设BOSS配置成功";
            }
        }
        board[0].InitializeMap();
        board[1].InitializeMap();
        
        // 随机生成侦察点
        srand((unsigned int)time(NULL));
        int count = 0, X, Y;
        int investigate = GET_OPTION_VALUE(option(), 侦察);
        if (investigate == 100) {
            investigate = rand() % (GET_OPTION_VALUE(option(), 边长) - 5) + 4;
        }
        while (count < investigate) {
            X = rand() % board[0].sizeX + 1;
            Y = rand() % board[0].sizeY + 1;
            if (board[0].map[X][Y][0] == 0) {
                board[0].map[X][Y][0] = board[1].map[X][Y][0] = 2;
                count++;
            }
        }
        return std::make_unique<PrepareStage>(*this);
    }

    virtual VariantSubStage NextSubStage(PrepareStage& sub_stage, const CheckoutReason reason) override
    {
        // 超时直接结束游戏
        if (timeout[0] || timeout[1]) {
            if (timeout[0]) player_scores_[0] = -1;
            if (timeout[1]) player_scores_[1] = -1;
            Boardcast() << Markdown(GetAllMap(1, 1, 0));
            return {};
        }
        // UI从转为要害显示
        board[0].prepare = 0;
        board[1].prepare = 0;

        return std::make_unique<AttackStage>(*this, ++round_);
    }

    virtual VariantSubStage NextSubStage(AttackStage& sub_stage, const CheckoutReason reason) override
    {
        if (board[0].alive && board[1].alive && !timeout[0] && !timeout[1]) {
            attack_count[0] = attack_count[1] = 0;
            return std::make_unique<AttackStage>(*this, ++round_);
        }

        if (timeout[0]) player_scores_[0] = -1;
        if (timeout[1]) player_scores_[1] = -1;

        if (board[0].alive == 0 && board[1].alive == 0) {
            Boardcast() << "游戏结束，所有飞机都被击落";
            if (attack_count[0] < attack_count[1]) {
                Boardcast() << "根据双方最后一回合的行动次数，判定 "<< At(PlayerID(0)) << " 取得胜利！";
                player_scores_[0] = 1;
            } else if (attack_count[0] > attack_count[1]) {
                Boardcast() << "根据双方最后一回合的行动次数，判定 "<< At(PlayerID(1)) << " 取得胜利！";
                player_scores_[1] = 1;
            } else {
                Boardcast() << "根据双方最后一回合的行动次数，游戏平局！";
                player_scores_[0] = player_scores_[1] = 1;
            }
        } else if (board[0].alive == 0) {
            Boardcast() << "游戏结束，"<< At(PlayerID(1)) << " 取得胜利！";
            player_scores_[1] = 1;
        } else if (board[1].alive == 0) {
            Boardcast() << "游戏结束，"<< At(PlayerID(0)) << " 取得胜利！";
            player_scores_[0] = 1;
        }

        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            for(int i = 1; i <= board[pid].sizeX; i++) {
                for(int j = 1; j <= board[pid].sizeY; j++) {
                    board[pid].this_turn[i][j] = 0;
                }
            }
        }
        Boardcast() << Markdown(GetAllMap(1, 1, 0));

        if (round_ == 1 && GET_OPTION_VALUE(option(), 飞机) >= 3) {
            for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
                if (player_scores_[pid] == 1 && player_scores_[!pid] == 0) {
                    if (attack_count[pid] == GET_OPTION_VALUE(option(), 飞机)) {
                        global_info().Achieve(pid, Achievement::超级一回杀);
                    } else {
                        global_info().Achieve(pid, Achievement::一回杀);
                    }
                }
            }
        }

        return {};
    }

};

class PrepareStage : public SubGameStage<>
{
   public:
    PrepareStage(MainStage& main_stage)
            : GameStage(main_stage, "放置阶段",
                    MakeStageCommand("查看当前飞机布置情况", &PrepareStage::Info_, VoidChecker("赛况")),
                    MakeStageCommand("放置飞机（飞机头坐标+方向）", &PrepareStage::Add_,
                        AnyArg("飞机头坐标", "C5"), AlterChecker<int>({{"上", 1}, {"下", 2}, {"左", 3}, {"右", 4}})),
                    MakeStageCommand("移除飞机（移除+飞机头坐标）", &PrepareStage::Remove_,
                        VoidChecker("移除"), AnyArg("飞机头坐标", "C5")),
					MakeStageCommand("清空地图", &PrepareStage::RemoveALL_, VoidChecker("清空")),
                    MakeStageCommand("布置完成，结束准备阶段", &PrepareStage::Finish_, VoidChecker("确认")))
    {}

    virtual void OnStageBegin() override
    {
		Boardcast() << Markdown(main_stage().GetAllMap(0, 0, GET_OPTION_VALUE(option(), 要害)));
        Boardcast() << "请私信裁判放置飞机，时限 " << GET_OPTION_VALUE(option(), 放置时限) << " 秒";
        
        // 游戏开始时展示特殊规则
        if (GET_OPTION_VALUE(option(), 重叠)) {
            Boardcast() << "[特殊规则] 本局允许飞机重叠：飞机的机身可任意数量重叠，机头不可与机身重叠。";
        }
        if (GET_OPTION_VALUE(option(), 要害) == 1) {
            Boardcast() << "[特殊规则] 本局无“要害”提示：命中机头时，仅告知命中，不再提示命中要害，且不具有额外一回合。";
        }
        if (GET_OPTION_VALUE(option(), 要害) == 2) {
            Boardcast() << "[特殊规则] 本局仅首“要害”公开：每个玩家命中过1次机头以后，之后再次命中其他机头时，仅告知命中，不提示命中要害，且不具有额外一回合。";
        }

        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            Tell(pid) << Markdown(main_stage().board[pid].Getmap(1, GET_OPTION_VALUE(option(), 要害)));
            Tell(pid) << "请放置飞机，指令为「坐标 方向」，如：C5 上\n可通过「帮助」命令查看全部命令格式";
        }
        StartTimer(GET_OPTION_VALUE(option(), 放置时限));
    }

   private:
    AtomReqErrCode Add_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str, const int64_t direction)
    {
        if (is_public) {
            reply() << "[错误] 放置失败，请私信裁判";
            return StageErrCode::FAILED;
        }
		if (IsReady(pid)) {
            reply() << "[错误] 放置失败：您已经确认过了，请等待对手行动";
            return StageErrCode::FAILED;
        }
		if (main_stage().board[pid].alive == main_stage().board[pid].planeNum) {
			reply() << "[错误] 放置失败：飞机数量已达本局上限，请先使用「移除 坐标」命令移除飞机，或使用「清空」命令清除全部飞机";
			return StageErrCode::FAILED;
		}

        string result = main_stage().board[pid].AddPlane(str, direction, GET_OPTION_VALUE(option(), 重叠));
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }

        reply() << Markdown(main_stage().board[pid].Getmap(1, GET_OPTION_VALUE(option(), 要害)));
		if (main_stage().board[pid].alive < main_stage().board[pid].planeNum) {
			reply() << "放置成功！您还有 " + to_string(main_stage().board[pid].planeNum - main_stage().board[pid].alive) + " 架飞机等待放置，请继续行动";
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
        if (IsReady(pid)) {
            reply() << "[错误] 已经确认了就不能反悔了哦~";
            return StageErrCode::FAILED;
        }

        string result = main_stage().board[pid].RemovePlane(str);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }

        reply() << Markdown(main_stage().board[pid].Getmap(1, GET_OPTION_VALUE(option(), 要害)));
        reply() << "移除成功！";
        return StageErrCode::OK;
    }

	AtomReqErrCode RemoveALL_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 清空失败，请私信裁判";
            return StageErrCode::FAILED;
        }
        if (IsReady(pid)) {
            reply() << "[错误] 已经确认了就不能反悔了哦~";
            return StageErrCode::FAILED;
        }
        
        main_stage().board[pid].RemoveAllPlanes();
        
        reply() << Markdown(main_stage().board[pid].Getmap(1, GET_OPTION_VALUE(option(), 要害)));
        reply() << "清空成功！";
        return StageErrCode::OK;
    }

    AtomReqErrCode Finish_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (IsReady(pid)) {
            reply() << "[错误] 您已经确认过了，请等待对手确认行动";
            return StageErrCode::FAILED;
        }
		if (main_stage().board[pid].alive < main_stage().board[pid].planeNum) {
			reply() << "[错误] 您还有 " + to_string(main_stage().board[pid].planeNum - main_stage().board[pid].alive) + " 架飞机等待放置";
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
        reply() << Markdown(main_stage().board[pid].Getmap(1, GET_OPTION_VALUE(option(), 要害)));
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                if (main_stage().board[pid].alive < GET_OPTION_VALUE(option(), 飞机)) {
                    Boardcast() << At(PlayerID(pid)) << " 超时未完成布置。";
                    main_stage().timeout[pid] = 1;
                } else {
                    Tell(pid) << "您超时未行动，已自动确认布置";
                }
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Boardcast() << At(PlayerID(pid)) << " 退出游戏，游戏结束。";
        main_stage().timeout[pid] = 1;
        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (!IsReady(pid)) {
            int X, Y;
            string str;
            int direction, try_count = 0;
            while (main_stage().board[pid].alive < main_stage().board[pid].planeNum) {
                X = rand() % main_stage().board[pid].sizeX;
                Y = rand() % main_stage().board[pid].sizeY + 1;
                str = string(1, 'A' + X) + to_string(Y);
                direction = rand() % 4 + 1;
                string result = main_stage().board[pid].AddPlane(str, direction, GET_OPTION_VALUE(option(), 重叠));
                if (result == "OK") {
                    try_count = 0;
                }
                if (try_count++ > 1000) {
                    main_stage().board[pid].RemoveAllPlanes();
                }
            }
            Boardcast() << "BOSS已抵达战场，请根据BOSS技能选择合适的部署和战术！";
            Boardcast() << Markdown(BossIntro());
            return StageErrCode::READY;
        }
	    return StageErrCode::OK;
    }

};

class AttackStage : public SubGameStage<>
{
  public:
    AttackStage(MainStage& main_stage, const uint64_t round)
        : GameStage(main_stage, "第 " + std::to_string(round) + " 回合",
				MakeStageCommand("查看当前游戏进展情况", &AttackStage::Info_, VoidChecker("赛况")),
                MakeStageCommand("标记飞机（飞机头坐标+方向）", &AttackStage::AddMark_,
                        AnyArg("飞机头坐标", "C5"), AlterChecker<int>({{"上", 1}, {"下", 2}, {"左", 3}, {"右", 4}})),
				MakeStageCommand("清空地图上的所有标记", &AttackStage::RemoveALLMark_, VoidChecker("清空")),
                MakeStageCommand("攻击坐标", &AttackStage::Attack_, AnyArg("坐标", "A1")))
    {
    }

    // 剩余连发次数
    int repeated[2];

    virtual void OnStageBegin() override
    {
        if (main_stage().round_ == 1) {
            Boardcast() << "请选择坐标发射导弹，每次行动时限 " << GET_OPTION_VALUE(option(), 进攻时限) << " 秒";
		    Boardcast() << "可通过私信裁判行动或「赛况」命令来查看己方地图部署。\n可使用指令「坐标 方向」来标记飞机，如：C5 上\n用「清空」来清除全部标记";
        } else {
            Boardcast() << "请继续行动，每次行动时限 " << GET_OPTION_VALUE(option(), 进攻时限) << " 秒";
        }
        repeated[0] = repeated[1] = GET_OPTION_VALUE(option(), 连发);
        StartTimer(GET_OPTION_VALUE(option(), 进攻时限));
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        Boardcast() << Markdown(main_stage().GetAllMap(0, 0, GET_OPTION_VALUE(option(), 要害)));
        // 重置上回合打击位置
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            for(int i = 1; i <= main_stage().board[pid].sizeX; i++) {
                for(int j = 1; j <= main_stage().board[pid].sizeY; j++) {
                    main_stage().board[pid].this_turn[i][j] = 0;
                }
            }
        }

        return StageErrCode::CHECKOUT;
    }

  private:
    AtomReqErrCode Attack_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str)
    {
        if (IsReady(pid)) {
            reply() << "您本回合已行动完成，请等待对手操作";
            return StageErrCode::FAILED;
        }

        string result = main_stage().board[!pid].Attack(str);
        if (result != "0" && result != "1" && result != "2" ) {
            reply() << result;
            return StageErrCode::FAILED;
        }

        // 首要害坐标添加
        bool first_crucial = false;
        if (result == "2" && GET_OPTION_VALUE(option(), 要害) == 2 && main_stage().board[!pid].firstX == -1) {
            string coordinate = str;
            main_stage().board[!pid].CheckCoordinate(coordinate);
            main_stage().board[!pid].firstX = coordinate[0] - 'A' + 1;
            main_stage().board[!pid].firstY = coordinate[1] - '0'; 
            if (coordinate.length() == 3) {
                main_stage().board[!pid].firstY = (coordinate[1] - '0') * 10 + coordinate[2] - '0';
            }
            first_crucial = true;
        }

        reply() << Markdown(main_stage().GetAllMap(!pid && !is_public, pid && !is_public, GET_OPTION_VALUE(option(), 要害)));
        main_stage().attack_count[pid]++;
        if (result == "0")
        {
            repeated[pid]--;
            if (repeated[pid] == 0) {
                reply() << "导弹未击中任何飞机。\n您已经没有导弹了，本回合结束。";
                return StageErrCode::READY;
            } else {
                reply() << "导弹未击中任何飞机。\n您还有 " + to_string(repeated[pid]) + " 发导弹。";
                StartTimer(GET_OPTION_VALUE(option(), 进攻时限));
                return StageErrCode::OK;
            }
        }
        else if (result == "1" ||
                (result == "2" && GET_OPTION_VALUE(option(), 要害) == 1) ||
                (result == "2" && GET_OPTION_VALUE(option(), 要害) == 2 && !first_crucial))
        {
            if (main_stage().board[!pid].alive == 0 && GET_OPTION_VALUE(option(), 要害) > 0) {
                reply() << "导弹命中了飞机！\n对方的所有飞机都已被击落！";
                return StageErrCode::READY;
            }
            reply() << "导弹命中了飞机！\n本回合结束。";
            return StageErrCode::READY;
        }
        else if (result == "2")
        {
            if (main_stage().board[!pid].alive == 0) {
                reply() << "导弹直击飞机要害！！\n对方的所有飞机都已被击落！";
                return StageErrCode::READY;
            } else {
                if (GET_OPTION_VALUE(option(), 要害) == 2) {
                    reply() << "导弹直击飞机要害！！\n获得额外一回合行动机会，再次命中要害时将不会进行提示。";
                } else {
                    reply() << "导弹直击飞机要害！！\n获得额外一回合行动机会，导弹已重新填充。";
                }
                repeated[pid] = GET_OPTION_VALUE(option(), 连发);
                StartTimer(GET_OPTION_VALUE(option(), 进攻时限));
                return StageErrCode::OK;
            }
        }
        Boardcast() << "[错误] Empty Return";
        return StageErrCode::FAILED;
    }

    AtomReqErrCode AddMark_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const std::string& str, const int64_t direction)
    {
        string result = main_stage().board[!pid].AddMark(str, direction);
        if (result != "OK") {
            reply() << result;
            return StageErrCode::FAILED;
        }
        reply() << Markdown(main_stage().GetAllMap(!pid && !is_public, pid && !is_public, GET_OPTION_VALUE(option(), 要害))) << "标记成功！";
        return StageErrCode::OK;
    }

	AtomReqErrCode RemoveALLMark_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        main_stage().board[!pid].RemoveAllMark();
        reply() << Markdown(main_stage().GetAllMap(!pid && !is_public, pid && !is_public, GET_OPTION_VALUE(option(), 要害))) << "清空标记成功！";
        return StageErrCode::OK;
    }

	AtomReqErrCode Info_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << Markdown(main_stage().GetAllMap(0, 0, GET_OPTION_VALUE(option(), 要害)));
        } else {
            reply() << Markdown(main_stage().GetAllMap(!pid, pid, GET_OPTION_VALUE(option(), 要害)));
        }
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
            if (!IsReady(pid)) {
                Boardcast() << At(PlayerID(pid)) << " 行动超时。";
                main_stage().timeout[pid] = 1;
            }
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        main_stage().timeout[pid] = 1;
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (!IsReady(pid)) {
            if (main_stage().round_ == 18) {
                Boardcast() << "【BOSS】普通打击已升级，每回合最多发射 8 枚导弹";
            }
            BossNormalAttack(Boardcast(), main_stage().board, main_stage().round_, main_stage().attack_count);
            BossSkillAttack(Boardcast(), main_stage().board, GET_OPTION_VALUE(option(), 重叠), main_stage().timeout);
            if (main_stage().timeout[1] == 1) {
                SetReady(0);
            }
            Boardcast() << Markdown(main_stage().GetAllMap(0, 0, GET_OPTION_VALUE(option(), 要害)));
            return StageErrCode::READY;
        }
	    return StageErrCode::OK;
    }

};

} // namespace lgtbot

} // namespace game

} // namespace lgtbot

#include "game_framework/make_main_stage.h"
