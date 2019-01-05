#include "stdafx.h"
#include "CppUnitTest.h"
#include "lgtbot.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
      LGTBOT::LoadGames();
      LGTBOT::Request(PRIVATE_MSG, INVALID_QQ, 654867229, "#сно╥ап╠М", 3);

		}

	};
}