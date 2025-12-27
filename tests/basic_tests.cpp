#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

// Test fixture for tests that may need setup/teardown
class SnobolTest : public ::testing::Test {
protected:
    snobol_context_t *ctx;

    void SetUp() override
    {
        ctx = snobol_context_create();
        ASSERT_NE(ctx, nullptr) << "Failed to create interpreter context";
    }

    void TearDown() override
    {
        // Note: Context cleanup would go here if we had a destroy function
        // For now, we rely on the OS to clean up memory on process exit
        // TODO: Add snobol_context_destroy() function if needed
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
    node_t *str = cstr_to_node(ctx, "hello");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "hello"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, CstrToNode_SingleChar)
{
    node_t *str = cstr_to_node(ctx, "a");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "a"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, CstrToNode_NumberString)
{
    node_t *str = cstr_to_node(ctx, "12345");
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "12345"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Copy_SimpleString)
{
    node_t *orig   = cstr_to_node(ctx, "test");
    node_t *copied = copy(ctx, orig);

    ASSERT_NE(orig, nullptr);
    ASSERT_NE(copied, nullptr);
    EXPECT_NE(orig, copied); // Should be different nodes
    EXPECT_TRUE(node_equals_cstr(orig, "test"));
    EXPECT_TRUE(node_equals_cstr(copied, "test"));

    delete_string(ctx, orig);
    delete_string(ctx, copied);
}

TEST_F(SnobolTest, Copy_NullString)
{
    node_t *copied = copy(ctx, nullptr);
    EXPECT_EQ(copied, nullptr);
}

TEST_F(SnobolTest, Copy_ModifyOriginalDoesNotAffectCopy)
{
    node_t *orig   = cstr_to_node(ctx, "original");
    node_t *copied = copy(ctx, orig);

    // Delete original
    delete_string(ctx, orig);

    // Copy should still be valid
    EXPECT_TRUE(node_equals_cstr(copied, "original"));

    delete_string(ctx, copied);
}

TEST_F(SnobolTest, Equal_IdenticalStrings)
{
    node_t *str1 = cstr_to_node(ctx, "hello");
    node_t *str2 = cstr_to_node(ctx, "hello");

    EXPECT_EQ(equal(str1, str2), 0);

    delete_string(ctx, str1);
    delete_string(ctx, str2);
}

TEST_F(SnobolTest, Equal_DifferentStrings)
{
    node_t *str1 = cstr_to_node(ctx, "abc");
    node_t *str2 = cstr_to_node(ctx, "def");

    EXPECT_NE(equal(str1, str2), 0);

    delete_string(ctx, str1);
    delete_string(ctx, str2);
}

TEST_F(SnobolTest, Equal_FirstStringGreater)
{
    node_t *str1 = cstr_to_node(ctx, "def");
    node_t *str2 = cstr_to_node(ctx, "abc");

    EXPECT_EQ(equal(str1, str2), 1); // str1 > str2

    delete_string(ctx, str1);
    delete_string(ctx, str2);
}

TEST_F(SnobolTest, Equal_FirstStringLess)
{
    node_t *str1 = cstr_to_node(ctx, "abc");
    node_t *str2 = cstr_to_node(ctx, "def");

    EXPECT_EQ(equal(str1, str2), -1); // str1 < str2

    delete_string(ctx, str1);
    delete_string(ctx, str2);
}

TEST_F(SnobolTest, Equal_DifferentLengths)
{
    node_t *str1 = cstr_to_node(ctx, "abc");
    node_t *str2 = cstr_to_node(ctx, "abcd");

    EXPECT_EQ(equal(str1, str2), -1); // str1 < str2 (shorter)

    delete_string(ctx, str1);
    delete_string(ctx, str2);
}

TEST_F(SnobolTest, Equal_NullStrings)
{
    EXPECT_EQ(equal(nullptr, nullptr), 0);

    node_t *str = cstr_to_node(ctx, "test");
    EXPECT_EQ(equal(nullptr, str), -1);
    EXPECT_EQ(equal(str, nullptr), 1);

    delete_string(ctx, str);
}

TEST_F(SnobolTest, Cat_SimpleConcatenation)
{
    node_t *str1 = cstr_to_node(ctx, "hello");
    node_t *str2 = cstr_to_node(ctx, "world");

    node_t *result = cat(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "helloworld"));

    // Original strings should still be valid (non-destructive)
    EXPECT_TRUE(node_equals_cstr(str1, "hello"));
    EXPECT_TRUE(node_equals_cstr(str2, "world"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Cat_FirstNull)
{
    node_t *str2   = cstr_to_node(ctx, "world");
    node_t *result = cat(ctx, nullptr, str2);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "world"));

    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Cat_SecondNull)
{
    node_t *str1   = cstr_to_node(ctx, "hello");
    node_t *result = cat(ctx, str1, nullptr);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "hello"));

    delete_string(ctx, str1);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Cat_BothNull)
{
    node_t *result = cat(ctx, nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SnobolTest, Dcat_DestructiveConcatenation)
{
    node_t *str1 = cstr_to_node(ctx, "foo");
    node_t *str2 = cstr_to_node(ctx, "bar");

    node_t *result = dcat(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "foobar"));

    // Original strings are deleted, we can only check result
    delete_string(ctx, result);
}

// ============================================================================
// Conversion Functions Tests
// ============================================================================

TEST_F(SnobolTest, Strbin_PositiveNumber)
{
    node_t *str = cstr_to_node(ctx, "123");
    int result  = strbin(ctx, str);
    EXPECT_EQ(result, 123);
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Strbin_NegativeNumber)
{
    node_t *str = cstr_to_node(ctx, "-456");
    int result  = strbin(ctx, str);
    EXPECT_EQ(result, -456);
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Strbin_Zero)
{
    node_t *str = cstr_to_node(ctx, "0");
    int result  = strbin(ctx, str);
    EXPECT_EQ(result, 0);
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Strbin_LargeNumber)
{
    node_t *str = cstr_to_node(ctx, "999999");
    int result  = strbin(ctx, str);
    EXPECT_EQ(result, 999999);
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Strbin_NullString)
{
    int result = strbin(ctx, nullptr);
    EXPECT_EQ(result, 0);
}

TEST_F(SnobolTest, Binstr_PositiveNumber)
{
    node_t *str = binstr(ctx, 123);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "123"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Binstr_NegativeNumber)
{
    node_t *str = binstr(ctx, -456);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "-456"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Binstr_Zero)
{
    node_t *str = binstr(ctx, 0);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "0"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, Binstr_LargeNumber)
{
    node_t *str = binstr(ctx, 999999);
    ASSERT_NE(str, nullptr);
    EXPECT_TRUE(node_equals_cstr(str, "999999"));
    delete_string(ctx, str);
}

TEST_F(SnobolTest, RoundTripConversion)
{
    int values[] = { 0, 1, -1, 123, -456, 9999, -9999 };
    for (int val : values) {
        node_t *str   = binstr(ctx, val);
        int converted = strbin(ctx, str);
        EXPECT_EQ(converted, val) << "Round-trip failed for value " << val;
        delete_string(ctx, str);
    }
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_F(SnobolTest, Add_PositiveNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "10");
    node_t *str2 = cstr_to_node(ctx, "20");

    node_t *result = add(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "30"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Add_NegativeNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "-10");
    node_t *str2 = cstr_to_node(ctx, "-20");

    node_t *result = add(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-30"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Add_MixedSigns)
{
    node_t *str1 = cstr_to_node(ctx, "10");
    node_t *str2 = cstr_to_node(ctx, "-5");

    node_t *result = add(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "5"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Add_WithZero)
{
    node_t *str1 = cstr_to_node(ctx, "42");
    node_t *str2 = cstr_to_node(ctx, "0");

    node_t *result = add(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "42"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Sub_PositiveNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "20");
    node_t *str2 = cstr_to_node(ctx, "10");

    node_t *result = sub(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "10"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Sub_NegativeResult)
{
    node_t *str1 = cstr_to_node(ctx, "10");
    node_t *str2 = cstr_to_node(ctx, "20");

    node_t *result = sub(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-10"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Sub_NegativeNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "-10");
    node_t *str2 = cstr_to_node(ctx, "-20");

    node_t *result = sub(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "10"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Mult_PositiveNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "6");
    node_t *str2 = cstr_to_node(ctx, "7");

    node_t *result = mult(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "42"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Mult_WithZero)
{
    node_t *str1 = cstr_to_node(ctx, "42");
    node_t *str2 = cstr_to_node(ctx, "0");

    node_t *result = mult(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "0"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Mult_NegativeNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "-6");
    node_t *str2 = cstr_to_node(ctx, "7");

    node_t *result = mult(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-42"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Divide_PositiveNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "20");
    node_t *str2 = cstr_to_node(ctx, "4");

    node_t *result = divide(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "5"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Divide_NegativeNumbers)
{
    node_t *str1 = cstr_to_node(ctx, "-20");
    node_t *str2 = cstr_to_node(ctx, "4");

    node_t *result = divide(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "-5"));

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

TEST_F(SnobolTest, Divide_Truncation)
{
    node_t *str1 = cstr_to_node(ctx, "7");
    node_t *str2 = cstr_to_node(ctx, "3");

    node_t *result = divide(ctx, str1, str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "2")); // Integer division

    delete_string(ctx, str1);
    delete_string(ctx, str2);
    delete_string(ctx, result);
}

// ============================================================================
// Symbol Table Operations Tests
// ============================================================================

TEST_F(SnobolTest, Init_CreatesSymbol)
{
    node_t *sym = init(ctx, "testvar", 0);
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 0);
}

TEST_F(SnobolTest, Init_SetsType)
{
    node_t *sym = init(ctx, "mytype", 5);
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 5);
}

TEST_F(SnobolTest, Look_FindsExistingSymbol)
{
    node_t *sym1 = init(ctx, "lookup_test", 3);
    node_t *str  = cstr_to_node(ctx, "lookup_test");
    node_t *sym2 = look(ctx, str);

    // Should return the same symbol
    EXPECT_EQ(sym1, sym2);

    delete_string(ctx, str);
}

TEST_F(SnobolTest, Look_CreatesNewSymbol)
{
    node_t *str = cstr_to_node(ctx, "new_symbol");
    node_t *sym = look(ctx, str);

    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->typ, 0); // Default type

    delete_string(ctx, str);
}

TEST_F(SnobolTest, Look_SameNameReturnsSameSymbol)
{
    node_t *str1 = cstr_to_node(ctx, "same_name");
    node_t *str2 = cstr_to_node(ctx, "same_name");

    node_t *sym1 = look(ctx, str1);
    node_t *sym2 = look(ctx, str2);

    // Should return the same symbol
    EXPECT_EQ(sym1, sym2);

    delete_string(ctx, str1);
    delete_string(ctx, str2);
}
