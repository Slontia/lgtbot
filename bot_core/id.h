// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <limits>

#define DEFINE_INTEGER_ID(idname, type) \
struct idname \
{ \
 public: \
  constexpr idname() : id_(std::numeric_limits<type>::max()) {} \
  constexpr idname(const type id) : id_(id) {} \
  constexpr idname(const idname&) = default; \
  constexpr idname& operator=(const type id) \
  { \
    id_ = id; \
    return *this; \
  } \
  constexpr idname& operator=(const idname&) = default; \
  operator type() const { return id_; } \
  auto operator<=>(const type id) const { return id_ <=> id; } \
  idname& operator++() \
  { \
    id_ += 1; \
    return *this; \
  } \
  template <typename Outputter> \
  friend auto& operator<<(Outputter& outputter, const idname id) { return outputter << id.id_; } \
  template <typename Inputter> \
  friend auto& operator>>(Inputter& inputter, idname& id) { return inputter >> id.id_; } \
  bool IsValid() const { return std::numeric_limits<type>::max() != id_; } \
  const type& Get() const { return id_; } \
\
 private: \
  type id_; \
}

DEFINE_INTEGER_ID(MatchID, uint32_t);
DEFINE_INTEGER_ID(ComputerID, uint32_t);
DEFINE_INTEGER_ID(PlayerID, uint32_t);

#define DEFINE_STRING_ID(idname) \
struct idname \
{ \
 public: \
  idname() = default; \
  idname(std::string id) : id_(std::move(id)) {} \
  idname(const std::string_view& id) : id_(id) {} \
  idname(const char* id) : id_(id) {} \
  idname(const idname&) = default; \
  idname(idname&&) = default; \
  idname& operator=(std::string id) \
  { \
    id_ = std::move(id); \
    return *this; \
  } \
  idname& operator=(const std::string_view& id) \
  { \
    id_ = id; \
    return *this; \
  } \
  idname& operator=(const idname&) = default; \
  idname& operator=(idname&&) = default; \
  operator std::string() const { return id_; } \
  auto operator<=>(const std::string& id) const { return id_ <=> id; } \
  auto operator<=>(const std::string_view& id) const { return id_ <=> id; } \
  auto operator<=>(const idname&) const = default; \
  template <typename Outputter> \
  friend auto& operator<<(Outputter& outputter, const idname& id) { return outputter << id.id_; } \
  template <typename Inputter> \
  friend auto& operator>>(Inputter& inputter, idname& id) { return inputter >> id.id_; } \
  bool IsValid() const { return !id_.empty(); } \
  const std::string& GetStr() const { return id_; } \
  const char* GetCStr() const { return id_.c_str(); } \
\
 private: \
  std::string id_; \
}

DEFINE_STRING_ID(UserID);
DEFINE_STRING_ID(GroupID);
