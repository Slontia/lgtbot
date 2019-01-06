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
    msg.ReplyError("创建游戏失败！未知的游戏名");
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
          msg.ReplyError("创建游戏失败！公开类型游戏不支持私信建立，请在群或讨论组中公屏建立");
          return;
        case GROUP_MSG:
          msg.Reply(match_manager.new_match(GROUP_MATCH, gamename, msg.usr_qq_, msg.src_qq_), "创建游戏成功！");
          break;
        case DISCUSS_MSG:
          msg.Reply(match_manager.new_match(DISCUSS_MATCH, gamename, msg.usr_qq_, msg.src_qq_), "创建游戏成功！");
          break;
        default:
          /* Never reach here! */
          assert(false);
      }
    }
    else if (match_type == "私密")
    {
      msg.Reply(match_manager.new_match(PRIVATE_MATCH, gamename, msg.usr_qq_, INVALID_QQ), "创建游戏成功！");
    }
    else
    {
      msg.ReplyError("创建游戏失败！未知的比赛类型，应该为【公开】或者【私密】");
    }
  }
}

void start_game(MessageIterator& msg)
{
  if (msg.has_next())
  {
    msg.ReplyError("开始游戏失败！多余的参数");
    return;
  }
  msg.Reply(match_manager.StartGame(msg.usr_qq_), "游戏开始！");
}

void leave(MessageIterator& msg)
{
  if (msg.has_next())
  {
    msg.ReplyError("退出游戏失败！多余的参数");
    return;
  }
  msg.Reply(match_manager.DeletePlayer(msg.usr_qq_), "退出游戏成功！");
}

void join(MessageIterator& msg)
{
  MatchId id;
  if (msg.has_next())
  {
    /* Match ID */
    if (!to_type(msg.get_next(), id))
    {
      msg.ReplyError("加入失败！比赛ID必须为整数");
      return;
    }
    if (msg.has_next())
    {
      msg.ReplyError("加入失败！多余的参数");
      return;
    }
  }
  else
  {
    std::shared_ptr<Match> match = nullptr;
    switch (msg.type_)
    {
      case PRIVATE_MSG:
        msg.ReplyError("加入失败！私密类型游戏，必须指定比赛ID");
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