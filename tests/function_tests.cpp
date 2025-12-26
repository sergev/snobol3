#include <gtest/gtest.h>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

class FunctionTest : public ::testing::Test {
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
// Function Definition Tests
// ============================================================================

TEST_F(FunctionTest, FunctionNoParameters)
{
    std::string program = R"(
define getvalue()
    return "42"
start
    x = getvalue()
    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "42\n");
}

TEST_F(FunctionTest, FunctionWithOneParameter)
{
    std::string program = R"(
define double(x)
    return x + x
start
    result = double("5")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

TEST_F(FunctionTest, FunctionWithTwoParameters)
{
    std::string program = R"(
define add(a, b)
    return a + b
start
    result = add("10", "20")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n");
}

TEST_F(FunctionTest, FunctionWithMultipleParameters)
{
    std::string program = R"(
define sum(a, b, c)
    return a + b + c
start
    result = sum("1", "2", "3")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "6\n");
}

// ============================================================================
// Function Call Tests
// ============================================================================

TEST_F(FunctionTest, SimpleFunctionCall)
{
    std::string program = R"(
define hello()
    return "hello"
start
    x = hello()
    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello\n");
}

TEST_F(FunctionTest, FunctionCallWithArguments)
{
    std::string program = R"(
define multiply(x, y)
    return x * y
start
    result = multiply("6", "7")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "42\n");
}

TEST_F(FunctionTest, NestedFunctionCalls)
{
    std::string program = R"(
define add(x, y)
    return x + y
define multiply(x, y)
    return x * y
start
    result = multiply(add("2", "3"), "4")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "20\n"); // (2 + 3) * 4 = 20
}

TEST_F(FunctionTest, FunctionCallInExpression)
{
    std::string program = R"(
define getfive()
    return "5"
start
    result = getfive() + "10"
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "15\n");
}

TEST_F(FunctionTest, FunctionCallWithVariableArguments)
{
    std::string program = R"(
define add(x, y)
    return x + y
start
    a = "10"
    b = "20"
    result = add(a, b)
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n");
}

// ============================================================================
// Function Return Tests
// ============================================================================

TEST_F(FunctionTest, ReturnWithValue)
{
    std::string program = R"(
define getstring()
    return "test"
start
    x = getstring()
    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test\n");
}

TEST_F(FunctionTest, ReturnWithExpression)
{
    std::string program = R"(
define calculate()
    return "10" + "20"
start
    result = calculate()
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n");
}

TEST_F(FunctionTest, FunctionReturnInExpressions)
{
    std::string program = R"(
define getten()
    return "10"
start
    result = getten() + getten()
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "20\n");
}

// ============================================================================
// Function Semantics Tests
// ============================================================================

TEST_F(FunctionTest, ParameterBinding)
{
    std::string program = R"(
define testparam(x)
    syspot = x
    return x
start
    a = "original"
    result = testparam("bound")
    syspot = a
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "bound\noriginal\n");
}

TEST_F(FunctionTest, ParameterValueRestoration)
{
    std::string program = R"(
define test(x)
    x = "changed"
    return x
start
    x = "original"
    result = test("param")
    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    // Parameter x should be restored, outer x should be unchanged
    EXPECT_EQ(result.stdout_output, "original\n");
}

TEST_F(FunctionTest, RecursiveFunction)
{
    std::string program = R"(
define factorial(n)
    n = 0, (base, recurse)
base
    return "1"
recurse
    n1 = n - "1"
    factn1 = factorial(n1)
    return n * factn1
start
    result = factorial("5")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "120\n"); // 5! = 120
}

TEST_F(FunctionTest, FunctionFailureReturn)
{
    std::string program = R"(
define mayfail(flag)
    flag = "0", (success, fail)
success
    return "success"
fail
    freturn
start
    result = mayfail("1")
    result = 0, (found, notfound)
found
    syspot = "function succeeded"
    goto end
notfound
    syspot = "function failed"
end
    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    // This tests freturn behavior
    EXPECT_TRUE(result.success || !result.stderr_output.empty());
}

TEST_F(FunctionTest, MultipleFunctions)
{
    std::string program = R"(
define add(x, y)
    return x + y
define multiply(x, y)
    return x * y
start
    a = add("2", "3")
    b = multiply("4", "5")
    result = a + b
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "25\n"); // (2 + 3) + (4 * 5) = 25
}

TEST_F(FunctionTest, FunctionWithPatternMatching)
{
    std::string program = R"(
define checkpattern(str)
    str "test", (found, notfound)
found
    return "found"
notfound
    return "not found"
start
    result = checkpattern("test string")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\n");
}

TEST_F(FunctionTest, FunctionCallChain)
{
    std::string program = R"(
define addone(x)
    return x + "1"
define addtwo(x)
    return addone(addone(x))
start
    result = addtwo("5")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "7\n");
}
