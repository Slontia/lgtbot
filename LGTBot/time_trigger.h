#pragma once

#include <iostream>
#include <functional>
#include <stack>
#include "windows.h"

class TimeTrigger
{
private:
  typedef std::stack<std::function<bool()>> HandleStack;
  class TimeData  // set a data type to pass param to ThreadFunc
  {
  private:
    uint32_t interval_;
    HandleStack& handle_stack_;
  public:
    TimeData(uint32_t interval, HandleStack& handle_stack);
    uint32_t interval();
    HandleStack& handle_stack();
  };
  HandleStack handle_stack_;
  HANDLE thread_;
  static DWORD WINAPI ThreadFunc(LPVOID interval);
public:
  TimeTrigger();
  void Time(uint32_t interval);
  void push_handle_to_stack(std::function<bool()> handle);
  void clear_stack();
};




