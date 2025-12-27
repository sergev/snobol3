#include <gtest/gtest.h>

#include "sno.h"
#include "test_helpers.h"

// ============================================================================
// Complete Program Tests
// ============================================================================

TEST(IntegrationTest, HelloWorld)
{
    std::string program = R"(
start   syspot = "Hello, World!"
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "Hello, World!\n");
}

TEST(IntegrationTest, DISABLED_InputOutput_WithSyspit)
{
    std::string program = R"(
start   x = syspit()
        x "end"         /s(done)f(start)
done    syspot = x
end     return
)";

    std::string input       = "test input\n";
    SnobolTestResult result = run_snobol_program(program, input);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test input\n");
}

TEST(IntegrationTest, DISABLED_FactorialCalculation)
{
    std::string program = R"(
define  factorial(n)
        n = 0               /s(base)f(recurse)
base    return "1"
recurse n1 = n - "1"
        factn1 = factorial(n1)
        return n * factn1
start   result = factorial("5")
        syspot = result
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "120\n"); // 5! = 120
}

TEST(IntegrationTest, DISABLED_StringProcessing)
{
    std::string program = R"(
start    str = "hello world"
    str "world" = "universe"
    syspot = str
    str "hello" = "hi"
    syspot = str
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello universe\nhi universe\n");
}

TEST(IntegrationTest, DISABLED_PatternMatchingProgram)
{
    std::string program = R"(
start       str = "test string"
            str "test"                          /s(found)f(notfound)
found       syspot = "pattern found"
            str "string"                        /s(found2)f(notfound2)
found2      syspot = "both patterns found"      /(end)
notfound2   syspot = "second pattern not found" /(end)
notfound    syspot = "pattern not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "pattern found\nboth patterns found\ndone\n");
}

TEST(IntegrationTest, DISABLED_FunctionBasedProgram)
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
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "20\n"); // (2 + 3) * 4 = 20
}

TEST(IntegrationTest, DISABLED_ComplexMultiFeatureProgram)
{
    std::string program = R"(
define process(str)
            str "old" = "new"               /s(changed)f(unchanged)
changed     return str
unchanged   return str
start       input = "old value"
            output = process(input)
            output "new"                    /s(found)f(notfound)
found       syspot = "processed: " output   /(end)
notfound    syspot = "not processed"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "processed: new value\ndone\n");
}

TEST(IntegrationTest, DISABLED_ArithmeticOperations)
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
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n30\n1000\n");
}

TEST(IntegrationTest, StringConcatenation)
{
    std::string program = R"(
start    a = "hello"
    b = " "
    c = "world"
    result = a b c "!"
    syspot = result
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello world!\n");
}

// ============================================================================
// Program Structure Tests
// ============================================================================

TEST(IntegrationTest, ProgramWithStartLabel)
{
    std::string program = R"(
    syspot = "before start"
start    syspot = "at start"
end    syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "at start\nend\n");
}

TEST(IntegrationTest, ProgramWithoutStartLabel)
{
    std::string program = R"(
first   syspot = "first"    /(second)
second  syspot = "second"
end     syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "first\nsecond\nend\n");
}

TEST(IntegrationTest, ProgramWithMultipleLabels)
{
    std::string program = R"(
start       syspot = "start"        /(middle)
middle      syspot = "middle"       /(end_label)
end_label   syspot = "end_label"
end         syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nmiddle\nend_label\nend\n");
}

TEST(IntegrationTest, ProgramExecutionOrder)
{
    std::string program = R"(
start   syspot = "1"    /(two)
two     syspot = "2"    /(three)
three   syspot = "3"
end     syspot = "4"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n2\n3\n4\n");
}

TEST(IntegrationTest, EndOfProgramHandling)
{
    std::string program = R"(
start   syspot = "start"
end     syspot = "end statement"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nend statement\n");
}

TEST(IntegrationTest, DISABLED_DefineStatementsNotExecutable)
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
end return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n2\n");
}

TEST(IntegrationTest, DISABLED_CompleteExampleFromGrammar)
{
    std::string program = R"(
define  add(a, b)
        return a + b
start   x = syspit()
        x "end"             /s(done)f(start)
done    y = add(x, "!")
        syspot = y
end     return
)";

    std::string input       = "test\n";
    SnobolTestResult result = run_snobol_program(program, input);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "test!\n");
}

TEST(IntegrationTest, LoopWithCondition)
{
    std::string program = R"(
start   count = "0"
loop    count = count + "1"
        count = "5"             /s(done)f(loop)
done    syspot = count
end     syspot = "finished"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\nfinished\n");
}

TEST(IntegrationTest, DISABLED_MultipleFunctionDefinitions)
{
    std::string program = R"(
define  double(x)
        return x + x
define  square(x)
        return x * x
define  add(x, y)
        return x + y
start   a = double("5")
        b = square("4")
        c = add(a, b)
        syspot = c
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "26\n"); // (5*2) + (4*4) = 10 + 16 = 26
}
