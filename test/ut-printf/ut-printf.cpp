#if defined(_MSC_VER)
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include "gtest/gtest.h"

#include "capo/printf.hpp"
#include "capo/output.hpp"

#include <string>
#include <string.h>

////////////////////////////////////////////////////////////////

#define TEST_METHOD(TEST_NAME) TEST(printf, TEST_NAME)

////////////////////////////////////////////////////////////////

TEST_METHOD(printf)
{
    char c = 'A';
    char buf[100];

    capo::printf(std::cout, "1234567%s%c\n", " ", c);
    capo::printf([&buf](const std::string& str)
    {
        strcpy(buf, str.c_str());
    }, "1234567%s%c\n", " ", c);
    EXPECT_STREQ("1234567 A\n", buf);

    EXPECT_THROW(capo::printf(std::cout, "%s\n"    , 123       ), std::invalid_argument);
    EXPECT_THROW(capo::printf(std::cout, "%d, %s\n", 123       ), std::invalid_argument);
    EXPECT_THROW(capo::printf(std::cout, "%d\n"    , 123, "123"), std::invalid_argument);
}

////////////////////////////////////////////////////////////////

std::string buf;
void out(const std::string& str)
{
    buf = str;
    std::cout << buf << std::endl;
}

TEST_METHOD(output)
{
    capo::output(out, "Hello, {0}!", "World");
    EXPECT_STREQ("Hello, World!", buf.c_str());

    capo::output(out, "{0} {1:.1} {2:04.} {3:04.04}", 123.321, 123.321, 123.321, 123.321);
    EXPECT_STREQ("123.321000 123.3 0123 123.3210", buf.c_str());

    capo::output(out, "{0} {0:.1} {0:04.} {0:04.04}", 123.321);
    EXPECT_STREQ("123.321000 123.3 0123 123.3210", buf.c_str());

    capo::output(out, "{0}, {1}, {2}, {3}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 1, 2, 3", buf.c_str());

    capo::output(out, "{0}, {3}, {1}, {2}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 3, 1, 2", buf.c_str());
}

TEST_METHOD(space)
{
    capo::output(out, "Hello, {0  }!", "World");
    EXPECT_STREQ("Hello, World!", buf.c_str());

    capo::output(out, "{ 0 } {0 \t : .1} { 0:  04. } { 0 :04.04}", 123.321);
    EXPECT_STREQ("123.321000 123.3 0123 123.3210", buf.c_str());

    capo::output(out, "{0}, {3}{2}{1}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 321", buf.c_str());
}

TEST_METHOD(no_placeholder)
{
    capo::output(out, "{}, {}, {}, {}", 0, 1, 2, 3);
    EXPECT_STREQ("0, 1, 2, 3", buf.c_str());

    capo::output(out, "{_}, {:}, { }, {\t}, {-}, { \t }, {gdgd}", 0, 1, 2);
    EXPECT_STREQ("0, 0, 0, 1, 0, 2, 0", buf.c_str());

    capo::output(out, "{{{}, {}}}, {{{}}}, {}", 0, 1, 2, 3);
    EXPECT_STREQ("{0, 1}, {2}, 3", buf.c_str());

    EXPECT_THROW(capo::output(out, "{{}, {}, {{}}, {}", 0, 1, 2, 3), std::invalid_argument);
    EXPECT_THROW(capo::output(out, "{}, {}}, {{}}, {}", 0, 1, 2, 3), std::invalid_argument);
    EXPECT_THROW(capo::output(out, "{}, {", 0, 1), std::invalid_argument);
    EXPECT_THROW(capo::output(out, "{}, {}{}", 0, 1), std::invalid_argument);
    EXPECT_THROW(capo::output(out, "{}, {}}{}", 0, 1), std::invalid_argument);
    EXPECT_THROW(capo::output(out, "Hello, {1}!", "World"), std::invalid_argument);
    EXPECT_THROW(capo::output(out, "Hello, {0}!", "World", 123), std::invalid_argument);
}

TEST_METHOD(default_out)
{
    capo::output("Hello, World!\n");
    capo::output("Hello, {0}!\n", "World");
    capo::output((char*)"Hello, {0}!\n", "World");
    capo::output("{}, {}, {}, {}\n", "World", 0, 1, 2);
}
