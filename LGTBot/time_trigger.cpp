#include "stdafx.h"

// [warning] HandleStack maybe not avaliable in TimeData

TimeTrigger::TimeData::TimeData(uint32_t interval, TimeTrigger::HandleStack& handle_stack) :
  interval_(interval), handle_stack_(handle_stack) {}

uint32_t TimeTrigger::TimeData::interval()
{
  return interval_;
}

TimeTrigger::HandleStack& TimeTrigger::TimeData::handle_stack()
{
  return handle_stack_;
}

TimeTrigger::TimeTrigger() {}

DWORD WINAPI TimeTrigger::ThreadFunc(LPVOID data_ptr)
{
  time_t begin_t = time(NULL);
  TimeData* data = (TimeData*) data_ptr;
  while (true)
  {
    if (difftime(time(NULL), begin_t) < data->interval)       // compare time
    {
      HandleStack& handle_stack = data->handle_stack();
      while (!handle_stack.empty() && handle_stack.top()())   // GameState over
      {
        handle_stack.pop();                                   // remove TimeHandle of ended GameState
      }
      break;
    }
  }
  return 0;
}

void TimeTrigger::Time(uint32_t interval)
{
  thread_ = CreateThread(NULL, 0, ThreadFunc, &TimeData(interval, handle_stack_), 0, NULL);
}

void TimeTrigger::push_handle_to_stack(std::function<bool()> handle)
{
  handle_stack_.push(handle);
}