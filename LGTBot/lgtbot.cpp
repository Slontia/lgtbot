#include "stdafx.h"
#include "my_game.h"
#include "lgtbot.h"
#include <list>
#include <sstream>
#include <memory>


GameContainer game_container;

MatchManager match_manager;



bool is_at(const std::string& msg)
{
  return msg.find(at_cq(CQ_getLoginQQ(-1))) != std::string::npos;
}



std::string NewGame(std::string game_id, int64_t host_qq)
{

}

void GameOver()
{

}

void LGTBOT::LoadGames()
{
  LOG_INFO("马斯塔，老子他娘的启动了！");
  Bind(game_container); /* Mygame */
}

void show_gamelist(MessageIterator& msg)
{
  auto gamelist = game_container.gamelist();
  std::stringstream ss;
  int i = 0;
  ss << "游戏列表：" << std::endl;
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

enum MassageIteratorError
{
  REACH_END,
  EXPECTED_NUM
};

template <class T>
bool to_type(const std::string& str, T& res)
{
  return (bool)(std::stringstream(str) >> res);
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

/* msg is not empty */
void global_request(MessageIterator& msg)
{
  assert(msg.has_next());
  const std::string front_msg = msg.get_next();

  if (front_msg == "游戏大厅")
  {

  }
  else if (front_msg == "游戏列表")
  {
    show_gamelist(msg);
  }
  else if (front_msg == "规则")
  {

  }
  else if (front_msg == "新游戏") // 新游戏 <游戏名> (公开|私密)
  {
    new_game(msg);
  }
  else if (front_msg == "帮助")
  {

  }
  else if (front_msg == "开始") // host only
  {

  }
  else if (front_msg == "加入") // 加入 [比赛编号]
  {

  }
  else if (front_msg == "退出")
  {

  }
}

bool Request(const MessageType& msg_type, const QQ& src_qq, const QQ& usr_qq, const std::string& msg)
{
  if (msg.empty()) return false;

  bool is_global = false;
  size_t last_pos = 0, pos = 0;

  /* If it is a global request, we should move char pointer behind '#'. */
  if (msg[0] == '#')
  {
    last_pos = 1;
    is_global = true;
  }
  
  std::vector<std::string> substrs;

  while (true)
  {
    /* Read next word. */
    pos = msg.find(' ', last_pos);
    if (last_pos != pos) substrs.push_back(msg.substr(last_pos, pos));

    /* If finish reading, break. */
    if (pos == std::string::npos) break;

    last_pos = pos + 1;
  }

  if (substrs.empty()) return true;

  /* Make Message Iterator. */
  std::shared_ptr<MessageIterator> msgit_p = 
    (msg_type == PRIVATE_MSG) ? std::dynamic_pointer_cast<MessageIterator>(std::make_shared<PrivateMessageIterator>(usr_qq, std::move(substrs))) :
    (msg_type == GROUP_MSG)   ? std::dynamic_pointer_cast<MessageIterator>(std::make_shared<GroupMessageIterator>(usr_qq, std::move(substrs), src_qq)) :
    (msg_type == DISCUSS_MSG) ? std::dynamic_pointer_cast<MessageIterator>(std::make_shared<DiscussMessageIterator>(usr_qq, std::move(substrs), src_qq)) :
                                nullptr;
  assert(msgit_p != nullptr);
  MessageIterator& msgit = *msgit_p;

  /* Distribute msg to global handle or match handle. */
  if (is_global)
    global_request(msgit);
  else
    match_manager.Request(msgit);

  return true;
}
