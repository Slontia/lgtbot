#pragma once

#include <mutex>
#include "cqp.h"

#define INVALID_QQ (QQ)0

/*
1. 传参msg使用iterator
2. game除了两种response之外，再加reply，根据消息来源判断那种response
3. 精简mygame中的接口
*/

typedef int64_t QQ;

extern std::mutex mutex;


const int32_t LGT_AC = -1;

enum MessageType
{
  PRIVATE_MSG,
  GROUP_MSG,
  DISCUSS_MSG
};

namespace LGTBOT
{
  void LoadGames();
  bool Request(const int32_t& subType, const int64_t& fromQQ, std::string msg);
};

inline std::string at_cq(const QQ& user_qq)
{
  return "[CQ:at,qq=" + std::to_string(user_qq) + "]";
}

inline void send_private_msg(const QQ& usr_qq, const std::string& msg)
{
  CQ_sendPrivateMsg(LGT_AC, usr_qq, msg.c_str());
}

inline void send_group_msg(const QQ& group_qq, const std::string& msg)
{
  CQ_sendGroupMsg(LGT_AC, group_qq, msg.c_str());
}

inline void send_group_msg(const QQ& group_qq, const std::string& msg, const QQ& to_usr_qq)
{
  CQ_sendGroupMsg(LGT_AC, group_qq, (at_cq(to_usr_qq) + msg).c_str());
}

inline void send_discuss_msg(const QQ& discuss_qq, const std::string& msg)
{
  CQ_sendDiscussMsg(LGT_AC, discuss_qq, msg.c_str());
}

inline void send_discuss_msg(const QQ& discuss_qq, const std::string& msg, const QQ& to_usr_qq)
{
  CQ_sendDiscussMsg(LGT_AC, discuss_qq, (at_cq(to_usr_qq) + msg).c_str());
}

class MessageIterator
{
public:
  const MessageType type_;
  const QQ usr_qq_;
  const QQ src_qq_;

  MessageIterator(const MessageType& type, const QQ& src_qq, const QQ& usr_qq, std::vector<std::string>&& msgs) :
    type_(type), src_qq_(src_qq), usr_qq_(usr_qq), msgs_(std::forward<std::vector<std::string>>(msgs)), it_(msgs.begin()) {}

  std::string get_next()
  {
    return *(it_++);
  }

  bool has_next() const
  {
    return it_ == msgs_.end();
  }

  bool is_public() const { return type_ == GROUP_MSG || type_ == DISCUSS_MSG; }

  virtual void Reply(const std::string& msg) const = 0;

private:
  const std::vector<std::string> msgs_;
  std::vector<std::string>::const_iterator it_;
};

class PrivateMessageIterator : public MessageIterator
{
public:
  PrivateMessageIterator(const QQ& usr_qq, std::vector<std::string>&& msgs) :
    MessageIterator(PRIVATE_MSG, INVALID_QQ, usr_qq, std::forward<std::vector<std::string>>(msgs)) {}

  void Reply(const std::string& msg) const
  {
    send_private_msg(usr_qq_, msg);
  }
};

class GroupMessageIterator : public MessageIterator
{
public:
  GroupMessageIterator(const QQ& usr_qq, std::vector<std::string>&& msgs, const QQ& group_qq) :
    MessageIterator(PRIVATE_MSG, group_qq, usr_qq, std::forward<std::vector<std::string>>(msgs)) {}

  void Reply(const std::string& msg) const
  {
    send_group_msg(src_qq_, msg, usr_qq_);
  }
};

class DiscussMessageIterator : public MessageIterator
{
public:
  DiscussMessageIterator(const QQ& usr_qq, std::vector<std::string>&& msgs, const QQ& discuss_qq) :
    MessageIterator(PRIVATE_MSG, discuss_qq, usr_qq, std::forward<std::vector<std::string>>(msgs)) {}

  void Reply(const std::string& msg) const
  {
    send_discuss_msg(src_qq_, msg, usr_qq_);
  }
};


