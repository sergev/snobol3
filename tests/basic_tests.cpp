#include <gtest/gtest.h>

#include <cstring>
#include <sstream>
#include <string>

#include "sno.h"
#include "test_helpers.h"

// Test fixture for tests that may need setup/teardown
class SnobolTest : public ::testing::Test {
protected:
    std::stringstream input_stream;
    std::stringstream output_stream;
    SnobolContext ctx;

    SnobolTest() : ctx(input_stream, output_stream) {}

    void SetUp() override
    {
        // Context is already initialized in constructor
    }

    // Helper method to convert node_t string to std::string
    std::string node_to_string(node_t *str)
    {
        if (str == nullptr) {
            return "";
        }
        std::string result;
        node_t *a = str;
        node_t *b = str->p2;
        while (a != b) {
            a = a->p1;
            result += a->ch;
        }
        return result;
    }

    // Helper method to compare node_t string with C string
    bool node_equals_cstr(node_t *str, const char *cstr)
    {
        if (str == nullptr && cstr == nullptr) {
            return true;
        }
        if (str == nullptr || cstr == nullptr) {
            return false;
        }
        return node_to_string(str) == cstr;
    }
};

// ============================================================================
// Character Classification Tests
// ============================================================================

TEST(CharacterClassificationTest, Operators)
{
    EXPECT_EQ(char_class('+'), CHAR_CLASS_PLUS);     // Plus operator
    EXPECT_EQ(char_class('-'), CHAR_CLASS_MINUS);    // Minus operator
    EXPECT_EQ(char_class('*'), CHAR_CLASS_ASTERISK); // Asterisk operator
    EXPECT_EQ(char_class('/'), CHAR_CLASS_SLASH);    // Division operator
    EXPECT_EQ(char_class('='), CHAR_CLASS_EQUALS);   // Equals sign
}

TEST(CharacterClassificationTest, Delimiters)
{
    EXPECT_EQ(char_class('('), CHAR_CLASS_LPAREN);        // Left parenthesis
    EXPECT_EQ(char_class(')'), CHAR_CLASS_RPAREN);        // Right parenthesis
    EXPECT_EQ(char_class('\''), CHAR_CLASS_STRING_DELIM); // Single quote
    EXPECT_EQ(char_class('"'), CHAR_CLASS_STRING_DELIM);  // Double quote
    EXPECT_EQ(char_class(','), CHAR_CLASS_COMMA);         // Comma
    EXPECT_EQ(char_class('$'), CHAR_CLASS_DOLLAR);        // Dollar sign
}

TEST(CharacterClassificationTest, Whitespace)
{
    EXPECT_EQ(char_class(' '), CHAR_CLASS_WHITESPACE);  // Space
    EXPECT_EQ(char_class('\t'), CHAR_CLASS_WHITESPACE); // Tab
}

TEST(CharacterClassificationTest, RegularCharacters)
{
    EXPECT_EQ(char_class('a'), CHAR_CLASS_OTHER); // Letter
    EXPECT_EQ(char_class('0'), CHAR_CLASS_OTHER); // Digit
    EXPECT_EQ(char_class('@'), CHAR_CLASS_OTHER); // Symbol
}

// ============================================================================
// String Operations Tests
// ============================================================================

TEST_F(SnobolTest, CstrToNode_SimpleString)
{
    node_t *str = ctx.cstr_to_node("hello");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "hello"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, CstrToNode_SingleChar)
{
    node_t *str = ctx.cstr_to_node("a");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "a"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, CstrToNode_NumberString)
{
    node_t *str = ctx.cstr_to_node("12345");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "12345"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Copy_SimpleString)
{
    node_t *orig   = ctx.cstr_to_node("test");
    node_t *copied = ctx.copy(orig);

    ASSERT_NE(orig, nullptr);
    ASSERT_NE(copied, nullptr);
    EXPECT_NE(orig, copied); // Should be different nodes
    EXPECT_TRUE(node_equals_cstr(orig, "test"));
    EXPECT_TRUE(node_equals_cstr(copied, "test"));

    ctx.delete_string(orig);
    ctx.delete_string(copied);
}

TEST_F(SnobolTest, Copy_NullString)
{
    node_t *copied = ctx.copy(nullptr);
    EXPECT_EQ(copied, nullptr);
}

TEST_F(SnobolTest, Copy_ModifyOriginalDoesNotAffectCopy)
{
    node_t *orig   = ctx.cstr_to_node("original");
    node_t *copied = ctx.copy(orig);

    // Delete original
    ctx.delete_string(orig);

    // Copy should still be valid
    EXPECT_TRUE(node_equals_cstr(copied, "original"));

    ctx.delete_string(copied);
}

TEST_F(SnobolTest, Equal_IdenticalStrings)
{
    node_t *str1 = ctx.cstr_to_node("hello");
    node_t *str2 = ctx.cstr_to_node("hello");

    EXPECT_EQ(equal(str1, str2), 0);

    ctx.delete_string(str1);
    ctx.delete_string(str2);
}

TEST_F(SnobolTest, Equal_DifferentStrings)
{
    node_t *str1 = ctx.cstr_to_node("abc");
    node_t *str2 = ctx.cstr_to_node("def");

    EXPECT_NE(equal(str1, str2), 0);

    ctx.delete_string(str1);
    ctx.delete_string(str2);
}

TEST_F(SnobolTest, Equal_FirstStringGreater)
{
    node_t *str1 = ctx.cstr_to_node("def");
    node_t *str2 = ctx.cstr_to_node("abc");

    EXPECT_EQ(equal(str1, str2), 1); // str1 > str2

    ctx.delete_string(str1);
    ctx.delete_string(str2);
}

TEST_F(SnobolTest, Equal_FirstStringLess)
{
    node_t *str1 = ctx.cstr_to_node("abc");
    node_t *str2 = ctx.cstr_to_node("def");

    EXPECT_EQ(equal(str1, str2), -1); // str1 < str2

    ctx.delete_string(str1);
    ctx.delete_string(str2);
}

TEST_F(SnobolTest, Equal_DifferentLengths)
{
    node_t *str1 = ctx.cstr_to_node("abc");
    node_t *str2 = ctx.cstr_to_node("abcd");

    EXPECT_EQ(equal(str1, str2), -1); // str1 < str2 (shorter)

    ctx.delete_string(str1);
    ctx.delete_string(str2);
}

TEST_F(SnobolTest, Equal_NullStrings)
{
    EXPECT_EQ(equal(nullptr, nullptr), 0);

    node_t *str = ctx.cstr_to_node("test");
    EXPECT_EQ(equal(nullptr, str), -1);
    EXPECT_EQ(equal(str, nullptr), 1);

    ctx.delete_string(str);
}

TEST_F(SnobolTest, Cat_SimpleConcatenation)
{
    node_t *str1 = ctx.cstr_to_node("hello");
    node_t *str2 = ctx.cstr_to_node("world");

    node_t *result = ctx.cat(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "helloworld"));

    // Original strings should still be valid (non-destructive)
    EXPECT_TRUE(node_equals_cstr(str1, "hello"));
    EXPECT_TRUE(node_equals_cstr(str2, "world"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Cat_FirstNull)
{
    node_t *str2   = ctx.cstr_to_node("world");
    node_t *result = ctx.cat(nullptr, str2);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "world"));

    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Cat_SecondNull)
{
    node_t *str1   = ctx.cstr_to_node("hello");
    node_t *result = ctx.cat(str1, nullptr);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "hello"));

    ctx.delete_string(str1);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Cat_BothNull)
{
    node_t *result = ctx.cat(nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SnobolTest, Dcat_DestructiveConcatenation)
{
    node_t *str1 = ctx.cstr_to_node("foo");
    node_t *str2 = ctx.cstr_to_node("bar");

    node_t *result = ctx.dcat(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "foobar"));

    // Original strings are deleted, we can only check result
    ctx.delete_string(result);
}

// ============================================================================
// Conversion Functions Tests
// ============================================================================

TEST_F(SnobolTest, Strbin_PositiveNumber)
{
    node_t *str = ctx.cstr_to_node("123");
    int result  = ctx.strbin(str);
    EXPECT_EQ(result, 123);
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Strbin_NegativeNumber)
{
    node_t *str = ctx.cstr_to_node("-456");
    int result  = ctx.strbin(str);
    EXPECT_EQ(result, -456);
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Strbin_Zero)
{
    node_t *str = ctx.cstr_to_node("0");
    int result  = ctx.strbin(str);
    EXPECT_EQ(result, 0);
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Strbin_LargeNumber)
{
    node_t *str = ctx.cstr_to_node("999999");
    int result  = ctx.strbin(str);
    EXPECT_EQ(result, 999999);
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Strbin_NullString)
{
    int result = ctx.strbin(nullptr);
    EXPECT_EQ(result, 0);
}

TEST_F(SnobolTest, Binstr_PositiveNumber)
{
    node_t *str = ctx.binstr(123);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "123"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Binstr_NegativeNumber)
{
    node_t *str = ctx.binstr(-456);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "-456"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Binstr_Zero)
{
    node_t *str = ctx.binstr(0);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "0"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, Binstr_LargeNumber)
{
    node_t *str = ctx.binstr(999999);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "999999"));
    ctx.delete_string(str);
}

TEST_F(SnobolTest, RoundTripConversion)
{
    int values[] = { 0, 1, -1, 123, -456, 9999, -9999 };
    for (int val : values) {
        node_t *str   = ctx.binstr(val);
        int converted = ctx.strbin(str);
        EXPECT_EQ(converted, val) << "Round-trip failed for value " << val;
        ctx.delete_string(str);
    }
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_F(SnobolTest, Add_PositiveNumbers)
{
    node_t *str1 = ctx.cstr_to_node("10");
    node_t *str2 = ctx.cstr_to_node("20");

    node_t *result = ctx.add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "30"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Add_NegativeNumbers)
{
    node_t *str1 = ctx.cstr_to_node("-10");
    node_t *str2 = ctx.cstr_to_node("-20");

    node_t *result = ctx.add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-30"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Add_MixedSigns)
{
    node_t *str1 = ctx.cstr_to_node("10");
    node_t *str2 = ctx.cstr_to_node("-5");

    node_t *result = ctx.add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "5"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Add_WithZero)
{
    node_t *str1 = ctx.cstr_to_node("42");
    node_t *str2 = ctx.cstr_to_node("0");

    node_t *result = ctx.add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "42"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Sub_PositiveNumbers)
{
    node_t *str1 = ctx.cstr_to_node("20");
    node_t *str2 = ctx.cstr_to_node("10");

    node_t *result = ctx.sub(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "10"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Sub_NegativeResult)
{
    node_t *str1 = ctx.cstr_to_node("10");
    node_t *str2 = ctx.cstr_to_node("20");

    node_t *result = ctx.sub(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-10"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Sub_NegativeNumbers)
{
    node_t *str1 = ctx.cstr_to_node("-10");
    node_t *str2 = ctx.cstr_to_node("-20");

    node_t *result = ctx.sub(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "10"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Mult_PositiveNumbers)
{
    node_t *str1 = ctx.cstr_to_node("6");
    node_t *str2 = ctx.cstr_to_node("7");

    node_t *result = ctx.mult(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "42"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Mult_WithZero)
{
    node_t *str1 = ctx.cstr_to_node("42");
    node_t *str2 = ctx.cstr_to_node("0");

    node_t *result = ctx.mult(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "0"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Mult_NegativeNumbers)
{
    node_t *str1 = ctx.cstr_to_node("-6");
    node_t *str2 = ctx.cstr_to_node("7");

    node_t *result = ctx.mult(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-42"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Divide_PositiveNumbers)
{
    node_t *str1 = ctx.cstr_to_node("20");
    node_t *str2 = ctx.cstr_to_node("4");

    node_t *result = ctx.divide(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "5"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Divide_NegativeNumbers)
{
    node_t *str1 = ctx.cstr_to_node("-20");
    node_t *str2 = ctx.cstr_to_node("4");

    node_t *result = ctx.divide(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-5"));

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Divide_Truncation)
{
    node_t *str1 = ctx.cstr_to_node("7");
    node_t *str2 = ctx.cstr_to_node("3");

    node_t *result = ctx.divide(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "2")); // Integer division

    ctx.delete_string(str1);
    ctx.delete_string(str2);
    ctx.delete_string(result);
}

// ============================================================================
// Symbol Table Operations Tests
// ============================================================================

TEST_F(SnobolTest, Init_CreatesSymbol)
{
    node_t *sym = ctx.init("testvar", 0);
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 0);
}

TEST_F(SnobolTest, Init_SetsType)
{
    node_t *sym = ctx.init("mytype", 5);
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 5);
}

TEST_F(SnobolTest, Look_FindsExistingSymbol)
{
    node_t *sym1 = ctx.init("lookup_test", 3);
    node_t *str  = ctx.cstr_to_node("lookup_test");
    node_t *sym2 = ctx.look(str);

    // Should return the same symbol
    EXPECT_EQ(sym1, sym2);

    ctx.delete_string(str);
}

TEST_F(SnobolTest, Look_CreatesNewSymbol)
{
    node_t *str = ctx.cstr_to_node("new_symbol");
    node_t *sym = ctx.look(str);

    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 0); // Default type

    ctx.delete_string(str);
}

TEST_F(SnobolTest, Look_SameNameReturnsSameSymbol)
{
    node_t *str1 = ctx.cstr_to_node("same_name");
    node_t *str2 = ctx.cstr_to_node("same_name");

    node_t *sym1 = ctx.look(str1);
    node_t *sym2 = ctx.look(str2);

    // Should return the same symbol
    EXPECT_EQ(sym1, sym2);

    ctx.delete_string(str1);
    ctx.delete_string(str2);
}
