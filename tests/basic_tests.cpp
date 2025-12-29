#include <gtest/gtest.h>

#include "sno.h"
#include "test_helpers.h"

// Test fixture for tests that may need setup/teardown
class SnobolTest : public ::testing::Test {
protected:
    std::stringstream input_stream;
    std::stringstream output_stream;
    SnobolContext ctx;

    SnobolTest() : ctx(output_stream) {}

    void SetUp() override
    {
        // Context is already initialized in constructor
    }

    // Helper method to convert Node string to std::string
    std::string node_to_string(Node *str)
    {
        if (str == nullptr) {
            return "";
        }
        std::string result;
        Node *a = str;
        Node *b = str->tail;
        while (a != b) {
            a = a->head;
            result += a->ch;
        }
        return result;
    }

    // Helper method to compare Node string with C string
    bool node_equals_cstr(Node *str, const char *cstr)
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
    EXPECT_EQ(SnobolContext::char_class('+'), CharClass::PLUS);     // Plus operator
    EXPECT_EQ(SnobolContext::char_class('-'), CharClass::MINUS);    // Minus operator
    EXPECT_EQ(SnobolContext::char_class('*'), CharClass::ASTERISK); // Asterisk operator
    EXPECT_EQ(SnobolContext::char_class('/'), CharClass::SLASH);    // Division operator
    EXPECT_EQ(SnobolContext::char_class('='), CharClass::EQUALS);   // Equals sign
}

TEST(CharacterClassificationTest, Delimiters)
{
    EXPECT_EQ(SnobolContext::char_class('('), CharClass::LPAREN);        // Left parenthesis
    EXPECT_EQ(SnobolContext::char_class(')'), CharClass::RPAREN);        // Right parenthesis
    EXPECT_EQ(SnobolContext::char_class('\''), CharClass::STRING_DELIM); // Single quote
    EXPECT_EQ(SnobolContext::char_class('"'), CharClass::STRING_DELIM);  // Double quote
    EXPECT_EQ(SnobolContext::char_class(','), CharClass::COMMA);         // Comma
    EXPECT_EQ(SnobolContext::char_class('$'), CharClass::DOLLAR);        // Dollar sign
}

TEST(CharacterClassificationTest, Whitespace)
{
    EXPECT_EQ(SnobolContext::char_class(' '), CharClass::WHITESPACE);  // Space
    EXPECT_EQ(SnobolContext::char_class('\t'), CharClass::WHITESPACE); // Tab
}

TEST(CharacterClassificationTest, RegularCharacters)
{
    EXPECT_EQ(SnobolContext::char_class('a'), CharClass::OTHER); // Letter
    EXPECT_EQ(SnobolContext::char_class('0'), CharClass::OTHER); // Digit
    EXPECT_EQ(SnobolContext::char_class('@'), CharClass::OTHER); // Symbol
}

// ============================================================================
// String Operations Tests
// ============================================================================

TEST_F(SnobolTest, CstrToNode_SimpleString)
{
    Node &str = ctx.cstr_to_node("hello");
    EXPECT_TRUE(node_equals_cstr(&str, "hello"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, CstrToNode_SingleChar)
{
    Node &str = ctx.cstr_to_node("a");
    EXPECT_TRUE(node_equals_cstr(&str, "a"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, CstrToNode_NumberString)
{
    Node &str = ctx.cstr_to_node("12345");
    EXPECT_TRUE(node_equals_cstr(&str, "12345"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Copy_SimpleString)
{
    Node &orig   = ctx.cstr_to_node("test");
    Node *copied = ctx.copy(&orig);

    ASSERT_NE(copied, nullptr);
    EXPECT_NE(&orig, copied); // Should be different nodes
    EXPECT_TRUE(node_equals_cstr(&orig, "test"));
    EXPECT_TRUE(node_equals_cstr(copied, "test"));

    ctx.delete_string(&orig);
    ctx.delete_string(copied);
}

TEST_F(SnobolTest, Copy_NullString)
{
    Node *copied = ctx.copy(nullptr);
    EXPECT_EQ(copied, nullptr);
}

TEST_F(SnobolTest, Copy_ModifyOriginalDoesNotAffectCopy)
{
    Node &orig   = ctx.cstr_to_node("original");
    Node *copied = ctx.copy(&orig);

    // Delete original
    ctx.delete_string(&orig);

    // Copy should still be valid
    EXPECT_TRUE(node_equals_cstr(copied, "original"));

    ctx.delete_string(copied);
}

TEST_F(SnobolTest, Equal_IdenticalStrings)
{
    Node &str1 = ctx.cstr_to_node("hello");
    Node &str2 = ctx.cstr_to_node("hello");

    EXPECT_EQ(str1.equal(&str2), 0);

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
}

TEST_F(SnobolTest, Equal_DifferentStrings)
{
    Node &str1 = ctx.cstr_to_node("abc");
    Node &str2 = ctx.cstr_to_node("def");

    EXPECT_NE(str1.equal(&str2), 0);

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
}

TEST_F(SnobolTest, Equal_FirstStringGreater)
{
    Node &str1 = ctx.cstr_to_node("def");
    Node &str2 = ctx.cstr_to_node("abc");

    EXPECT_EQ(str1.equal(&str2), 1); // str1 > str2

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
}

TEST_F(SnobolTest, Equal_FirstStringLess)
{
    Node &str1 = ctx.cstr_to_node("abc");
    Node &str2 = ctx.cstr_to_node("def");

    EXPECT_EQ(str1.equal(&str2), -1); // str1 < str2

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
}

TEST_F(SnobolTest, Equal_DifferentLengths)
{
    Node &str1 = ctx.cstr_to_node("abc");
    Node &str2 = ctx.cstr_to_node("abcd");

    EXPECT_EQ(str1.equal(&str2), -1); // str1 < str2 (shorter)

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
}

TEST_F(SnobolTest, Equal_NullStrings)
{
    Node &str = ctx.cstr_to_node("test");
    EXPECT_EQ(str.equal(nullptr), 1);

    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Cat_SimpleConcatenation)
{
    Node &str1 = ctx.cstr_to_node("hello");
    Node &str2 = ctx.cstr_to_node("world");

    Node *result = ctx.cat(&str1, &str2);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "helloworld"));

    // Original strings should still be valid (non-destructive)
    EXPECT_TRUE(node_equals_cstr(&str1, "hello"));
    EXPECT_TRUE(node_equals_cstr(&str2, "world"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Cat_FirstNull)
{
    Node &str2   = ctx.cstr_to_node("world");
    Node *result = ctx.cat(nullptr, &str2);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "world"));

    ctx.delete_string(&str2);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Cat_SecondNull)
{
    Node &str1   = ctx.cstr_to_node("hello");
    Node *result = ctx.cat(&str1, nullptr);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(node_equals_cstr(result, "hello"));

    ctx.delete_string(&str1);
    ctx.delete_string(result);
}

TEST_F(SnobolTest, Cat_BothNull)
{
    Node *result = ctx.cat(nullptr, nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(SnobolTest, Dcat_DestructiveConcatenation)
{
    Node &str1 = ctx.cstr_to_node("foo");
    Node &str2 = ctx.cstr_to_node("bar");

    Node *result = ctx.dcat(str1, str2);
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
    Node &str  = ctx.cstr_to_node("123");
    int result = ctx.strbin(&str);
    EXPECT_EQ(result, 123);
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Strbin_NegativeNumber)
{
    Node &str  = ctx.cstr_to_node("-456");
    int result = ctx.strbin(&str);
    EXPECT_EQ(result, -456);
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Strbin_Zero)
{
    Node &str  = ctx.cstr_to_node("0");
    int result = ctx.strbin(&str);
    EXPECT_EQ(result, 0);
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Strbin_LargeNumber)
{
    Node &str  = ctx.cstr_to_node("999999");
    int result = ctx.strbin(&str);
    EXPECT_EQ(result, 999999);
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Strbin_NullString)
{
    int result = ctx.strbin(nullptr);
    EXPECT_EQ(result, 0);
}

TEST_F(SnobolTest, Binstr_PositiveNumber)
{
    Node &str = ctx.binstr(123);
    EXPECT_TRUE(node_equals_cstr(&str, "123"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Binstr_NegativeNumber)
{
    Node &str = ctx.binstr(-456);
    EXPECT_TRUE(node_equals_cstr(&str, "-456"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Binstr_Zero)
{
    Node &str = ctx.binstr(0);
    EXPECT_TRUE(node_equals_cstr(&str, "0"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Binstr_LargeNumber)
{
    Node &str = ctx.binstr(999999);
    EXPECT_TRUE(node_equals_cstr(&str, "999999"));
    ctx.delete_string(&str);
}

TEST_F(SnobolTest, RoundTripConversion)
{
    int values[] = { 0, 1, -1, 123, -456, 9999, -9999 };
    for (int val : values) {
        Node &str     = ctx.binstr(val);
        int converted = ctx.strbin(&str);
        EXPECT_EQ(converted, val) << "Round-trip failed for value " << val;
        ctx.delete_string(&str);
    }
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_F(SnobolTest, Add_PositiveNumbers)
{
    Node &str1 = ctx.cstr_to_node("10");
    Node &str2 = ctx.cstr_to_node("20");

    Node &result = ctx.add(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "30"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Add_NegativeNumbers)
{
    Node &str1 = ctx.cstr_to_node("-10");
    Node &str2 = ctx.cstr_to_node("-20");

    Node &result = ctx.add(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "-30"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Add_MixedSigns)
{
    Node &str1 = ctx.cstr_to_node("10");
    Node &str2 = ctx.cstr_to_node("-5");

    Node &result = ctx.add(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "5"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Add_WithZero)
{
    Node &str1 = ctx.cstr_to_node("42");
    Node &str2 = ctx.cstr_to_node("0");

    Node &result = ctx.add(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "42"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Sub_PositiveNumbers)
{
    Node &str1 = ctx.cstr_to_node("20");
    Node &str2 = ctx.cstr_to_node("10");

    Node &result = ctx.sub(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "10"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Sub_NegativeResult)
{
    Node &str1 = ctx.cstr_to_node("10");
    Node &str2 = ctx.cstr_to_node("20");

    Node &result = ctx.sub(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "-10"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Sub_NegativeNumbers)
{
    Node &str1 = ctx.cstr_to_node("-10");
    Node &str2 = ctx.cstr_to_node("-20");

    Node &result = ctx.sub(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "10"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Mult_PositiveNumbers)
{
    Node &str1 = ctx.cstr_to_node("6");
    Node &str2 = ctx.cstr_to_node("7");

    Node &result = ctx.mult(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "42"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Mult_WithZero)
{
    Node &str1 = ctx.cstr_to_node("42");
    Node &str2 = ctx.cstr_to_node("0");

    Node &result = ctx.mult(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "0"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Mult_NegativeNumbers)
{
    Node &str1 = ctx.cstr_to_node("-6");
    Node &str2 = ctx.cstr_to_node("7");

    Node &result = ctx.mult(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "-42"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Divide_PositiveNumbers)
{
    Node &str1 = ctx.cstr_to_node("20");
    Node &str2 = ctx.cstr_to_node("4");

    Node &result = ctx.divide(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "5"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Divide_NegativeNumbers)
{
    Node &str1 = ctx.cstr_to_node("-20");
    Node &str2 = ctx.cstr_to_node("4");

    Node &result = ctx.divide(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "-5"));

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

TEST_F(SnobolTest, Divide_Truncation)
{
    Node &str1 = ctx.cstr_to_node("7");
    Node &str2 = ctx.cstr_to_node("3");

    Node &result = ctx.divide(str1, str2);
    EXPECT_TRUE(node_equals_cstr(&result, "2")); // Integer division

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
    ctx.delete_string(&result);
}

// ============================================================================
// Symbol Table Operations Tests
// ============================================================================

TEST_F(SnobolTest, Init_CreatesSymbol)
{
    Node &sym = ctx.init("testvar", Token::EXPR_VAR_REF);
    EXPECT_EQ(sym.typ, Token::EXPR_VAR_REF);
}

TEST_F(SnobolTest, Init_SetsType)
{
    Node &sym = ctx.init("mytype", Token::EXPR_FUNCTION);
    EXPECT_EQ(sym.typ, Token::EXPR_FUNCTION);
}

TEST_F(SnobolTest, Look_FindsExistingSymbol)
{
    Node &sym1 = ctx.init("lookup_test", Token::EXPR_SYSPIT);
    Node &str  = ctx.cstr_to_node("lookup_test");
    Node &sym2 = ctx.look(str);

    // Should return the same symbol
    EXPECT_EQ(&sym1, &sym2);

    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Look_CreatesNewSymbol)
{
    Node &str = ctx.cstr_to_node("new_symbol");
    Node &sym = ctx.look(str);

    EXPECT_EQ(sym.typ, Token::EXPR_VAR_REF); // Default type

    ctx.delete_string(&str);
}

TEST_F(SnobolTest, Look_SameNameReturnsSameSymbol)
{
    Node &str1 = ctx.cstr_to_node("same_name");
    Node &str2 = ctx.cstr_to_node("same_name");

    Node &sym1 = ctx.look(str1);
    Node &sym2 = ctx.look(str2);

    // Should return the same symbol
    EXPECT_EQ(&sym1, &sym2);

    ctx.delete_string(&str1);
    ctx.delete_string(&str2);
}
