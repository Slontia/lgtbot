#include "stdafx.h"
#include "CppUnitTest.h"
#include "../new-rock-paper-scissors/msg_checker.h"
#include <memory>
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameTest
{
  class IntChecker : public MsgArgChecker<int>
  {
  public:
    virtual std::string FormatInfo() const { return "<整数>"; }
    virtual std::string ExampleInfo() const { return "50"; }
    virtual std::optional<int> Check(MsgReader& reader) const
    {
      if (!reader.HasNext()) { return {}; }
      try { return std::stoi(reader.NextArg()); }
      catch (...) { return {}; }
    };
  };

  class StringChecker : public MsgArgChecker<std::vector<std::string>>
  {
  public:
    virtual std::string FormatInfo() const { return "<字符串...>"; }
    virtual std::string ExampleInfo() const { return "森高 是个 大帅哥"; }
    virtual std::optional<std::vector<std::string>> Check(MsgReader& reader) const
    {
      std::vector<std::string> ret;
      while (reader.HasNext())
      {
        ret.push_back(reader.NextArg());
      }
      return !ret.empty() ? ret : std::optional<std::vector<std::string>>();
    };
  };

	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			//const std::function<std::string(const int)> f = [](const int a) -> std::string { return std::to_string(a); };
      const std::function<std::string(const bool b, const int i)> f = [](const bool b, const int i) { return std::to_string(b ? -i : i); };
      const std::function<std::string(const bool b, const std::optional<int> i)> fi = [](const bool b, const std::optional<int> i) { return ""; };
      std::shared_ptr<MsgCommand<std::string(const bool)>> command = MakeCommand<std::string(const bool)>(f, std::make_unique<IntChecker>());
      std::shared_ptr<MsgCommand<std::string(const bool)>> command2 = MakeCommand<std::string(const bool)>(f, std::make_unique<ArithChecker<int, 0, 100>>());
      std::shared_ptr<MsgCommand<std::string(const bool)>> command4 = MakeCommand<std::string(const bool)>(fi, std::make_unique<ArithChecker<int, 0, 100, true>>());
      std::shared_ptr<MsgCommand<std::string()>> command3 = MakeCommand<std::string()>(f, std::make_unique<BoolChecker>("true", "false"), std::make_unique<ArithChecker<int, 0, 100>>());

      const auto test = [&](std::string s, auto& cmd, auto& tuple)
			{
				MsgReader reader(s);
        Logger::WriteMessage(std::to_string(cmd->CallIfValid(reader, std::move(tuple)).has_value()).c_str());
			};
      test("false", command3, std::tuple{});
      test("1", command2, std::tuple{true});
      test("10000", command2, std::tuple{ true });
      test("0", command4, std::tuple{ true });
      test(" ", command4, std::tuple{ true });
		}
	};

}