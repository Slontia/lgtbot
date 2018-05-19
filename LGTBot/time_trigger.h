#pragma once

#include <iostream>
#include <functional>
#include "windows.h"

class TimeTrigger
{
private:
  uint32_t sec_;
  const std::function<void()> handle_;
public:
  TimeTrigger(std::function<void()> handle);
  void ResetTime(uint32_t sec);
};