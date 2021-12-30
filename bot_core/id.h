// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#define DEFINE_ID(idname) \
struct idname \
{ \
 public: \
  constexpr idname() : id_(UINT64_MAX) {} \
  constexpr idname(const uint64_t id) : id_(id) {} \
  constexpr idname(const idname&) = default; \
  constexpr idname& operator=(const uint64_t id) \
  { \
    id_ = id; \
    return *this; \
  } \
  constexpr idname& operator=(const idname&) = default; \
  operator uint64_t() const { return id_; } \
  auto operator<=>(const uint64_t id) const { return id_ <=> id; } \
  idname& operator++() \
  { \
    id_ += 1; \
    return *this; \
  } \
  template <typename Outputter> \
  friend auto& operator<<(Outputter& outputter, const idname id) { return outputter << id.id_; } \
  template <typename Inputter> \
  friend auto& operator>>(Inputter& inputter, idname& id) { return inputter >> id.id_; } \
  bool IsValid() const { return UINT64_MAX != id_; } \
  const uint64_t& Get() const { return id_; } \
\
 private: \
  uint64_t id_; \
}

DEFINE_ID(UserID);
DEFINE_ID(GroupID);
DEFINE_ID(MatchID);
DEFINE_ID(ComputerID);
DEFINE_ID(PlayerID);
