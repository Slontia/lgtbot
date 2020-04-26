#pragma once

class GameBase
{
public:
  virtual ~GameBase() = default;
  virtual void __cdecl HandleRequest(const uint64_t player_id, const bool is_public, const char* const msg) = 0;
  virtual void HandleTimeout(const bool* const stage_is_over) = 0;
};