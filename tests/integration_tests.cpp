#include <gtest/gtest.h>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

class IntegrationTest : public ::testing::Test {
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
// Complete Program Tests
// ============================================================================

TEST_F(IntegrationTest, HelloWorld)
{
    std::string program = R"(
start    syspot = "Hello, World!"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "Hello, World!\n");
}

TEST_F(IntegrationTest, InputOutput_WithSyspit)
{
    std::string program = R"(
start    x = syspit()
    x "end", (done, start)
done    syspot = x
end
)";

    std::string input       = "test input\n";
    SnobolTestResult result = run_snobol_program(program, input);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test input\n");
}

TEST_F(IntegrationTest, FactorialCalculation)
{
    std::string program = R"(
define factorial(n)
    n = 0, (base, recurse)
base    return "1"
recurse    n1 = n - "1"
    factn1 = factorial(n1)
    return n * factn1
start    result = factorial("5")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "120\n"); // 5! = 120
}

TEST_F(IntegrationTest, StringProcessing)
{
    std::string program = R"(
start    str = "hello world"
    str "world" = "universe"
    syspot = str
    str "hello" = "hi"
    syspot = str
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello universe\nhi universe\n");
}

TEST_F(IntegrationTest, PatternMatchingProgram)
{
    std::string program = R"(
start    str = "test string"
    str "test", (found, notfound)
found    syspot = "pattern found"
    str "string", (found2, notfound2)
found2    syspot = "both patterns found"
    goto end
notfound2    syspot = "second pattern not found"
    goto end
notfound    syspot = "pattern not found"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "pattern found\nboth patterns found\ndone\n");
}

TEST_F(IntegrationTest, FunctionBasedProgram)
{
    std::string program = R"(
define add(x, y)
    return x + y
define multiply(x, y)
    return x * y
define calculate(x, y, z)
    sum = add(x, y)
    return multiply(sum, z)
start    result = calculate("2", "3", "4")
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "20\n"); // (2 + 3) * 4 = 20
}

TEST_F(IntegrationTest, ComplexMultiFeatureProgram)
{
    std::string program = R"(
define process(str)
    str "old" = "new", (changed, unchanged)
changed    return str
unchanged    return str
start    input = "old value"
    output = process(input)
    output "new", (found, notfound)
found    syspot = "processed: " output
    goto end
notfound    syspot = "not processed"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "processed: new value\ndone\n");
}

TEST_F(IntegrationTest, ArithmeticOperations)
{
    std::string program = R"(
start    a = "10"
    b = "20"
    c = "3"
    sum = a + b
    product = a * c
    power = a ** c
    syspot = sum
    syspot = product
    syspot = power
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n30\n1000\n");
}

TEST_F(IntegrationTest, StringConcatenation)
{
    std::string program = R"(
start    a = "hello"
    b = " "
    c = "world"
    result = a b c "!"
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello world!\n");
}

// ============================================================================
// Program Structure Tests
// ============================================================================

TEST_F(IntegrationTest, ProgramWithStartLabel)
{
    std::string program = R"(
    syspot = "before start"
start    syspot = "at start"
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "at start\nend\n");
}

TEST_F(IntegrationTest, ProgramWithoutStartLabel)
{
    std::string program = R"(
first    syspot = "first"
    goto second
second    syspot = "second"
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "first\nsecond\nend\n");
}

TEST_F(IntegrationTest, ProgramWithMultipleLabels)
{
    std::string program = R"(
start    syspot = "start"
    goto middle
middle    syspot = "middle"
    goto end_label
end_label    syspot = "end_label"
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nmiddle\nend_label\nend\n");
}

TEST_F(IntegrationTest, ProgramExecutionOrder)
{
    std::string program = R"(
start    syspot = "1"
    goto two
two    syspot = "2"
    goto three
three    syspot = "3"
end    syspot = "4"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n2\n3\n4\n");
}

TEST_F(IntegrationTest, EndOfProgramHandling)
{
    std::string program = R"(
start    syspot = "start"
end    syspot = "end statement"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nend statement\n");
}

TEST_F(IntegrationTest, DefineStatementsNotExecutable)
{
    std::string program = R"(
define func1()
    return "1"
define func2()
    return "2"
start    result = func1()
    syspot = result
    result = func2()
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n2\n");
}

TEST_F(IntegrationTest, CompleteExampleFromGrammar)
{
    std::string program = R"(
define add(a, b)
    return a + b
start    x = syspit()
    x "end", (done, start)
done    y = add(x, "!")
    syspot = y
end
)";

    std::string input       = "test\n";
    SnobolTestResult result = run_snobol_program(program, input);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test!\n");
}

TEST_F(IntegrationTest, LoopWithCondition)
{
    std::string program = R"(
start    count = "0"
loop    count = count + "1"
    count = "5", (done, loop)
done    syspot = count
end    syspot = "finished"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\nfinished\n");
}

TEST_F(IntegrationTest, MultipleFunctionDefinitions)
{
    std::string program = R"(
define double(x)
    return x + x
define square(x)
    return x * x
define add(x, y)
    return x + y
start    a = double("5")
    b = square("4")
    c = add(a, b)
    syspot = c
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "26\n"); // (5*2) + (4*4) = 10 + 16 = 26
}
