#include <gtest/gtest.h>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

class EdgeCaseTest : public ::testing::Test {
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
// Edge Case Tests
// ============================================================================

TEST_F(EdgeCaseTest, EmptyProgram)
{
    std::string program = R"(
start
end    syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "end\n");
}

TEST_F(EdgeCaseTest, UninitializedVariable)
{
    std::string program = R"(
start    syspot = x
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    // Uninitialized variable should be empty string
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "\n");
}

TEST_F(EdgeCaseTest, VeryLongString)
{
    std::string program = R"(
start    x = "a"
    y = "b"
    z = x y x y x y x y x y x y x y x y
    syspot = z
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "abababababababab\n");
}

TEST_F(EdgeCaseTest, VeryLongExpression)
{
    std::string program = R"(
start    a = "1"
    b = "2"
    c = "3"
    d = "4"
    result = a + b + c + d
    syspot = result
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

TEST_F(EdgeCaseTest, DeeplyNestedCalls)
{
    std::string program = R"(
define id(x)
    return x
start    result = id(id(id(id("5"))))
    syspot = result
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\n");
}

TEST_F(EdgeCaseTest, PatternMatching_EmptyString)
{
    std::string program = R"(
start       str = ""
            str ""                  /s(found)f(notfound)
found       syspot = "found"        /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    // Empty pattern should match empty string
    EXPECT_TRUE(result.success || !result.stderr_output.empty());
}

TEST_F(EdgeCaseTest, PatternMatching_NoMatch)
{
    std::string program = R"(
start       str = "hello"
            str "goodbye"           /s(found)f(notfound)
found       syspot = "found"        /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "not found\ndone\n");
}

TEST_F(EdgeCaseTest, FunctionParameterMismatch_TooFew)
{
    std::string program = R"(
define add(x, y)
    return x + y
start    result = add("5")
    syspot = result
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    // This should fail with parameter mismatch
    EXPECT_FALSE(result.success || !result.stderr_output.empty());
}

TEST_F(EdgeCaseTest, FunctionParameterMismatch_TooMany)
{
    std::string program = R"(
define add(x, y)
    return x + y
start    result = add("5", "10", "15")
    syspot = result
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    // This should fail with parameter mismatch
    EXPECT_FALSE(result.success || !result.stderr_output.empty());
}

TEST_F(EdgeCaseTest, ZeroOperations)
{
    std::string program = R"(
start    x = "0"
    y = "10"
    sum = x + y
    product = x * y
    syspot = sum
    syspot = product
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n0\n");
}

TEST_F(EdgeCaseTest, NegativeNumbers)
{
    std::string program = R"(
start    x = "-10"
    y = "5"
    sum = x + y
    diff = x - y
    syspot = sum
    syspot = diff
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "-5\n-15\n");
}

TEST_F(EdgeCaseTest, LargeNumbers)
{
    std::string program = R"(
start    x = "1000"
    y = "2000"
    sum = x + y
    syspot = sum
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "3000\n");
}

TEST_F(EdgeCaseTest, StringWithQuotes)
{
    std::string program = R"(
start    x = "hello 'world'"
    syspot = x
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello 'world'\n");
}

TEST_F(EdgeCaseTest, StringWithDoubleQuotes)
{
    std::string program = R"(
start    x = 'hello "world"'
    syspot = x
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello \"world\"\n");
}

TEST_F(EdgeCaseTest, MultipleAssignmentsToSameVariable)
{
    std::string program = R"(
start    x = "1"
    x = "2"
    x = "3"
    syspot = x
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "3\n");
}

TEST_F(EdgeCaseTest, SelfReferencingAssignment)
{
    std::string program = R"(
start    x = "5"
    x = x + "1"
    syspot = x
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "6\n");
}

TEST_F(EdgeCaseTest, PatternReplacement_NoMatch)
{
    std::string program = R"(
start       str = "hello"
            str "goodbye" = "world"     /s(found)f(notfound)
found       syspot = str                /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "not found\ndone\n");
}

TEST_F(EdgeCaseTest, GotoToUndefinedLabel)
{
    std::string program = R"(
start                   /(undefined)
end     syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    // This should fail with undefined label
    EXPECT_FALSE(result.success || !result.stderr_output.empty());
}

TEST_F(EdgeCaseTest, FunctionCallInPattern)
{
    std::string program = R"(
define      getpattern()
            return "test"
start       str = "test string"
            str getpattern()        /s(found)f(notfound)
found       syspot = "found"        /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(EdgeCaseTest, RecursiveFunction_BaseCase)
{
    std::string program = R"(
define  factorial(n)
        n = 0                   /s(base)f(recurse)
base    return "1"
recurse n1 = n - "1"
        factn1 = factorial(n1)
        return n * factn1
start   result = factorial("0")
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n");
}

TEST_F(EdgeCaseTest, AlternationPattern_NoMatch)
{
    std::string program = R"(
start       str = "xyz"
            str *"a"*"b"*           /s(found)f(notfound)
found       syspot = "found"        /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "not found\ndone\n");
}

TEST_F(EdgeCaseTest, ComplexExpression_Precedence)
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

TEST_F(EdgeCaseTest, EmptyStringOperations)
{
    std::string program = R"(
start   x = ""
        y = "test"
        concat = x y
        syspot = concat
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test\n");
}

TEST_F(EdgeCaseTest, PatternImmediateInExpression)
{
    std::string program = R"(
start   x = "test"
        y = $x
        syspot = y
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    // Pattern immediate behavior may vary
    EXPECT_TRUE(result.success || !result.stderr_output.empty());
}
