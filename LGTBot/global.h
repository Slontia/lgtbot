#pragma once

#include <mutex>


extern std::mutex mutex;

enum MessageType
{
  PRIVATE_MSG,
  PUBLIC_MSG
};
