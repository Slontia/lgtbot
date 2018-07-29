#pragma once

#include <iostream>

class GamePlayer
{
public:
  GamePlayer(int64_t qqid) : qqid_(qqid), score_(0) {}

protected:
  int score_;
  int64_t qqid_;
};