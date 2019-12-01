#pragma once
#include <iostream>
/*
将Game、GameState和GamePlayer作为dll的一部分
传入dll的参数：At private group discuss
传出dll的参数：开始游戏接口、请求接口、游戏信息（名称、最小玩家数、最大玩家数）

// bool get_int(const std::string& raw, std::shared_ptr<int>& i); (better ?)
bool get_str(const std::string& raw, std::string& str);
bool get_int(const std::string& raw, uint32_t& i);
bool get_limit_int(const std::string& raw, uint32_t& i)
{
  return ((i = atoi(raw.c_str())) < 10);
}

template <class F, class T>
std::function<bool(const std::string&)> check_msg(std::function<bool(const std::string&, T&)> f, T& t)
{
  return std::bind(f, std::placeholder::_1, t);
}

struct MsgArgChecker
{
  virtual bool do_check(const std::string&) = 0;
  virtual std::string hint() const = 0;
  virtual std::string example() const = 0;
}

struct MsgArgIntChecker : public MsgArgChecker
{
  int& i_;
  MsgArgIntChecker(int& i) : i_(i) {}
  bool do_check(const std::string&) { ... }
  std::string hint() const { return "[整数]"; }
  std::string example() const { return "1"; }
}

struct MsgChecker
{
  std::vector<std::shared_ptr<MsgArgChecker>> 

}

std::string str;
uint32_t i;
double d;
if (msg_iter.check_next(check_arg(get_str, str), check_arg(get_int, i), check_over()))
{
  // do something with str & i
}
else if (msg_iter.check(check_msg(get_double)))
*/

#ifdef BUILD_DLL
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI 
#endif

class Game;
class Match;

#ifdef __cplusplus
extern "C" {
#endif

  typedef void(*AtCallback)(void* const match, const uint32_t player_id, const char* const msg, const size_t&);
  typedef void(*PrivateResponseCallback)(void* const match, const uint32_t player_id, const char* const msg);
  typedef void(*PublicResponseCallback)(void* const match, const char* const msg);
  typedef void(*GameOverCallback)(void* const match, int32_t* const scores, const uint32_t player_num);

  DLLAPI void Init(const AtCallback at,
                   const PrivateResponseCallback pri_msg_cb,
                   const PublicResponseCallback pub_msg_cb,
                   const GameOverCallback game_over_cb);
  DLLAPI Game* StartGame(const uint32_t player_num);
  DLLAPI bool RequestGame(Game* const game, int64_t* qq, char* const msg);
  DLLAPI void GameInfo(char** const name, uint32_t* const min_player, uint32_t* const max_player);

#ifdef __cplusplus
}
#endif
