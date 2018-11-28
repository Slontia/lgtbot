#pragma once

#include <iostream>

class GamePlayer
{
public:
  GamePlayer() {}

  virtual int get_score() = 0;
};