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
    virtual std::string FormatInfo() const { return "<ÕûÊý>"; }
    virtual std::string ExampleInfo() const { return "50"; }
    virtual std::pair<bool, int> Check(MsgReader& reader) const
    {
      if (!reader.HasNext()) { return {false, 0}; }
      try { return {true, std::stoi(reader.NextArg())}; }
      catch (...) { return {false, 0}; } 
    };
  };

	TEST_CLASS(UnitTest1)
	{
	public:
		
    TEST_METHOD(TestMethod1)
    {
      const auto f = [](const auto...) {};
      std::shared_ptr<MsgCommand> command =
        MsgCommand::Make(std::move(f),
                         //std::make_unique<IntChecker>(),
                         //std::make_unique<IntChecker>(),
                         std::make_unique<MsgArgChecker<void>>("a"));
      const auto test = [&](std::string s)
      {
        MsgReader reader(s);
        Logger::WriteMessage(std::to_string(command->CallIfValid(reader)).c_str());
      };
      test("1 2 c");
      test("1 2");
      test("1");
      test("a");
    }
	};
}