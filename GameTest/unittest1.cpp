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
      const std::function<std::string(const bool b, const int i)> f = [](const int b, const int i) { return std::to_string(b ? -i : i); };
      std::shared_ptr<MsgCommand<std::string(const bool)>> command = MakeCommand<std::string(const bool)>(f, std::make_unique<IntChecker>());
			const auto test = [&](std::string s)
			{
				MsgReader reader(s);
        Logger::WriteMessage(std::to_string(command->CallIfValid(reader, std::tuple{ true }).has_value()).c_str());
			};
			test("1 2 c");
			test("1 2");
      test("1");
			test("a");
		}
	};

}