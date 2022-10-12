#include <array>
#include <functional>
#include <memory>
#include <set>

#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "problems.h"

using namespace std; 

string specialRule(vector<Player>& players, int type, string time)
{
    string ret = "";
    if(time == "gameStart")
    {
        ret += "本局游戏特殊规则：\n\n";
        if(type == 1)
        {
            ret += "    [ 迷雾模式 ]";
            ret += "\n\n";
            ret += "游戏结束前，不公布玩家选项及分数。";
        }
        if(type == 2)
        {
            ret += "    [ 马太效应 ]";
            ret += "\n\n";
            ret += "每个回合结束时，得分最高的玩家+1。";
        }
        if(type == 3)
        {
            ret += "    [ 中间赛跑 ]";
            ret += "\n\n";
            ret += "每个回合结束时，得分最高的玩家-2。";
        }
        if(type == 4)
        {
            ret += "    [ 一线之隔 ]";
            ret += "\n\n";
            ret += "每个回合结束时，分数最低和最高的玩家分数交换。";
        }	
        if(type == 5)
        {
			ret += "    [ 有意为之 ]";
            ret += "\n\n";
            ret += "游戏结束时，所有玩家分数取反。";
		}
		if(type == 6)
        {
			ret += "    [ 整型溢出 ]";
            ret += "\n\n";
            ret += "每个回合结束时，如若某玩家的分数 > 10 ，他 -10。";
		}
		if(type == 7)
        {
			ret += "    [ 战术挂机 ]";
            ret += "\n\n";
            ret += "每个回合结束时，如果某玩家选择了 A ，他 +0.5。";
		}
		if(type == 8)
	    {
			ret += "    [ 华丽收场 ]";
	        ret += "\n\n";
	        ret += "游戏结束时，如果某玩家分数为 0  ，他 +100。";
		}
		
		return ret; 
    }
    
    
    
	int n = players.size();
	double maxScore = -99999;
	double minScore = 99999;
	for(int i = 0; i < n; i++)
	{
		maxScore = max(maxScore, players[i].score);
		minScore = min(minScore, players[i].score);
	}
	
    
    if(type == 1)
    {
		
	}
	if(type == 2)
	{
		if(time == "roundEnd")
		{
			for(int i = 0; i < n; i++)
			{
				if(players[i].score == maxScore)
					players[i].score += 1;
			}
		}
	}
	if(type == 3)
	{
		if(time == "roundEnd")
		{
			for(int i = 0; i < n; i++)
			{
				if(players[i].score == maxScore)
					players[i].score -= 2;
			}
		}
	}
	if(type == 4)
	{
		if(time == "roundEnd")
		{
			for(int i = 0; i < n; i++)
			{
				if(players[i].score == maxScore) players[i].score = minScore;
				else if(players[i].score == minScore) players[i].score = maxScore;
			}
		}
	}
	if(type == 5)
	{
		if(time == "gameEnd")
		{
			for(int i = 0; i < n; i++)
			{
				players[i].score = -players[i].score;
			}
		}
	}
	if(type == 6)
	{
		if(time == "roundEnd")
		{
			for(int i = 0; i < n; i++)
			{
				if(players[i].score > 10)
					players[i].score -= 10;
			}
		}
	}
	if(type == 7)
	{
		if(time == "roundEnd")
		{
			for(int i = 0; i < n; i++)
			{
				if(players[i].select == 0)
					players[i].score += 0.5;
			}
		}
	}
	if(type == 8)
	{
		if(time == "gameEnd")
		{
			for(int i = 0; i < n; i++)
			{
				if(players[i].score == 0)
					players[i].score = 100;
			}
		}
	}



    return ret;
}
