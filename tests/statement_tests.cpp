#include <gtest/gtest.h>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

class StatementTest : public ::testing::Test {
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
// Assignment Statement Tests
// ============================================================================

TEST_F(StatementTest, SimpleAssignment)
{
    std::string program = R"(
start
    x = "hello"
    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello\n");
}

TEST_F(StatementTest, AssignmentWithExpression)
{
    std::string program = R"(
start
    a = "10"
    b = "20"
    result = a + b
    syspot = result
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "30\n");
}

TEST_F(StatementTest, AssignmentWithFunctionCall)
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

TEST_F(StatementTest, MultipleAssignments)
{
    std::string program = R"(
start
    a = "1"
    b = "2"
    c = "3"
    syspot = a
    syspot = b
    syspot = c
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n2\n3\n");
}

TEST_F(StatementTest, AssignmentToSyspot)
{
    std::string program = R"(
start
    syspot = "hello"
    syspot = "world"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello\nworld\n");
}

TEST_F(StatementTest, AssignmentChain)
{
    std::string program = R"(
start
    a = "10"
    b = a
    c = b
    syspot = c
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

// ============================================================================
// Pattern Matching Statement Tests
// ============================================================================

TEST_F(StatementTest, PatternMatchStatement)
{
    std::string program = R"(
start
    str = "hello world"
    str "hello", (found, notfound)
found
    syspot = "pattern found"
    goto end
notfound
    syspot = "pattern not found"
end
    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "pattern found\ndone\n");
}

TEST_F(StatementTest, PatternReplacementStatement)
{
    std::string program = R"(
start
    str = "hello world"
    str "world" = "universe"
    syspot = str
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello universe\n");
}

TEST_F(StatementTest, PatternMatchWithVariables)
{
    std::string program = R"(
start
    str = "test string"
    pattern = "test"
    str pattern, (found, notfound)
found
    syspot = "found"
    goto end
notfound
    syspot = "not found"
end
    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(StatementTest, PatternMatchFailureHandling)
{
    std::string program = R"(
start
    str = "hello"
    str "goodbye", (found, notfound)
found
    syspot = "found"
    goto end
notfound
    syspot = "not found"
end
    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "not found\ndone\n");
}

// ============================================================================
// Expression Statement Tests
// ============================================================================

TEST_F(StatementTest, SimpleExpressionStatement)
{
    std::string program = R"(
start
    x = "10"
    y = "20"
    x + y
    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "done\n");
}

TEST_F(StatementTest, ExpressionWithSideEffects)
{
    std::string program = R"(
start
    x = "5"
    y = "10"
    x + y
    syspot = x
    syspot = y
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\n10\n");
}

// ============================================================================
// Label Tests
// ============================================================================

TEST_F(StatementTest, LabeledStatement)
{
    std::string program = R"(
start
    syspot = "start"
    goto middle
middle
    syspot = "middle"
    goto end
end
    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nmiddle\nend\n");
}

TEST_F(StatementTest, StartLabelBehavior)
{
    std::string program = R"(
    syspot = "before start"
start
    syspot = "at start"
end
    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "at start\nend\n");
}

TEST_F(StatementTest, NoStartLabel_ExecutesFirst)
{
    std::string program = R"(
first
    syspot = "first"
    goto second
second
    syspot = "second"
end
    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "first\nsecond\nend\n");
}

TEST_F(StatementTest, EndLabelBehavior)
{
    std::string program = R"(
start
    syspot = "start"
end
    syspot = "end statement"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nend statement\n");
}

TEST_F(StatementTest, MultipleLabels)
{
    std::string program = R"(
start
    syspot = "start"
    goto label1
label1
    syspot = "label1"
    goto label2
label2
    syspot = "label2"
end
    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nlabel1\nlabel2\nend\n");
}

TEST_F(StatementTest, LabelLookup)
{
    std::string program = R"(
start
    goto target
target
    syspot = "target reached"
end
    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "target reached\nend\n");
}
