#include <gtest/gtest.h>
#include "test_helpers.h"

#include <cstring>
#include <string>

extern "C" {
#include "sno.h"
}

// Test fixture for tests that may need setup/teardown
class SnobolTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Note: Global state is managed by the library itself
        // We don't need to reset it for each test as tests are independent
    }

    void TearDown() override
    {
        // Clean up any strings we allocated in tests
    }
};

// ============================================================================
// Character Classification Tests
// ============================================================================

TEST(CharacterClassificationTest, Operators)
{
    EXPECT_EQ(char_class('+'), 4);  // Plus operator
    EXPECT_EQ(char_class('-'), 5);  // Minus operator
    EXPECT_EQ(char_class('*'), 6);  // Asterisk operator
    EXPECT_EQ(char_class('/'), 7);  // Division operator
    EXPECT_EQ(char_class('='), 10); // Equals sign
}

TEST(CharacterClassificationTest, Delimiters)
{
    EXPECT_EQ(char_class('('), 2);  // Left parenthesis
    EXPECT_EQ(char_class(')'), 1);  // Right parenthesis
    EXPECT_EQ(char_class('\''), 9); // Single quote
    EXPECT_EQ(char_class('"'), 9);  // Double quote
    EXPECT_EQ(char_class(','), 11); // Comma
    EXPECT_EQ(char_class('$'), 8);  // Dollar sign
}

TEST(CharacterClassificationTest, Whitespace)
{
    EXPECT_EQ(char_class(' '), 3);  // Space
    EXPECT_EQ(char_class('\t'), 3); // Tab
}

TEST(CharacterClassificationTest, RegularCharacters)
{
    EXPECT_EQ(char_class('a'), 0); // Letter
    EXPECT_EQ(char_class('0'), 0); // Digit
    EXPECT_EQ(char_class('@'), 0); // Symbol
}

// ============================================================================
// String Operations Tests
// ============================================================================

TEST(StringOperationsTest, CstrToNode_SimpleString)
{
    node_t *str = cstr_to_node("hello");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "hello"));
    delete_string(str);
}

TEST(StringOperationsTest, CstrToNode_SingleChar)
{
    node_t *str = cstr_to_node("a");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "a"));
    delete_string(str);
}

TEST(StringOperationsTest, CstrToNode_NumberString)
{
    node_t *str = cstr_to_node("12345");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "12345"));
    delete_string(str);
}

TEST(StringOperationsTest, Copy_SimpleString)
{
    node_t *orig   = cstr_to_node("test");
    node_t *copied = copy(orig);

    ASSERT_NE(orig, nullptr);
    ASSERT_NE(copied, nullptr);
    EXPECT_NE(orig, copied); // Should be different nodes
    EXPECT_TRUE(node_equals_cstr(orig, "test"));
    EXPECT_TRUE(node_equals_cstr(copied, "test"));

    delete_string(orig);
    delete_string(copied);
}

TEST(StringOperationsTest, Copy_NullString)
{
    node_t *copied = copy(nullptr);
    EXPECT_EQ(copied, nullptr);
}

TEST(StringOperationsTest, Copy_ModifyOriginalDoesNotAffectCopy)
{
    node_t *orig   = cstr_to_node("original");
    node_t *copied = copy(orig);

    // Delete original
    delete_string(orig);

    // Copy should still be valid
    EXPECT_TRUE(node_equals_cstr(copied, "original"));

    delete_string(copied);
}

TEST(StringOperationsTest, Equal_IdenticalStrings)
{
    node_t *str1 = cstr_to_node("hello");
    node_t *str2 = cstr_to_node("hello");

    EXPECT_EQ(equal(str1, str2), 0);

    delete_string(str1);
    delete_string(str2);
}

TEST(StringOperationsTest, Equal_DifferentStrings)
{
    node_t *str1 = cstr_to_node("abc");
    node_t *str2 = cstr_to_node("def");

    EXPECT_NE(equal(str1, str2), 0);

    delete_string(str1);
    delete_string(str2);
}

TEST(StringOperationsTest, Equal_FirstStringGreater)
{
    node_t *str1 = cstr_to_node("def");
    node_t *str2 = cstr_to_node("abc");

    EXPECT_EQ(equal(str1, str2), 1); // str1 > str2

    delete_string(str1);
    delete_string(str2);
}

TEST(StringOperationsTest, Equal_FirstStringLess)
{
    node_t *str1 = cstr_to_node("abc");
    node_t *str2 = cstr_to_node("def");

    EXPECT_EQ(equal(str1, str2), -1); // str1 < str2

    delete_string(str1);
    delete_string(str2);
}

TEST(StringOperationsTest, Equal_DifferentLengths)
{
    node_t *str1 = cstr_to_node("abc");
    node_t *str2 = cstr_to_node("abcd");

    EXPECT_EQ(equal(str1, str2), -1); // str1 < str2 (shorter)

    delete_string(str1);
    delete_string(str2);
}

TEST(StringOperationsTest, Equal_NullStrings)
{
    EXPECT_EQ(equal(nullptr, nullptr), 0);

    node_t *str = cstr_to_node("test");
    EXPECT_EQ(equal(nullptr, str), -1);
    EXPECT_EQ(equal(str, nullptr), 1);

    delete_string(str);
}

TEST(StringOperationsTest, Cat_SimpleConcatenation)
{
    node_t *str1 = cstr_to_node("hello");
    node_t *str2 = cstr_to_node("world");

    node_t *result = cat(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "helloworld"));

    // Original strings should still be valid (non-destructive)
    EXPECT_TRUE(node_equals_cstr(str1, "hello"));
    EXPECT_TRUE(node_equals_cstr(str2, "world"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(StringOperationsTest, Cat_FirstNull)
{
    node_t *str2   = cstr_to_node("world");
    node_t *result = cat(nullptr, str2);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "world"));

    delete_string(str2);
    delete_string(result);
}

TEST(StringOperationsTest, Cat_SecondNull)
{
    node_t *str1   = cstr_to_node("hello");
    node_t *result = cat(str1, nullptr);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "hello"));

    delete_string(str1);
    delete_string(result);
}

TEST(StringOperationsTest, Cat_BothNull)
{
    node_t *result = cat(nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST(StringOperationsTest, Dcat_DestructiveConcatenation)
{
    node_t *str1 = cstr_to_node("foo");
    node_t *str2 = cstr_to_node("bar");

    node_t *result = dcat(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "foobar"));

    // Original strings are deleted, we can only check result
    delete_string(result);
}

// ============================================================================
// Conversion Functions Tests
// ============================================================================

TEST(ConversionTest, Strbin_PositiveNumber)
{
    node_t *str = cstr_to_node("123");
    int result  = strbin(str);
    EXPECT_EQ(result, 123);
    delete_string(str);
}

TEST(ConversionTest, Strbin_NegativeNumber)
{
    node_t *str = cstr_to_node("-456");
    int result  = strbin(str);
    EXPECT_EQ(result, -456);
    delete_string(str);
}

TEST(ConversionTest, Strbin_Zero)
{
    node_t *str = cstr_to_node("0");
    int result  = strbin(str);
    EXPECT_EQ(result, 0);
    delete_string(str);
}

TEST(ConversionTest, Strbin_LargeNumber)
{
    node_t *str = cstr_to_node("999999");
    int result  = strbin(str);
    EXPECT_EQ(result, 999999);
    delete_string(str);
}

TEST(ConversionTest, Strbin_NullString)
{
    int result = strbin(nullptr);
    EXPECT_EQ(result, 0);
}

TEST(ConversionTest, Binstr_PositiveNumber)
{
    node_t *str = binstr(123);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "123"));
    delete_string(str);
}

TEST(ConversionTest, Binstr_NegativeNumber)
{
    node_t *str = binstr(-456);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "-456"));
    delete_string(str);
}

TEST(ConversionTest, Binstr_Zero)
{
    node_t *str = binstr(0);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "0"));
    delete_string(str);
}

TEST(ConversionTest, Binstr_LargeNumber)
{
    node_t *str = binstr(999999);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "999999"));
    delete_string(str);
}

TEST(ConversionTest, RoundTripConversion)
{
    int values[] = { 0, 1, -1, 123, -456, 9999, -9999 };
    for (int val : values) {
        node_t *str   = binstr(val);
        int converted = strbin(str);
        EXPECT_EQ(converted, val) << "Round-trip failed for value " << val;
        delete_string(str);
    }
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST(ArithmeticTest, Add_PositiveNumbers)
{
    node_t *str1 = cstr_to_node("10");
    node_t *str2 = cstr_to_node("20");

    node_t *result = add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "30"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Add_NegativeNumbers)
{
    node_t *str1 = cstr_to_node("-10");
    node_t *str2 = cstr_to_node("-20");

    node_t *result = add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-30"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Add_MixedSigns)
{
    node_t *str1 = cstr_to_node("10");
    node_t *str2 = cstr_to_node("-5");

    node_t *result = add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "5"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Add_WithZero)
{
    node_t *str1 = cstr_to_node("42");
    node_t *str2 = cstr_to_node("0");

    node_t *result = add(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "42"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Sub_PositiveNumbers)
{
    node_t *str1 = cstr_to_node("20");
    node_t *str2 = cstr_to_node("10");

    node_t *result = sub(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "10"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Sub_NegativeResult)
{
    node_t *str1 = cstr_to_node("10");
    node_t *str2 = cstr_to_node("20");

    node_t *result = sub(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-10"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Sub_NegativeNumbers)
{
    node_t *str1 = cstr_to_node("-10");
    node_t *str2 = cstr_to_node("-20");

    node_t *result = sub(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "10"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Mult_PositiveNumbers)
{
    node_t *str1 = cstr_to_node("6");
    node_t *str2 = cstr_to_node("7");

    node_t *result = mult(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "42"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Mult_WithZero)
{
    node_t *str1 = cstr_to_node("42");
    node_t *str2 = cstr_to_node("0");

    node_t *result = mult(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "0"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Mult_NegativeNumbers)
{
    node_t *str1 = cstr_to_node("-6");
    node_t *str2 = cstr_to_node("7");

    node_t *result = mult(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-42"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Divide_PositiveNumbers)
{
    node_t *str1 = cstr_to_node("20");
    node_t *str2 = cstr_to_node("4");

    node_t *result = divide(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "5"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Divide_NegativeNumbers)
{
    node_t *str1 = cstr_to_node("-20");
    node_t *str2 = cstr_to_node("4");

    node_t *result = divide(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-5"));

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

TEST(ArithmeticTest, Divide_Truncation)
{
    node_t *str1 = cstr_to_node("7");
    node_t *str2 = cstr_to_node("3");

    node_t *result = divide(str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "2")); // Integer division

    delete_string(str1);
    delete_string(str2);
    delete_string(result);
}

// ============================================================================
// Symbol Table Operations Tests
// ============================================================================

TEST(SymbolTableTest, Init_CreatesSymbol)
{
    node_t *sym = init("testvar", 0);
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 0);
}

TEST(SymbolTableTest, Init_SetsType)
{
    node_t *sym = init("mytype", 5);
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 5);
}

TEST(SymbolTableTest, Look_FindsExistingSymbol)
{
    node_t *sym1 = init("lookup_test", 3);
    node_t *str  = cstr_to_node("lookup_test");
    node_t *sym2 = look(str);

    // Should return the same symbol
    EXPECT_EQ(sym1, sym2);

    delete_string(str);
}

TEST(SymbolTableTest, Look_CreatesNewSymbol)
{
    node_t *str = cstr_to_node("new_symbol");
    node_t *sym = look(str);

    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 0); // Default type

    delete_string(str);
}

TEST(SymbolTableTest, Look_SameNameReturnsSameSymbol)
{
    node_t *str1 = cstr_to_node("same_name");
    node_t *str2 = cstr_to_node("same_name");

    node_t *sym1 = look(str1);
    node_t *sym2 = look(str2);

    // Should return the same symbol
    EXPECT_EQ(sym1, sym2);

    delete_string(str1);
    delete_string(str2);
}
