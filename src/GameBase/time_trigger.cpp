#include "stdafx.h"
#include <mutex>
#include "time_trigger.h"
//#include "lgtbot.h"

std::mutex mutex;

void TimeTrigger::ThreadFunc()
{
  while (itv_sec_)
  {
    while (difftime(time(NULL), start_time_) < itv_sec_ && !terminated_)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    itv_sec_ = 0;
    if (mutex.try_lock())
    {
      /* Time up. Keep call functions from stack until false returned or stack cleared. */
      if (!terminated_)
      {
        terminated_ = true;
        /* The TimerHandle will be popped before stage endings. If Timer is still needed, terminated_ will be set false. */
        while (!handle_stack_.empty() && handle_stack_.top()());
      }
      mutex.unlock();
    }
    if (terminated_) return;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

TimeTrigger::TimeTrigger() : terminated_(false) {}

TimeTrigger::~TimeTrigger()
{
  terminated_ = true;
  itv_sec_ = 0;
  // TODO: Is it possible thread_ terminates between the two conditions?
  if (thread_ && thread_->joinable())
    thread_->join();
}

void TimeTrigger::Time(const uint32_t& interval)
{
  bool not_this_thread = !thread_ || thread_->get_id() != std::this_thread::get_id();
  if (thread_ && not_this_thread && thread_->joinable()) thread_->join();
  itv_sec_ = interval;
  start_time_ = time(NULL);
  terminated_ = false;
  if (not_this_thread) thread_ = std::make_unique<std::thread>(std::bind(&TimeTrigger::ThreadFunc, this));
}

void TimeTrigger::Terminate()
{
  terminated_ = true;
}

void TimeTrigger::push_handle_to_stack(std::function<bool()> handle)
{
  handle_stack_.push(handle);
}

void TimeTrigger::pop()
{
  handle_stack_.pop();
}

void TimeTrigger::clear_stack()
{
  while (!handle_stack_.empty())
  {
    handle_stack_.pop();
  }
}