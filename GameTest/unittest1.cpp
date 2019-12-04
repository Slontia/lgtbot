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
      //std::string s = reader.NextArg();
      //Logger::WriteMessage(s.c_str());
      try { return {true, std::stoi(reader.NextArg())}; }
      catch (...) { return {false, 0}; } 
    };
  };

	TEST_CLASS(UnitTest1)
	{
	public:
		
    TEST_METHOD(TestMethod1)
    {
      const auto f = [](const int a, const int b)
      {
        std::stringstream ss;
        ss << a << " " << b;
        Logger::WriteMessage(ss.str().c_str());
      };
      MsgCommand* command = new MsgCommandImpl<decltype(f), IntChecker::arg_type, int, void>(std::move(f),
          std::unique_ptr<MsgArgChecker<int>>(new IntChecker()),
          std::unique_ptr<MsgArgChecker<int>>(new IntChecker()),
          std::unique_ptr<MsgArgChecker<void>>(new MsgArgChecker<void>("c")));
      const auto test = [&](std::string s)
      {
        MsgReader reader(s);
        command->CallIfValid(reader);
      };
      test("1 2 c"); 
      //MsgReader reader("aa bb cc");
      //reader.Reset();
      //Logger::WriteMessage(reader.NextArg().c_str());
    }
	};
}