#pragma once

#include <iostream>
#include <functional>
#include <stack>
#include <thread>

class TimeTrigger
{
public:
  typedef std::stack<std::function<bool()>> HandleStack;
private:
  time_t start_time_;
  HandleStack handle_stack_;
  bool terminated_;
  std::unique_ptr<std::thread> thread_;
  uint32_t  itv_sec_;
public:
  TimeTrigger();
  ~TimeTrigger();
  void Time(const uint32_t& interval);
  void Terminate();
  void ThreadFunc();
  void push_handle_to_stack(std::function<bool()> handle);
  void clear_stack();
  void pop();
};
 


