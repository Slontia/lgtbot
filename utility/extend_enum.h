#ifdef ENUM_FILE

#define ENUM_BEGIN(name) \
class name \
{ \
  public: \
    enum InnerEnum : uint32_t {
#define ENUM_MEMBER(name, member) member,
#define ENUM_END(name) \
        name##_MAX_, \
        name##_INVALID_ = UINT32_MAX \
    }; \
 \
    name() : v_(name##_INVALID_) {} \
 \
    name(const InnerEnum& v) : v_(v) {} \
 \
    constexpr auto operator<=>(const name&) const = default; \
 \
    constexpr auto operator<=>(const name::InnerEnum& e) const { return v_ <=> e; }; \
 \
    constexpr const char* ToString() const { return strings_[v_]; } \
 \
    constexpr auto ToUInt() const { return v_; } \
 \
    constexpr static size_t Count() { return name##_MAX_; } \
 \
    static const std::array<name, name##_MAX_>& Members() { return members_; } \
 \
    static std::optional<name> Parse(const std::string_view& str) \
    { \
        const auto it = parser_.find(str); \
        if (it == parser_.end()) { \
           return std::nullopt; \
        } \
        return it->second; \
    } \
 \
    explicit operator uint32_t() const { return static_cast<uint32_t>(v_); } \
 \
    template <typename OS> \
    friend inline decltype(auto) operator<<(OS& os, const name& e) { return os << e.ToString(); } \
 \
  private: \
    InnerEnum v_; \
 \
    static std::array<const char*, name##_MAX_> strings_; \
    static std::array<name, name##_MAX_> members_; \
    static std::map<std::string_view, name> parser_; \
};
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) std::array<const char*, name::name##_MAX_> name::strings_ = {
#define ENUM_MEMBER(name, member) #member,
#define ENUM_END(name) };
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) std::array<name, name::name##_MAX_> name::members_ = {
#define ENUM_MEMBER(name, member) member,
#define ENUM_END(name) };
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#define ENUM_BEGIN(name) std::map<std::string_view, name> name::parser_ = {
#define ENUM_MEMBER(name, member)  { #member, member },
#define ENUM_END(name) };
#include ENUM_FILE
#undef ENUM_BEGIN
#undef ENUM_MEMBER
#undef ENUM_END

#undef ENUM_FILE
#endif
