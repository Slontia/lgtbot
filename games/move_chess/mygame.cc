// 睦月
//
// 2023.1.30

#include <array>
#include <functional>
#include <memory>
#include <set>

#include <stdlib.h>

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
    .name_ = "移子棋",// the game name which should be unique among all the games
    .developer_ = "睦月",
    .description_ = "通过移动棋子形成四连的棋类游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; }// 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; }
const MutableGenericOptions k_default_generic_options{
    .is_formal_{false},
};
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() != 2) {
        reply() << "该游戏必须 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 2;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GAME STAGES ==========

class RoundStage;

// 棋盘类
class Board
{
public:
	
	// 棋盘大小 
	int sizeX = 11, sizeY = 11;
	// 棋子 
    int chess[30][30] = {0};
	// 上一枚落子 
	int lastX1 = 0, lastY1 = 0, lastX2 = 0, lastY2 = 0;
	// 棋盘的markdown格式 
	string grid[30][30];
	
	// 将数字转为字符串 
	string str(int x)
	{
		return to_string(x);
	}
	
	// 图形界面
	string GetUI()
	{
		// 初始化 
		for(int i = 0; i <= sizeX + 2; i++)
			for(int j = 0; j <= sizeY + 2; j++)
				grid[i][j] = "";
		
		// 用来填充的字符串 
		string fill = "<font size=7>　</font><font size=2>　</font>";
		
		// 每一横排的数字 
		for(int i = 1; i <= sizeX; i++)
		{
			grid[i][0] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[i][0] += str(i);
			grid[i][0] += "</font></td>";
			
			grid[i][sizeY + 1] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[i][sizeY + 1] += str(i);
			grid[i][sizeY + 1] += "</font></td>";
			
			// 最右侧的填充格 
			grid[i][sizeY + 2] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[i][sizeY + 2] += fill;
			grid[i][sizeY + 2] += "</font></td>";
		}
		
		// 每一竖排的数字 
		for(int j = 1; j <= sizeY; j++)
		{
			grid[0][j] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[0][j] += char(j + 'A' - 1);
			grid[0][j] += "</font></td>";
			
			grid[sizeX + 1][j] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[sizeX + 1][j] += char(j + 'A' - 1);
			grid[sizeX + 1][j] += "</font></td>";
			
			// 最下方的填充格 
			grid[sizeX + 2][j] += "<td bgcolor=\"#FFFFFF\"><font size=7>";
			grid[sizeX + 2][j] += fill;
			grid[sizeX + 2][j] += "</font></td>";
		}
		
		// 边缘
		grid[0][0] = grid[0][sizeY + 1] = grid[0][sizeY + 2] = 
		grid[sizeX + 1][0] = grid[sizeX + 1][sizeY + 1] = grid[sizeX + 1][sizeY + 2] = 
		grid[sizeX + 2][0] = grid[sizeX + 2][sizeY + 1] = grid[sizeX + 2][sizeY + 2] = 
		"<td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td>";
		
		// 中间棋盘
		for(int i = 1; i <= sizeX; i++)
		{
	 		for(int j = 1; j <= sizeY; j++)
	 		{
			 	grid[i][j] += "<td bgcolor=\"";
			 	
			 	// 染色 
			 	string cr = "#97B0CE";
			 	if((i == lastX1 && j == lastY1) || (i == lastX2 && j == lastY2))
			 	{
					cr = "#A77AE0";
				}
			 	grid[i][j] += cr;
			 	
			 	// 棋子的大小略小一点 
			 	grid[i][j] += "\"><font size=6>";
			 	
			 	// 棋子
			 	string ch;
			 	if(chess[i][j] == 0)
				 	ch = fill;
			 	if(chess[i][j] == 1)
			 		ch = "<font color=\"#000000\">●</font>";
			 	if(chess[i][j] == 2)
				 	ch = "<font color=\"#E9E9E9\">●</font>";
			 	grid[i][j] += ch;
			 	
			 	grid[i][j] += "</font></td>";
			}
		}
		
		// 构造UI 
		string UI = "";
		UI += "<table style=\"text-align:center;margin:auto;\"><tbody>";
		
		// 为保持对称，开头要额外增加一行，并且每行下方都额外增加一格
		UI += "<tr><td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td></tr>";
		for(int i = 0; i <= sizeX + 2; i++)
		{
			UI += "<tr>";
			
			// 额外增加一格 
			UI += "<td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td>";
			
	 		for(int j = 0; j <= sizeY + 2; j++)
	 		{
	 			UI += grid[i][j];
	 		}
	 		UI += "</tr>";
		}
		UI += "</table>";
		
		return UI; 
	}
	
	// 将字符串转为一个位置pair。必须确保字符串是合法的再执行这个操作。 
	pair<int, int> TranString(string s)
	{
		if (s[0] <= 'z' && s[0] >= 'a')
		{
			s[0] = s[0] - 'a' + 'A';
		}
		int nowY = s[0] - 'A' + 1, nowX = s[1] - '0'; 
		if (s.length() == 3)
		{
			nowX = (s[1] - '0') * 10 + s[2] - '0';
		}
		pair<int, int> ret;
		ret.first = nowX;
		ret.second = nowY;
		return ret;
	}
	
	// 检查一个字符串是否是合法的位置 
	string CheckMove(string s)
	{
		// 长度必须为2或3 
		if (s.length() != 2 && s.length() != 3)
		{
			return "[错误] 每个格子只能是长度不超过 3 的字符串，如：A1";
		}
		// 大小写不敏感 
		if (s[0] <= 'z' && s[0] >= 'a')
		{
			s[0] = s[0] - 'a' + 'A';
		}
		// 检查是否为合法输入 
		if (s[0] > 'Z' || s[0] < 'A' || s[1] > '9' || s[1] < '0' )
		{
			return "[错误] 请输入合法的字符串（字母+数字），如：A1";
		}
		if (s.length() == 3 && (s[2] > '9' || s[2] < '0'))
		{
			return "[错误] 请输入合法的字符串（字母+数字），如：A1";
		}
		// 转化 (注意XY)
		int nowY = s[0] - 'A' + 1, nowX = s[1] - '0'; 
		if (s.length() == 3)
		{
			nowX = (s[1] - '0') * 10 + s[2] - '0';
		}
		// 检查是否越界
		if (nowX < 1 || nowX > sizeX || nowY < 1 || nowY > sizeY) 
		{
			return "[错误] 你选择的位置超出了棋盘的大小";
		}
		
		// OK
		return "OK";
	}
	
	// 进行一次常规落子 
	string MakeMove1(string s, int round)
	{
		int player = (round - 1) % 2 + 1; 
		auto pos = TranString(s);
		int x = pos.first, y = pos.second; 
		if(chess[x][y] != 0)
		{
			return "[错误] 这个位置已经下过棋子了";
		}
		
		for (int i = x - 1; i <= x + 1; i++)
		{
			for (int j = y - 1; j <= y + 1; j++)
			{
				// 除了第四回合 不允许非法落子 
				if (chess[i][j] == player && round != 4)
				{
					return "[错误] 你不能在这个位置落子，因为该位置的周围八格有己方的棋子";
				}
			}
		}
		 
		
		lastX1 = pos.first, lastY1 = pos.second;
		lastX2 = -1, lastY2 = -1;
		chess[pos.first][pos.second] = player; 
		
		
		
		// OK
		return "OK"; 
	} 
	
	// 进行一次常规移动 
	string MakeMove2(string s1, string s2, int round)
	{
		int player = (round - 1) % 2 + 1; 
		auto pos1 = TranString(s1);
		auto pos2 = TranString(s2);
		
		int x1 = pos1.first, x2 = pos2.first, y1 = pos1.second, y2 = pos2.second;
		
		if ((x1 != x2 && y1 != y2) || (x1 == x2 && y1 == y2))
		{
			return "[错误] 这不是一个合法的移动";
		}
		
		if (chess[x1][y1] != player)
		{
			return "[错误] 这个位置没有可以移动的棋子";
		}
		
		int addx = 0, addy = 0;
		if (x1 < x2) addx = 1;
		if (x1 > x2) addx = -1;
		if (y1 < y2) addy = 1;
		if (y1 > y2) addy = -1;
		
		while(x1 != x2 || y1 != y2)
		{
			x1 = x1 + addx;
			y1 = y1 + addy;
			if (chess[x1][y1] != 0)
			{
				return "[错误] 移动的路径上有别的棋子";
			}
		}
		
		x1 = pos1.first, x2 = pos2.first, y1 = pos1.second, y2 = pos2.second;
		
		chess[x1][y1] = 0;
		chess[x2][y2] = player;
		
		// 注意last2才是被移动的 
		lastX1 = x2, lastY1 = y2;
		lastX2 = x1, lastY2 = y1;
		
		
		// OK
		return "OK"; 
	} 
	
	
};

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility), MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , stop(0)
    {
    }

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 当前行动玩家
    int currentPlayer;
    // 棋盘
    Board board;
    // 强制停止 
    bool stop;
    // 回合数 
	int round_;

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        // reply() << "这里输出当前游戏情况";
        // Returning |OK| means the game stage
        return StageErrCode::OK;
    }

};


class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
        : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合",
                MakeStageCommand(*this, "落子", &RoundStage::MakeMove1_, AnyArg("落子", "A1")),
                MakeStageCommand(*this, "移子", &RoundStage::MakeMove2_, AnyArg("起始位置", "A1")
																, AnyArg("结束位置", "A2")))
    {
    }

    virtual void OnStageBegin() override
    {
		Global().Boardcast() << Markdown(Main().board.GetUI());
		Global().SetReady(!Main().currentPlayer);
        Global().StartTimer(GAME_OPTION(时限));
        
        Global().Boardcast() << Name() << "，请" << (Main().round_ % 2 == 1 ? "黑" : "白") << "方落子。";
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
    	int &cp = Main().currentPlayer;
        Global().Boardcast() << At(PlayerID(cp)) << "玩家超时，游戏结束。";
        Main().player_scores_[!cp] = 1;
		Main().stop = 1;
		Global().SetReady(0), Global().SetReady(1);
        // Returning |CHECKOUT| means the current stage will be over.
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Global().Boardcast() << Global().PlayerName(pid) << "退出游戏，游戏立刻结束。";
        Main().player_scores_[!pid] = 1;
        Main().stop = 1;
        Global().SetReady(0), Global().SetReady(1);
        // Returning |CONTINUE| means the current stage will be continued.
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        Global().Boardcast() << "暂无bot";
	    Main().stop = 1;
	    Main().player_scores_[!pid] = 1;
	    Global().SetReady(0), Global().SetReady(1);
	    return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
    	// 不执行任何操作 
        return StageErrCode::CHECKOUT;
    }

  private:
    AtomReqErrCode MakeMove1_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, string str)
    {
        
        if (Global().IsReady(pid)) {
            reply() << "[错误] 现在是对方的回合";
            return StageErrCode::FAILED;
        }
        
        string ret = Main().board.CheckMove(str);
        
        // 不合法的落子选择 
        if (ret != "OK")
        {
			reply() << ret;
			return StageErrCode::FAILED;
		}
		
		ret = Main().board.MakeMove1(str, Main().round_);
		// 不合法的移动选择 
        if (ret != "OK")
        {
			reply() << ret;
			return StageErrCode::FAILED;
		}
        
        return StageErrCode::READY;
    }
    
    AtomReqErrCode MakeMove2_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, string str1, string str2)
    {
        
        if (Global().IsReady(pid)) {
            reply() << "[错误] 现在是对方的回合";
            return StageErrCode::FAILED;
        }
        
        string ret1 = Main().board.CheckMove(str1);
        string ret2 = Main().board.CheckMove(str2);
        
        // 不合法的落子选择 
        if (ret1 != "OK")
        {
			reply() << ret1;
			return StageErrCode::FAILED;
		}
		if (ret2 != "OK")
        {
			reply() << ret2;
			return StageErrCode::FAILED;
		}
		
		ret1 = Main().board.MakeMove2(str1, str2, Main().round_);
		// 不合法的移动选择 
        if (ret1 != "OK")
        {
			reply() << ret1;
			return StageErrCode::FAILED;
		}
        
        return StageErrCode::READY;
    }
};

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
	// 随机生成先后手 
	srand((unsigned int)time(NULL));
	currentPlayer = rand() % 2;
	Global().Boardcast() << "先手（黑棋）：" << At(PlayerID(currentPlayer));
	
	// 设置读取棋盘大小 
	board.sizeX = (GAME_OPTION(边长));
	board.sizeY = (GAME_OPTION(边长));
	
    setter.Emplace<RoundStage>(*this, ++round_);
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
	// 有人退出，强制结束 
	if (stop == 1)
	{
		Global().Boardcast() << Markdown(board.GetUI());
		return;
	}
	round_++;
	// 棋盘已满 或 回合到达上限 
	if (round_ > board.sizeX * board.sizeY || round_ > (GAME_OPTION(回合数)))
	{ 
		// 平局 
		Global().Boardcast() << "棋盘已满或回合达到上限，游戏结束";
		Global().Boardcast() << Markdown(board.GetUI());
		return;
	}
	// 检查胜利
	int winner = -1;
	int sz = (GAME_OPTION(边长));
	auto &c = board.chess;
	
	// 这里会访问到棋盘之外的数组位置，不过类里的棋盘数组开到了30x30，远超过了options中的边长上限15
	// 所以理论上不用担心越界问题。但是如果options被改动，这里必须也改动以避免段错误 
	for (int i = 1; i <= sz; i++)
	{
		for (int j = 1; j <= sz; j++)
		{
			if (c[i][j] == 0)
			{
				continue;
			}
			if ((c[i][j] == c[i + 1][j] && c[i][j] == c[i + 2][j] && c[i][j] == c[i + 3][j]) ||
			(c[i][j] == c[i][j + 1] && c[i][j] == c[i][j + 2] && c[i][j] == c[i][j + 3]) ||
			(c[i][j] == c[i + 1][j + 1] && c[i][j] == c[i + 2][j + 2] && c[i][j] == c[i + 3][j + 3]))
			{
				winner = c[i][j] - 1; 
			}
			// 特判，不然可能访问到负数位置 
			if (j >= 4)
			{
				if (c[i][j] == c[i + 1][j - 1] && c[i][j] == c[i + 2][j - 2] && c[i][j] == c[i + 3][j - 3])
				{
					winner = c[i][j] - 1; 
				}
			}
		}
	}
	if (winner != -1)
	{
		player_scores_[winner] = 1;
		Global().Boardcast() << "有玩家连成四子，游戏结束";
		Global().Boardcast() << Markdown(board.GetUI());
		return;
	}
	
	// 交换玩家 
	currentPlayer = !currentPlayer;
    setter.Emplace<RoundStage>(*this, round_);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

