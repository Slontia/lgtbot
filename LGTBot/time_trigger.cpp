#include "stdafx.h"
#include <mutex>
#include "time_trigger.h"
#include "global.h"


TimeTrigger timer;
std::mutex mutex;

ThreadGuard::ThreadGuard(const uint32_t& itv_minu, TimeTrigger::HandleStack& handle_stack) :
  terminated_(false), handle_stack_(handle_stack), thread_(std::bind(&ThreadGuard::ThreadFunc, this, itv_minu)) {}

ThreadGuard::~ThreadGuard()
{
  terminated_ = true;
  if (thread_.joinable()) thread_.join();
}

void ThreadGuard::ThreadFunc(const uint32_t& itv_minu)
{
  auto start_time = time(NULL);

  while (difftime(time(NULL), start_time) < itv_minu * 60 && !terminated_)
  {
    std::this_thread::sleep_for(std::chrono::minutes(1));
  }

  std::unique_lock<std::mutex> lock(mutex);
  while (true)
  {
    if (lock.try_lock())
    {
      /* Time up. Keep call functions from stack until false returned or stack cleared. */
      while (!handle_stack_.empty() && handle_stack_.top()()) handle_stack_.pop();
      return;
    }

    if (terminated_) return;

    std::this_thread::sleep_for(std::chrono::minutes(1));
  }
}

void ThreadGuard::terminate()
{
  terminated_ = true;
}


TimeTrigger::TimeTrigger() {}

void TimeTrigger::Time(const uint32_t& interval)
{
  thread_guard_ = std::make_shared<ThreadGuard>(interval, handle_stack_);
}

void TimeTrigger::Terminate()
{
  if (thread_guard_) thread_guard_->terminate();
}

void TimeTrigger::push_handle_to_stack(std::function<bool()> handle)
{
  handle_stack_.push(handle);
}

void TimeTrigger::clear_stack()
{
  while (!handle_stack_.empty())
  {
    handle_stack_.pop();
  }
}