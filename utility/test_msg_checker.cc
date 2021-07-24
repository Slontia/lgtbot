#ifdef ENUM_BEGIN
#ifdef ENUM_MEMBER
#ifdef ENUM_END

ENUM_BEGIN(MyEnum)
ENUM_MEMBER(MyEnum, one)
ENUM_MEMBER(MyEnum, two)
ENUM_MEMBER(MyEnum, three)
ENUM_END(MyEnum)

#endif
#endif
#endif

#ifndef TEST_MSG_CHECKER_CC
#define TEST_MSG_CHECKER_CC

#include <array>
#include <string_view>
#include <map>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include "msg_checker.h"

#define ENUM_FILE "test_msg_checker.cc"
#include "extend_enum.h"

class TestMsgChecker : public testing::Test
{

};

template <typename Checker, typename Str>
void CheckSucc(const Checker& checker, const Str& str, const typename Checker::arg_type& arg)
{
    MsgReader reader(str);
    const auto ret = checker.Check(reader);
    ASSERT_TRUE(ret.has_value());
    ASSERT_EQ(arg, *ret);
}

template <typename Checker, typename Str> requires std::is_void_v<typename Checker::arg_type>
void CheckSucc(const Checker& checker, const Str& str)
{
    MsgReader reader(str);
    const auto ret = checker.Check(reader);
    ASSERT_TRUE(ret);
}

template <typename Checker, typename Str>
void CheckFail(const Checker& checker, const Str& str)
{
    MsgReader reader(str);
    const auto ret = checker.Check(reader);
    ASSERT_FALSE(ret.has_value());
}

template <typename Checker, typename Str> requires std::is_void_v<typename Checker::arg_type>
void CheckFail(const Checker& checker, const Str& str)
{
    MsgReader reader(str);
    const auto ret = checker.Check(reader);
    ASSERT_FALSE(ret);
}

TEST_F(TestMsgChecker, any_checker)
{
    AnyArg checker;
    CheckFail(checker, "");
    CheckFail(checker, " ");
    CheckFail(checker, "\t");
    CheckFail(checker, "\n");
    CheckFail(checker, " \n \t\t\n\t ");
    CheckSucc(checker, "abc", "abc");
    CheckSucc(checker, "  abc def", "abc");
    CheckSucc(checker, "  中文测试 表现不错", "中文测试");
    CheckSucc(checker, "Test\ttab", "Test");
}

TEST_F(TestMsgChecker, test_bool_checker)
{
    BoolChecker checker_1("true", "false");
    CheckFail(checker_1, "");
    CheckFail(checker_1, " ");
    CheckFail(checker_1, "\t");
    CheckFail(checker_1, "\n");
    CheckFail(checker_1, " \n \t\t\n\t ");
    CheckFail(checker_1, "invalid");
    CheckSucc(checker_1, "true", true);
    CheckSucc(checker_1, "false", false);
    CheckSucc(checker_1, "true invlid", true);
    CheckSucc(checker_1, "false invlid", false);

    BoolChecker checker_2("true invalid", " ");
    CheckFail(checker_2, "");
    CheckFail(checker_2, " ");
    CheckFail(checker_2, "\t");
    CheckFail(checker_2, "\n");
    CheckFail(checker_2, " \n \t\t\n\t ");
    CheckFail(checker_2, "invalid");
    CheckFail(checker_2, "true");
    CheckFail(checker_2, "false");
    CheckFail(checker_2, "true invlid");
    CheckFail(checker_2, "false invlid");
}

TEST_F(TestMsgChecker, test_alter_checker)
{
    AlterChecker<int> checker({
        { "", 1 },
        { " ", 2},
        { "\n", 3 },
        { "\t", 4 },
        { "5", 5 },
        { "6 invalid", 6 },
        { " 7", 7 }
    });
    CheckFail(checker, "");
    CheckFail(checker, " ");
    CheckFail(checker, "\t");
    CheckFail(checker, "\n");
    CheckFail(checker, " \n \t\t\n\t ");
    CheckFail(checker, "6 invalid");
    CheckFail(checker, " 7");
    CheckSucc(checker, " 5", 5);
    CheckSucc(checker, " 5 other", 5);
}

TEST_F(TestMsgChecker, test_arith_checker)
{
    ArithChecker<int> checker(-1, 1);
    CheckFail(checker, "");
    CheckFail(checker, " ");
    CheckFail(checker, "\t");
    CheckFail(checker, "\n");
    CheckFail(checker, " \n \t\t\n\t ");
    CheckFail(checker, "-2");
    CheckFail(checker, "2");
    CheckFail(checker, "zero");
    CheckSucc(checker, "-1", -1);
    CheckSucc(checker, "0", 0);
    CheckSucc(checker, "-0", 0);
    CheckSucc(checker, "+0", 0);
    CheckSucc(checker, "1", 1);
    CheckSucc(checker, "+1", 1);
}

struct Obj
{
    int v_;
    auto operator<=>(const Obj& obj) const = default;
    friend std::istream& operator>>(std::istream& os, Obj& obj)
    {
        os >> obj.v_;
        ++obj.v_;
        return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const Obj& obj)
    {
        return os;
    }
};

TEST_F(TestMsgChecker, test_basic_checker)
{
    BasicChecker<Obj> checker;
    CheckFail(checker, "");
    CheckFail(checker, " ");
    CheckFail(checker, "\t");
    CheckFail(checker, "\n");
    CheckFail(checker, " \n \t\t\n\t ");
    CheckFail(checker, "zero");
    CheckSucc(checker, "-1", {0});
    CheckSucc(checker, "0", {1});
    CheckSucc(checker, "-0", {1});
    CheckSucc(checker, "+0", {1});
    CheckSucc(checker, "1", {2});
    CheckSucc(checker, "+1", {2});
}

TEST_F(TestMsgChecker, test_void_checker)
{
    VoidChecker checker("test");
    CheckFail(checker, "");
    CheckFail(checker, " ");
    CheckFail(checker, "\t");
    CheckFail(checker, "\n");
    CheckFail(checker, " \n \t\t\n\t ");
    CheckFail(checker, "failed");
    CheckSucc(checker, "test");
    CheckSucc(checker, "test other");
}

TEST_F(TestMsgChecker, test_repeatable_normal_checker)
{
    RepeatableChecker<ArithChecker<int>> checker(-1, 1);
    CheckFail(checker, "-2");
    CheckFail(checker, "2");
    CheckFail(checker, "zero");
    CheckFail(checker, "1 zero");
    CheckFail(checker, "0 1 2");
    CheckSucc(checker, "", {});
    CheckSucc(checker, " ", {});
    CheckSucc(checker, "\t", {});
    CheckSucc(checker, "\n", {});
    CheckSucc(checker, " \n \t\t\n\t ", {});
    CheckSucc(checker, "-1", {-1});
    CheckSucc(checker, "0", {0});
    CheckSucc(checker, "-0", {0});
    CheckSucc(checker, "+0", {0});
    CheckSucc(checker, "1", {1});
    CheckSucc(checker, "+1", {1});
    CheckSucc(checker, "1 -1", {1, -1});
    CheckSucc(checker, "1 0", {1, 0});
    CheckSucc(checker, "1 -0", {1, 0});
    CheckSucc(checker, "1 +0", {1, 0});
    CheckSucc(checker, "1 1", {1, 1});
    CheckSucc(checker, "1 +1", {1, 1});
}

TEST_F(TestMsgChecker, test_flags_checker)
{
    FlagsChecker<MyEnum> checker;
    CheckFail(checker, " one four");
    CheckFail(checker, "four");
    CheckSucc(checker, "", 0b000);
    CheckSucc(checker, " ", 0b000);
    CheckSucc(checker, "\t", 0b000);
    CheckSucc(checker, "\n", 0b000);
    CheckSucc(checker, " \n \t\t\n\t ", 0b000);
    CheckSucc(checker, "one", 0b001);
    CheckSucc(checker, "two", 0b010);
    CheckSucc(checker, "three", 0b100);
    CheckSucc(checker, "two one", 0b011);
    CheckSucc(checker, "two three", 0b110);
    CheckSucc(checker, "three one one", 0b101);
    CheckSucc(checker, "three one two", 0b111);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}

#endif
