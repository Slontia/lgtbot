#pragma once

#include <iostream>
#include <functional>
#include <stack>
#include "windows.h"

class TimeTrigger
{
private:
  uint32_t sec_;
  std::vector<std::function<void()>> handle_stack_;
public:
  TimeTrigger();
  void Time(uint32_t sec);
  void PushHandle(std::function<void()> handle);
  std::function<void()> PopHandle();
};