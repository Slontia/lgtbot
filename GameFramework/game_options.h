#pragma once
#include "msg_checker.h"
#include <string_view>
#include <array>

#ifndef GAME_OPTIONS_HEADER
#error GAME_OPTIONS_HEADER is not defined
#endif

#define OPTION_(name) OPTION_##name
#define CHECKER_(name) (*std::get<OPTION_(name) * 2 + 1>(options_))
#define VALUE_(name) (std::get<OPTION_(name) * 2>(options_))
#define GET_VALUE(option, name) (option.Option2Value<option.Name2Option(#name)>())
#define GET_VALUE_THIS(name) (Option2Value<Name2Option(#name)>)

class GameOption
{
 private:
	enum Option : int
	{
		INVALID_OPTION = -1,
#define GAME_OPTION(_0, name, _1, _2) OPTION_(name),
#include GAME_OPTIONS_HEADER
#undef GAME_OPTION
		OPTION_NUM
	};

 public:
	template <typename T> class foo;
	 GameOption() : options_
	 {
#define GAME_OPTION(_0, _1, checker, default_value) default_value, std::move(checker),
#include GAME_OPTIONS_HEADER
#undef GAME_OPTION
	 }, infos_
	 {
#define GAME_OPTION(description, name, _0, _1) [this]\
{\
	std::stringstream ss;\
	ss << description << std::endl;\
	ss << "格式：" << #name << " " << std::get<1>(options_)->FormatInfo() << std::endl;\
	ss << "例如：" << #name << " " << std::get<1>(options_)->ExampleInfo() << std::endl;\
	return ss.str();\
}(),
#include GAME_OPTIONS_HEADER
#undef GAME_OPTION
	 } {};

  bool IsValidPlayerNum(const uint64_t player_num) const;

	constexpr Option Name2Option(std::string_view read_name) const
	{
#define GAME_OPTION(_0, name, _1, _2) if (#name == read_name) { return OPTION_(name); }
#include GAME_OPTIONS_HEADER
#undef GAME_OPTION
		return Option::INVALID_OPTION;
	}

	template <Option op>
	const auto Option2Value() const
	{
		static_assert(op == Option::INVALID_OPTION, "Unexpected option");
		return std::get<op * 2>(options_);
	}

	bool SetOption(MsgReader& msg_reader)
	{
#define GAME_OPTION(_0, name, _1, _2)\
    static_assert(!std::is_void_v<std::decay<decltype(CHECKER_(name))>::type::arg_type>, "Checker cannot be void checker");\
		if (msg_reader.Reset(); #name == msg_reader.NextArg())\
		{\
			auto value = CHECKER_(name).Check(msg_reader);\
			if (value.has_value() && !msg_reader.HasNext())\
			{\
				VALUE_(name) = *value;\
				return true;\
			}\
		}
#include GAME_OPTIONS_HEADER
#undef GAME_OPTION
		return false;
	}

	const auto Infos() const { return infos_; }

	const std::string StatusInfo() const;

 private:
	 decltype(std::tuple // 这里是int checker int checker这么排列的
	 {
 #define GAME_OPTION(_0, _1, checker, default_value)\
			static_cast<std::decay<decltype(*checker)>::type::arg_type>(default_value),\
			static_cast<std::unique_ptr<MsgArgChecker<std::decay<decltype(*checker)>::type::arg_type>>>(checker),
 #include GAME_OPTIONS_HEADER
 #undef GAME_OPTION
	 }) options_;
	 std::array<std::string, Option::OPTION_NUM> infos_;
};

#undef OPTION_
#undef VALUE_
#undef CHECKER_

