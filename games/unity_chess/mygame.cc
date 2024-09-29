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
    .name_ = "合纵连横", // the game name which should be unique among all the games
    .developer_ = "睦月",
    .description_ = "通过三连珠进行棋盘染色的棋类游戏",
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return 2; } // 0 indicates no max-player limits
uint32_t Multiple(const CustomOptions& options) { return 2; } // the default score multiple for the game, 0 for a testing game, 1 for a formal game, 2 or 3 for a long formal game
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
	int sizeX = 6, sizeY = 6;
	// 棋子 
    int chess[30][30] = {0};
    // 颜色
	int color[30][30] = {0};
	// 上一枚落子 
	int lastX = 0, lastY = 0;
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
		// 记录颜色格数
		int n1 = 0, n2 = 0;
		
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
			 	string cr = "";
			 	if(color[i][j] == 0)
			 		cr = "#C2C2C2";
			 	if(color[i][j] == 1)
			 	{
			 		n1++;
				 	cr = "#6495ED";
			 	}
				if(color[i][j] == 2)
				{
					n2++;
					cr = "#DDA0DD";
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
				 	ch = "<font color=\"#F3F3F3\">●</font>";
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
		
		// 颜色格数量显示 
		UI += "<table style=\"text-align:center;margin:auto;\"><tbody><tr>";
		UI += "<td bgcolor=\"#6495ED\"><font size=7>" + fill + "</font></td>";
		UI += "<td bgcolor=\"#FFFFFF\"><font size=7>" + str(n1) + "</font></td>";
		UI += "<td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td>";
		UI += "<td bgcolor=\"#FFFFFF\"><font size=7>" + fill + "</font></td>";
		UI += "<td bgcolor=\"#DDA0DD\"><font size=7>" + fill + "</font></td>";
		UI += "<td bgcolor=\"#FFFFFF\"><font size=7>" + str(n2) + "</font></td>";
		UI += "</tr><tr></tr></table>";
		
		return UI; 
	}
	
	// 检查一次行动 
	string CheckMove(string s, int player)
	{
		// 长度必须为2或3 
		if (s.length() != 2 && s.length() != 3)
		{
			return "[错误] 请输入长度不超过 3 的字符串，如：A1";
		}
		// 大小写不敏感 
		if (s[0] <= 'z' && s[0] >= 'a')
		{
			s[0] = s[0] - 'a' + 'A';
		}
		/*
			为了支持更高位数，不支持横纵交换了。 
		*/ 
//		// 横纵交换不敏感 
//		if (s[0] <= '9' && s[0] >= '0')
//		{
//			swap(s[0], s[1]);
//		}
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
		// 检查是否已经有子 
		if (chess[nowX][nowY] != 0)
		{
			return "[错误] 你选择的位置已经有棋子了";
		}
		
		// Ok进行一次落子 
		Move(nowX, nowY, player); 
		
		return "OK";
	}
	
	// 执行一次落子 
	void Move(int x, int y, int player)
	{
		chess[x][y] = player;
		// 检查是否满足染色
		if (
			(chess[x - 1][y - 1] == chess[x][y] && chess[x + 1][y + 1] == chess[x][y]) ||
			(chess[x + 1][y - 1] == chess[x][y] && chess[x - 1][y + 1] == chess[x][y]) ||
			(chess[x][y - 1] == chess[x][y] && chess[x][y + 1] == chess[x][y]) ||
			(chess[x - 1][y] == chess[x][y] && chess[x + 1][y] == chess[x][y]) 
		)
		{
			Draw(x, y, player);
		}
		
		// 左上 
		if(x > 1 && y > 1 && chess[x - 2][y - 2] == chess[x][y] && chess[x - 1][y - 1] == chess[x][y])
			Draw(x - 1, y - 1, player);
		// 上 
		if(x > 1 && chess[x - 2][y] == chess[x][y] && chess[x - 1][y] == chess[x][y])
			Draw(x - 1, y, player);
		// 右上 
		if(x > 1 && y < sizeY && chess[x - 2][y + 2] == chess[x][y] && chess[x - 1][y + 1] == chess[x][y])
			Draw(x - 1, y + 1, player);
		// 右 
		if(y < sizeY && chess[x][y + 2] == chess[x][y] && chess[x][y + 1] == chess[x][y])
			Draw(x, y + 1, player);
		// 右下 
		if(x < sizeX && y < sizeY && chess[x + 2][y + 2] == chess[x][y] && chess[x + 1][y + 1] == chess[x][y])
			Draw(x + 1, y + 1, player);
		// 下 
		if(x < sizeX && chess[x + 2][y] == chess[x][y] && chess[x + 1][y] == chess[x][y])
			Draw(x + 1, y, player);
		// 左下 
		if(x < sizeX && y > 1 && chess[x + 2][y - 2] == chess[x][y] && chess[x + 1][y - 1] == chess[x][y])
			Draw(x + 1, y - 1, player);
		// 左 
		if(y > 1 && chess[x][y - 2] == chess[x][y] && chess[x][y - 1] == chess[x][y])
			Draw(x, y - 1, player);
	}
	
	// 执行一次染色
	void Draw(int x, int y, int player)
	{
		for (int i = x - 1; i <= x + 1; i++)
		{
			for (int j = y - 1; j <= y + 1; j++)
			{
				color[i][j] = player;
			}
		}
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
                MakeStageCommand(*this, "落子", &RoundStage::MakeMove_, AnyArg("落子", "A1")))
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
    AtomReqErrCode MakeMove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, string str)
    {
        
        if (Global().IsReady(pid)) {
            reply() << "[错误] 现在是对方的回合";
            return StageErrCode::FAILED;
        }
        
        string ret = Main().board.CheckMove(str, (!(Main().round_ % 2)) + 1);
        
        // 不合法的落子选择 
        if (ret != "OK")
        {
			reply() << ret;
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
    return;
}

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
	// 有人退出，强制结束 
	if (stop == 1)
	{
		Global().Boardcast() << Markdown(board.GetUI());
		return;
	}
	// 棋盘已满 
	if ((++round_) > board.sizeX * board.sizeY)
	{ 
		// 开始结算胜负 
		int score1 = 0, score2 = 0;
		for (int i = 1; i <= board.sizeX; i++)
		{
			for (int j = 1; j <= board.sizeY; j++)
			{
				if (board.color[i][j] == 1)
					score1++;
				else
					score2++;
			}
		}
		player_scores_[0] = score1;
		player_scores_[1] = score2;
		Global().Boardcast() << Markdown(board.GetUI());
		return;
	}
	// 交换玩家 
	currentPlayer = !currentPlayer;
    setter.Emplace<RoundStage>(*this, round_);
    return;
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

