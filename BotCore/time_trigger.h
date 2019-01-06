#pragma once

#include <iostream>
#include <functional>
#include <stack>
#include <thread>

class ThreadGuard;

class TimeTrigger
{
public:
  typedef std::stack<std::function<bool()>> HandleStack;
private:
  HandleStack handle_stack_;
  std::shared_ptr<ThreadGuard> thread_guard_;
public:
  TimeTrigger();
  void Time(const uint32_t& interval);
  void Terminate();
  void push_handle_to_stack(std::function<bool()> handle);
  void clear_stack();
};
 
class ThreadGuard
{
public:
  ThreadGuard(const uint32_t& itv_minu, TimeTrigger::HandleStack& handle_stack);
  ~ThreadGuard();
  ThreadGuard(const ThreadGuard &) = delete;
  ThreadGuard& operator=(const ThreadGuard &) = delete;
  void ThreadFunc(const uint32_t& itv_minu);
  void terminate();

private:
  bool terminated_;
  TimeTrigger::HandleStack& handle_stack_;
  std::thread thread_;
};


