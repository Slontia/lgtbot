#include "stdafx.h"
#include "CppUnitTest.h"
#include "../BotCore/lgtbot.h"
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void at_cq(const QQ& usr_qq, char* str, const size_t& len)
{
  sprintf_s(str, len, "[@%lld]", usr_qq);
}

void send_private_msg(const QQ& usr_qq, const char* msg)
{
  std::cout << std::endl << "<private> <" << usr_qq << ">" << std::endl << msg << std::endl;
}

void send_group_msg(const QQ& group_qq, const char* msg)
{
  std::cout << std::endl << "<group> <" << group_qq << ">" << std::endl << msg << std::endl;
}

void send_discuss_msg(const QQ& discuss_qq, const char* msg)
{
  std::cout << std::endl << "<discuss> <" << discuss_qq << ">" << std::endl << msg << std::endl;
}

namespace UnitTest
{		
	TEST_CLASS(UnitTest1)
	{
  private:

    void init_bot()
    {
      LGTBOT::set_this_qq(114514);
      LGTBOT::set_at_cq_callback((AT_CALLBACK) at_cq);
      LGTBOT::set_send_discuss_msg_callback(send_discuss_msg);
      LGTBOT::set_send_group_msg_callback(send_group_msg);
      LGTBOT::set_send_private_msg_callback(send_private_msg);
    }

	public:

		TEST_METHOD(TestMethod1)
		{
      init_bot();
      LGTBOT::LoadGames();
      LGTBOT::Request(PRIVATE_MSG, INVALID_QQ, 654867229, "#сно╥ап╠М");

		}

	};
}