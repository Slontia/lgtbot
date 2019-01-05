#include "stdafx.h"

#include <sstream>


#include "message_iterator.h"
#include "game_container.h"

#include "message_handlers.h"
#include "match.h"

static std::string read_gamename(MessageIterator& msg);

template <class T>
static bool to_type(const std::string& str, T& res)
{
  return (bool) (std::stringstream(str) >> res);
}

void show_gamelist(MessageIterator& msg)
{
  auto gamelist = game_container.gamelist();
  std::stringstream ss;
  int i = 0;
  ss << "游戏列表：";
  for (auto it = gamelist.begin(); it != gamelist.end(); it++)
  {
    auto name = *it;
    ss << std::endl << i << '.' << name;
  }
  msg.Reply(ss.str());
}

std::string read_gamename(MessageIterator& msg)
{
  if (!msg.has_next())
  {
    /* error: without game name */
    return "";
  }
  const std::string& name = msg.get_next();
  if (!game_container.has_game(name))
  {
    /* error: game not found */
    return "";
  }
  return name;
};

void new_game(MessageIterator& msg)
{
  auto gamename = read_gamename(msg);
  if (gamename.empty())
  {
    msg.Reply("未知的游戏名");
    return;
  }
  if (msg.has_next())
  {
    auto match_type = msg.get_next();
    if (match_type == "公开")
    {
      switch (msg.type_)
      {
        case PRIVATE_MSG:
          msg.Reply("公开游戏请在群或讨论组中建立");
          return;
        case GROUP_MSG:
          msg.Reply(match_manager.new_match(GROUP_MATCH, gamename, msg.usr_qq_, msg.src_qq_), "创建成功");
          break;
        case DISCUSS_MSG:
          msg.Reply(match_manager.new_match(DISCUSS_MATCH, gamename, msg.usr_qq_, msg.src_qq_), "创建成功");
          break;
        default:
          /* Never reach here! */
          assert(false);
      }
    }
    else if (match_type == "私密")
    {
      msg.Reply(match_manager.new_match(PRIVATE_MATCH, gamename, msg.usr_qq_, INVALID_QQ), "创建成功");
    }
    else
    {
      msg.Reply("未知的比赛类型，应该为公开或者私密");
    }
  }
}



void start_game(MessageIterator& msg)
{
  if (msg.has_next())
  {
    msg.Reply("多余的参数");
    return;
  }
  msg.Reply(match_manager.StartGame(msg.usr_qq_), "开始游戏成功");
}

void leave(MessageIterator& msg)
{
  if (msg.has_next())
  {
    msg.Reply("多余的参数");
    return;
  }
  msg.Reply(match_manager.DeletePlayer(msg.usr_qq_), "退出成功");
}

void join(MessageIterator& msg)
{
  MatchId id;
  if (msg.has_next())
  {
    /* Match ID */
    if (!to_type(msg.get_next(), id))
    {
      msg.Reply("比赛ID必须为整数！");
      return;
    }
    if (msg.has_next())
    {
      msg.Reply("多余的参数");
      return;
    }
  }
  else
  {
    std::shared_ptr<Match> match = nullptr;
    switch (msg.type_)
    {
      case PRIVATE_MSG:
        msg.Reply("私密游戏必须指定比赛id");
        return;
      case GROUP_MSG:
        id = match_manager.get_group_match_id(msg.src_qq_);
        break;
      case DISCUSS_MSG:
        id = match_manager.get_discuss_match_id(msg.src_qq_);
        break;
      default:
        assert(false);
    }
  }
  msg.Reply(match_manager.AddPlayer(id, msg.usr_qq_), "加入成功！");
}