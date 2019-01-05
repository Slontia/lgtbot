#pragma once

#include <mutex>
#include <iostream>
#include <string>

#define INVALID_QQ (QQ)0

#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

#define RETURN_IF_FAILED(str) \
do\
{\
  if (const std::string& err = (str); !err.empty())\
    return err;\
} while (0);


/*
1. 进行“开始”指令、“加入”指令和“退出”指令的衔接
2. 补充错误提示
3. 精简mygame中的接口
4. 最好可以一次从iterator里取出多个字符串
*/

typedef int64_t QQ;
typedef std::string ErrMsg;

extern std::mutex mutex;

const int32_t LGT_AC = -1;

enum MessageType
{
  PRIVATE_MSG,
  GROUP_MSG,
  DISCUSS_MSG
};

typedef void(*AT_CALLBACK)(const QQ&, const char*, const size_t& sz);
typedef void(*MSG_CALLBACK)(const QQ&, const char*);

class LGTBOT
{
public:
  static AT_CALLBACK at_cq;
  static MSG_CALLBACK send_private_msg_callback;
  static MSG_CALLBACK send_group_msg_callback;
  static MSG_CALLBACK send_discuss_msg_callback;
  static QQ this_qq;

  DLL_EXPORT static void LoadGames();
  DLL_EXPORT static bool Request(const MessageType& msg_type, const QQ& src_qq, const QQ& usr_qq, char* msg);
  DLL_EXPORT static void set_at_cq_callback(AT_CALLBACK fun);
  DLL_EXPORT static void set_send_private_msg_callback(MSG_CALLBACK fun);
  DLL_EXPORT static void set_send_group_msg_callback(MSG_CALLBACK fun);
  DLL_EXPORT static void set_send_discuss_msg_callback(MSG_CALLBACK fun);
  DLL_EXPORT static void set_this_qq(const QQ& this_qq);

  static bool is_at(const std::string& msg);
  static std::string at_str(const QQ&);
  static void send_private_msg(const QQ& usr_qq, const std::string& msg);
  static void send_group_msg(const QQ& group_qq, const std::string& msg);
  static void send_discuss_msg(const QQ& discuss_qq, const std::string& msg);
  static void send_group_msg(const QQ& group_qq, const std::string& msg, const QQ& to_usr_qq);
  static void send_discuss_msg(const QQ& discuss_qq, const std::string& msg, const QQ& to_usr_qq);
};








