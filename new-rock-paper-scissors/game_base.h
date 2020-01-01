#pragma once
class GameBase
{
public:
  virtual ~GameBase() = default;
  virtual bool HandleRequest(const uint64_t player_id, const bool is_public, char* const msg) = 0;
};