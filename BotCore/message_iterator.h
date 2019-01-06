#pragma once

#include "lgtbot.h"

class MessageIterator
{
public:
  const MessageType type_;
  const QQ usr_qq_;
  const QQ src_qq_;

  MessageIterator(const MessageType& type, const QQ& src_qq, const QQ& usr_qq, std::vector<std::string>&& msgs) :
    type_(type), src_qq_(src_qq), usr_qq_(usr_qq), msgs_(std::forward<std::vector<std::string>>(msgs)), it_(msgs_.begin())
  {}


  std::string get_next()
  {
    return *(it_++);
  }

  bool has_next() const
  {
    return it_ != msgs_.end();
  }

  bool is_public() const { return type_ == GROUP_MSG || type_ == DISCUSS_MSG; }

  virtual void Reply(const std::string& msg) const = 0;

  void Reply(const std::string& msg, const std::string& bakmsg)
  {
    if (!msg.empty())
      Reply(msg);
    else if (!bakmsg.empty())
      Reply(bakmsg);
  }

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
    LGTBOT::send_private_msg(usr_qq_, msg);
  }
};

class GroupMessageIterator : public MessageIterator
{
public:
  GroupMessageIterator(const QQ& usr_qq, std::vector<std::string>&& msgs, const QQ& group_qq) :
    MessageIterator(PRIVATE_MSG, group_qq, usr_qq, std::forward<std::vector<std::string>>(msgs)) {}

  void Reply(const std::string& msg) const
  {
    LGTBOT::send_group_msg(src_qq_, msg, usr_qq_);
  }
};

class DiscussMessageIterator : public MessageIterator
{
public:
  DiscussMessageIterator(const QQ& usr_qq, std::vector<std::string>&& msgs, const QQ& discuss_qq) :
    MessageIterator(PRIVATE_MSG, discuss_qq, usr_qq, std::forward<std::vector<std::string>>(msgs)) {}

  void Reply(const std::string& msg) const
  {
    LGTBOT::send_discuss_msg(src_qq_, msg, usr_qq_);
  }
};