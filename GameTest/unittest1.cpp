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
    virtual std::pair<bool, int> Check(MsgReader& reader) const
    {
      if (!reader.HasNext()) { return {false, 0}; }
      try { return {true, std::stoi(reader.NextArg())}; }
      catch (...) { return {false, 0}; } 
    };
  };

  class StringChecker : public MsgArgChecker<std::vector<std::string>>
  {
  public:
    virtual std::string FormatInfo() const { return "<字符串...>"; }
    virtual std::string ExampleInfo() const { return "森高 是个 大帅哥"; }
    virtual std::pair<bool, std::vector<std::string>> Check(MsgReader& reader) const
    {
      std::vector<std::string> ret;
      while (reader.HasNext())
      {
        ret.push_back(reader.NextArg());
      }
      return {!ret.empty(), ret};
    };
  };

	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			auto f = [](const int) -> std::string { return "aaa"; };
			std::function f1{ f };
			std::shared_ptr<MsgCommand<std::string>> command = Make(f,
				//std::shared_ptr<MsgCommand<void>> command = std::make_shared<MsgCommandImpl>(std::function{ f },
				//std::make_unique<IntChecker>(),
								   //std::make_unique<IntChecker>(),
								   //std::make_unique<MsgArgChecker<void>>("a"),
				std::make_unique<IntChecker>());
			const auto test = [&](std::string s)
			{
				MsgReader reader(s);
				Logger::WriteMessage(command->CallIfValid(reader).second.c_str());
			};
			test("1 2 c");
			test("1 2");
			test("1");
			test("a");
		}
	};

}