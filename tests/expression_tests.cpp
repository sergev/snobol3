#include <gtest/gtest.h>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

class ExpressionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Setup if needed
    }

    void TearDown() override
    {
        // Cleanup if needed
    }
};

// ============================================================================
// Simple Expression Tests
// ============================================================================

TEST_F(ExpressionTest, StringLiteral_SingleQuote)
{
    std::string program = R"(
start   syspot = 'hello'
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello\n");
}

TEST_F(ExpressionTest, StringLiteral_DoubleQuote)
{
    std::string program = R"(
start   syspot = "world"
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "world\n");
}

TEST_F(ExpressionTest, StringLiteral_EmptyString)
{
    std::string program = R"(
start   syspot = ""
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "\n");
}

TEST_F(ExpressionTest, VariableReference)
{
    std::string program = R"(
start   x = "test"
        syspot = x
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test\n");
}

TEST_F(ExpressionTest, StringConcatenation_Whitespace)
{
    std::string program = R"(
start   syspot = "hello" "world"
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "helloworld\n");
}

TEST_F(ExpressionTest, StringConcatenation_Multiple)
{
    std::string program = R"(
start   syspot = "a" "b" "c"
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "abc\n");
}

TEST_F(ExpressionTest, Arithmetic_Addition)
{
    std::string program = R"(
start   x = "10"
        y = "20"
        z = x + y
        syspot = z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n");
}

TEST_F(ExpressionTest, Arithmetic_Subtraction)
{
    std::string program = R"(
start   x = "20"
        y = "10"
        z = x - y
        syspot = z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

TEST_F(ExpressionTest, Arithmetic_Multiplication)
{
    std::string program = R"(
start   x = "6"
        y = "7"
        z = x * y
        syspot = z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "42\n");
}

TEST_F(ExpressionTest, Arithmetic_Division)
{
    std::string program = R"(
start   x = "20"
        y = "4"
        z = x / y
        syspot = z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\n");
}

TEST_F(ExpressionTest, OperatorPrecedence_MultiplicationBeforeAddition)
{
    std::string program = R"(
start   x = "2"
        y = "3"
        z = "4"
        result = x + y * z
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "14\n"); // 2 + (3 * 4) = 14
}

TEST_F(ExpressionTest, OperatorPrecedence_DivisionBeforeSubtraction)
{
    std::string program = R"(
start   x = "20"
        y = "4"
        z = "2"
        result = x - y / z
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "18\n"); // 20 - (4 / 2) = 18
}

TEST_F(ExpressionTest, ParenthesizedExpression)
{
    std::string program = R"(
start   x = "2"
        y = "3"
        z = "4"
        result = (x + y) * z
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "20\n"); // (2 + 3) * 4 = 20
}

TEST_F(ExpressionTest, MixedArithmeticAndString)
{
    std::string program = R"(
start   x = "10"
        y = "5"
        z = x + y
        syspot = "result: " z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "result: 15\n");
}

TEST_F(ExpressionTest, NegativeNumbers)
{
    std::string program = R"(
start   x = "-10"
        y = "5"
        z = x + y
        syspot = z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "-5\n");
}

// ============================================================================
// Complex Expression Tests
// ============================================================================

TEST_F(ExpressionTest, NestedArithmetic)
{
    std::string program = R"(
start   a = "2"
        b = "3"
        c = "4"
        d = "5"
        result = a + b * c - d
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "9\n"); // 2 + (3 * 4) - 5 = 9
}

TEST_F(ExpressionTest, ComplexStringConcatenation)
{
    std::string program = R"(
start   a = "hello"
        b = " "
        c = "world"
        syspot = a b c "!"
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello world!\n");
}

TEST_F(ExpressionTest, PatternImmediate)
{
    std::string program = R"(
start   x = "test"
        test = "hello"
        y = $x
        syspot = y
end     return
)";
    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello\n");
}

TEST_F(ExpressionTest, ExpressionEvaluationOrder)
{
    std::string program = R"(
start   x = "1"
        y = "2"
        z = "3"
        result = x + y * z
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "7\n"); // 1 + (2 * 3) = 7
}

TEST_F(ExpressionTest, LargeNumbers)
{
    std::string program = R"(
start   x = "1000"
        y = "2000"
        z = x + y
        syspot = z
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "3000\n");
}

TEST_F(ExpressionTest, ZeroOperations)
{
    std::string program = R"(
start   x = "0"
        y = "10"
        z1 = x + y
        z2 = x * y
        syspot = z1
        syspot = z2
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n0\n");
}
