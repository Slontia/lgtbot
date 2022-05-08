// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#ifdef ENUM_FILE

#include <string>
#include <array>
#include <optional>
#include <map>
#include <type_traits>
#include <bitset>

#define INNER_ENUM(name) __##name##InnerEnum
#define INNER_CONSTANT(name) __##name##InnerConstant

#define ENUM_BEGIN(name) enum class INNER_ENUM(name) : uint32_t {
#define ENUM_MEMBER(name, member) member,
#define ENUM_END(name) \
    name##_MAX_, \
    name##_INVALID_ = UINT32_MAX \
};
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) \
class name; \
\
template <INNER_ENUM(name) e> \
class INNER_CONSTANT(name) \
{ \
  public: \
    constexpr operator INNER_ENUM(name)() const { return e; } \
}; \
\
class name \
{ \
  public:
#define ENUM_MEMBER(name, member) \
    static constexpr const INNER_CONSTANT(name)<INNER_ENUM(name)::member> member{};
#define ENUM_END(name) \
\
    constexpr name() : v_(INNER_ENUM(name)::name##_INVALID_) {} \
\
    template <INNER_ENUM(name) e> \
    constexpr name(const INNER_CONSTANT(name)<e>) : v_(e) {} \
\
    constexpr name(const INNER_ENUM(name)& v) : v_(v) {} \
\
    explicit constexpr name(const uint32_t i) : v_(static_cast<INNER_ENUM(name)>(i)) {} \
\
    constexpr auto operator<=>(const name&) const = default; \
\
    template <INNER_ENUM(name) e> \
    constexpr auto operator<=>(const INNER_CONSTANT(name)<e>) const { return v_ <=> e; }; \
\
    constexpr auto operator<=>(const INNER_ENUM(name)& e) const { return v_ <=> e; }; \
\
    template <typename OS> \
    friend inline decltype(auto) operator<<(OS& os, const name& e) { return os << e.ToString(); } \
\
    inline const char* ToString() const; \
\
    constexpr auto ToUInt() const { return static_cast<uint32_t>(v_); } \
\
    explicit constexpr operator uint32_t() const { return static_cast<uint32_t>(v_); } \
\
    constexpr operator INNER_ENUM(name)() const { return v_; } \
\
    constexpr static size_t Count() { return static_cast<uint32_t>(INNER_ENUM(name)::name##_MAX_); } \
\
    inline static const std::array<name, static_cast<uint32_t>(INNER_ENUM(name)::name##_MAX_)>& Members(); \
\
    inline static const std::map<std::string, name>& ParseMap(); \
\
    inline static std::optional<name> Parse(const std::string& str); \
\
    constexpr static name Condition(const bool cond, const name _1, const name _2) { return cond ? _1 : _2; } \
\
    template <INNER_ENUM(name) ...MEMBERS> class SubSet; \
\
    class BitSet; \
  private: \
    INNER_ENUM(name) v_; \
}; \
\
template <INNER_ENUM(name) ...MEMBERS> \
class name::SubSet : public name \
{ \
private: \
    using Tuple = std::tuple<INNER_CONSTANT(name)<MEMBERS>...>; \
\
public: \
    constexpr SubSet() {} \
\
    template <INNER_ENUM(name) ...SUBSET_MEMBERS> requires requires () { ((std::get<INNER_CONSTANT(name)<SUBSET_MEMBERS>>(Tuple())), ...); } \
    constexpr SubSet(const SubSet<SUBSET_MEMBERS...> e) : name(e) {} \
\
    template <INNER_ENUM(name) e> requires requires () { std::get<INNER_CONSTANT(name)<e>>(Tuple()); } \
    constexpr SubSet(const INNER_CONSTANT(name)<e>) : name(e) {} \
\
    explicit constexpr operator uint32_t() const { return static_cast<uint32_t>(v_); } \
\
    constexpr operator INNER_ENUM(name)() const { return v_; } \
\
    constexpr static name::SubSet<MEMBERS...> Condition(const bool cond, const name::SubSet<MEMBERS...> _1, const name::SubSet<MEMBERS...> _2) { return cond ? _1 : _2; } \
}; \
\
class name::BitSet : protected std::bitset<name::Count()> \
{ \
  public: \
    using std::bitset<name::Count()>::bitset; \
\
    bool operator==(const BitSet& rhs) const = default; \
\
    constexpr bool operator[](const name e) const { return std::bitset<name::Count()>::operator[](static_cast<uint32_t>(e)); } \
\
    decltype(auto) operator[](const name e) { return std::bitset<name::Count()>::operator[](static_cast<uint32_t>(e)); } \
};
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) \
inline const char* name::ToString() const \
{ \
    static const std::array<const char*, static_cast<uint32_t>(INNER_ENUM(name)::name##_MAX_)> strings = {
#define ENUM_MEMBER(name, member) #member,
#define ENUM_END(name) \
    }; \
    return strings[static_cast<uint32_t>(v_)]; \
}
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) \
inline const std::array<name, static_cast<uint32_t>(INNER_ENUM(name)::name##_MAX_)>& name::Members() \
{ \
    static std::array<name, static_cast<uint32_t>(INNER_ENUM(name)::name##_MAX_)> members = {
#define ENUM_MEMBER(name, member) name::member,
#define ENUM_END(name) \
    }; \
    return members; \
}
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) \
inline const std::map<std::string, name>& name::ParseMap() \
{ \
    static std::map<std::string, name> parser = {
#define ENUM_MEMBER(name, member)  { #member, name::member },
#define ENUM_END(name) \
    }; \
    return parser; \
}
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) \
inline std::optional<name> name::Parse(const std::string& str) \
{ \
    const auto it = ParseMap().find(str); \
    if (it == ParseMap().end()) { \
        return std::nullopt; \
    } \
    return it->second; \
}
#define ENUM_MEMBER(name, member)
#define ENUM_END(name)
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#undef INNER_ENUM
#undef ENUM_FILE
#endif
