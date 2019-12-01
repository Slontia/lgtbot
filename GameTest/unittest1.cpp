#include "stdafx.h"
#include "CppUnitTest.h"
#include "../new-rock-paper-scissors/msg_checker.h"
#include <memory>
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameTest
{
  template <>
  class MsgArgChecker<int>
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
      const auto f = [](const int value) { std::cout << "get " << value << std::endl; };
      MsgReader reader("asdf, asdf");
      MsgCommand* command = new MsgCommandImpl<decltype(f), int>(std::move(f), std::make_unique<MsgArgChecker<int>>());
      command->CallIfValid(reader);
    }
	};
}