#include "msg_checker.h"
#include <string_view>
#ifdef GAME_OPTION_LIST_HEADER

#define OPTION(name) OPTION_##name

class GameOption
{
 private:
	enum Option : int
	{
		INVALID_OPTION = -1,
#define GAME_OPTION(name, checker, default_value) OPTION(name),
#include GAME_OPTION_LIST_HEADER
#undef GAME_OPTION
	};

 public:
	constexpr Option Name2Option(std::string_view read_name) const
	{
#define GAME_OPTION(name, checker, default_value) if (#name == read_name) { return OPTION(name); }
#include GAME_OPTION_LIST_HEADER
#undef GAME_OPTION
		return Option::INVALID_OPTION;
	}

	template <Option op>
	auto Option2Value()
	{
		static_assert(op == Option::INVALID_OPTION, "Unexpected option");
		return std::get<op>(options_);
	}

#define GetOption(name) Option2Value<Name2Option(name)>()

	template <typename T>
	bool SetOption(std::string_view write_name, const T& value)
	{
#define GAME_OPTION(name, checker, default_value) if (#name == write_name) { std::get<OPTION(name)>(options_) = value; return true; }
#include GAME_OPTION_LIST_HEADER
#undef GAME_OPTION
		return false;
	}

 private:
	decltype(std::tuple
	{
#define GAME_OPTION(name, checker, default_value) std::pair<std::decay<decltype(*checker)>::type::arg_type, std::unique_ptr<MsgArgChecker<std::decay<decltype(*checker)>::type::arg_type>>>(default_value, std::move(checker)),
#include GAME_OPTION_LIST_HEADER
#undef GAME_OPTION
	}) options_
  {
#define GAME_OPTION(name, checker, default_value) std::pair(default_value, std::move(checker)),
#include GAME_OPTION_LIST_HEADER
#undef GAME_OPTION
	};

};

#undef OPTION

#endif // GAME_OPTION_LIST_HEADER

