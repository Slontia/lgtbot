// Copyright (c) 2018-present, Chang Liu <github.com/slontia>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

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

#define ASSERT_ARG(checker, str, arg) \
do { \
    MsgReader reader(str); \
    const auto ret = checker.Check(reader); \
    ASSERT_TRUE(ret); \
    ASSERT_EQ(arg, *ret); \
} while (0)

#define ASSERT_ARG_VOID(checker, str) \
do { \
    MsgReader reader(str); \
    const auto ret = checker.Check(reader); \
    ASSERT_TRUE(ret); \
} while (0)

#define ASSERT_FAIL(checker, str) \
do { \
    MsgReader reader(str); \
    ASSERT_FALSE(checker.Check(reader)); \
} while (0)

TEST_F(TestMsgChecker, any_checker)
{
    AnyArg checker;
    ASSERT_FAIL(checker, "");
    ASSERT_FAIL(checker, " ");
    ASSERT_FAIL(checker, "\t");
    ASSERT_FAIL(checker, "\n");
    ASSERT_FAIL(checker, " \n \t\t\n\t ");
    ASSERT_ARG(checker, "abc", "abc");
    ASSERT_ARG(checker, "  abc def", "abc");
    ASSERT_ARG(checker, "  中文测试 表现不错", "中文测试");
    ASSERT_ARG(checker, "Test\ttab", "Test");
}

TEST_F(TestMsgChecker, test_bool_checker)
{
    BoolChecker checker_1("true", "false");
    ASSERT_FAIL(checker_1, "");
    ASSERT_FAIL(checker_1, " ");
    ASSERT_FAIL(checker_1, "\t");
    ASSERT_FAIL(checker_1, "\n");
    ASSERT_FAIL(checker_1, " \n \t\t\n\t ");
    ASSERT_FAIL(checker_1, "invalid");
    ASSERT_ARG(checker_1, "true", true);
    ASSERT_ARG(checker_1, "false", false);
    ASSERT_ARG(checker_1, "true invlid", true);
    ASSERT_ARG(checker_1, "false invlid", false);

    BoolChecker checker_2("true invalid", " ");
    ASSERT_FAIL(checker_2, "");
    ASSERT_FAIL(checker_2, " ");
    ASSERT_FAIL(checker_2, "\t");
    ASSERT_FAIL(checker_2, "\n");
    ASSERT_FAIL(checker_2, " \n \t\t\n\t ");
    ASSERT_FAIL(checker_2, "invalid");
    ASSERT_FAIL(checker_2, "true");
    ASSERT_FAIL(checker_2, "false");
    ASSERT_FAIL(checker_2, "true invlid");
    ASSERT_FAIL(checker_2, "false invlid");
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
    ASSERT_FAIL(checker, "");
    ASSERT_FAIL(checker, " ");
    ASSERT_FAIL(checker, "\t");
    ASSERT_FAIL(checker, "\n");
    ASSERT_FAIL(checker, " \n \t\t\n\t ");
    ASSERT_FAIL(checker, "6 invalid");
    ASSERT_FAIL(checker, " 7");
    ASSERT_ARG(checker, " 5", 5);
    ASSERT_ARG(checker, " 5 other", 5);
}

TEST_F(TestMsgChecker, test_arith_checker)
{
    ArithChecker<int> checker(-1, 1);
    ASSERT_FAIL(checker, "");
    ASSERT_FAIL(checker, " ");
    ASSERT_FAIL(checker, "\t");
    ASSERT_FAIL(checker, "\n");
    ASSERT_FAIL(checker, " \n \t\t\n\t ");
    ASSERT_FAIL(checker, "-2");
    ASSERT_FAIL(checker, "2");
    ASSERT_FAIL(checker, "zero");
    ASSERT_ARG(checker, "-1", -1);
    ASSERT_ARG(checker, "0", 0);
    ASSERT_ARG(checker, "-0", 0);
    ASSERT_ARG(checker, "1", 1);
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
    ASSERT_FAIL(checker, "");
    ASSERT_FAIL(checker, " ");
    ASSERT_FAIL(checker, "\t");
    ASSERT_FAIL(checker, "\n");
    ASSERT_FAIL(checker, " \n \t\t\n\t ");
    ASSERT_FAIL(checker, "zero");
    ASSERT_ARG(checker, "-1", Obj{0});
    ASSERT_ARG(checker, "0", Obj{1});
    ASSERT_ARG(checker, "-0", Obj{1});
    ASSERT_ARG(checker, "+0", Obj{1});
    ASSERT_ARG(checker, "1", Obj{2});
    ASSERT_ARG(checker, "+1", Obj{2});
}

TEST_F(TestMsgChecker, test_void_checker)
{
    VoidChecker checker("test");
    ASSERT_FAIL(checker, "");
    ASSERT_FAIL(checker, " ");
    ASSERT_FAIL(checker, "\t");
    ASSERT_FAIL(checker, "\n");
    ASSERT_FAIL(checker, " \n \t\t\n\t ");
    ASSERT_FAIL(checker, "failed");
    ASSERT_ARG_VOID(checker, "test");
    ASSERT_ARG_VOID(checker, "test other");
}

TEST_F(TestMsgChecker, test_repeatable_normal_checker)
{
    RepeatableChecker<ArithChecker<int>> checker(-1, 1);
    ASSERT_FAIL(checker, "-2");
    ASSERT_FAIL(checker, "2");
    ASSERT_FAIL(checker, "zero");
    ASSERT_FAIL(checker, "1 zero");
    ASSERT_FAIL(checker, "0 1 2");
    ASSERT_ARG(checker, "", std::vector<int>{});
    ASSERT_ARG(checker, " ", std::vector<int>{});
    ASSERT_ARG(checker, "\t", std::vector<int>{});
    ASSERT_ARG(checker, "\n", std::vector<int>{});
    ASSERT_ARG(checker, " \n \t\t\n\t ", std::vector<int>{});
    ASSERT_ARG(checker, "-1", std::vector<int>{-1});
    ASSERT_ARG(checker, "0", std::vector<int>{0});
    ASSERT_ARG(checker, "-0", std::vector<int>{0});
    ASSERT_ARG(checker, "1", std::vector<int>{1});
    ASSERT_ARG(checker, "1 -1", (std::vector<int>{1, -1}));
    ASSERT_ARG(checker, "1 0", (std::vector<int>{1, 0}));
    ASSERT_ARG(checker, "1 -0", (std::vector<int>{1, 0}));
    ASSERT_ARG(checker, "1 1", (std::vector<int>{1, 1}));
}

TEST_F(TestMsgChecker, test_flags_checker)
{
    FlagsChecker<MyEnum> checker;
    ASSERT_FAIL(checker, " one four");
    ASSERT_FAIL(checker, "four");
    ASSERT_ARG(checker, "", 0b000);
    ASSERT_ARG(checker, " ", 0b000);
    ASSERT_ARG(checker, "\t", 0b000);
    ASSERT_ARG(checker, "\n", 0b000);
    ASSERT_ARG(checker, " \n \t\t\n\t ", 0b000);
    ASSERT_ARG(checker, "one", 0b001);
    ASSERT_ARG(checker, "two", 0b010);
    ASSERT_ARG(checker, "three", 0b100);
    ASSERT_ARG(checker, "two one", 0b011);
    ASSERT_ARG(checker, "two three", 0b110);
    ASSERT_ARG(checker, "three one one", 0b101);
    ASSERT_ARG(checker, "three one two", 0b111);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}

#endif
