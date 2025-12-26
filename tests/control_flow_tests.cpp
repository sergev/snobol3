#include <gtest/gtest.h>

#include "test_helpers.h"

extern "C" {
#include "sno.h"
}

class ControlFlowTest : public ::testing::Test {
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
// Simple Goto Tests
// ============================================================================

TEST_F(ControlFlowTest, SimpleGoto)
{
    std::string program = R"(
start    syspot = "start"
    goto target
target    syspot = "target"
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\ntarget\nend\n");
}

TEST_F(ControlFlowTest, GotoAfterAssignment)
{
    std::string program = R"(
start    x = "10", next
next    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

TEST_F(ControlFlowTest, GotoAfterExpression)
{
    std::string program = R"(
start    x = "5"
    y = "10"
    x + y, next
next    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "done\n");
}

TEST_F(ControlFlowTest, MultipleGotos)
{
    std::string program = R"(
start    syspot = "1"
    goto two
two    syspot = "2"
    goto three
three    syspot = "3"
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "1\n2\n3\nend\n");
}

// ============================================================================
// Success/Failure Goto Tests
// ============================================================================

TEST_F(ControlFlowTest, SuccessFailureGoto_BothTargets)
{
    std::string program = R"(
start    str = "hello"
    str "hello", (success, failure)
success    syspot = "success"
    goto end
failure    syspot = "failure"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "success\ndone\n");
}

TEST_F(ControlFlowTest, SuccessFailureGoto_FailurePath)
{
    std::string program = R"(
start    str = "hello"
    str "goodbye", (success, failure)
success    syspot = "success"
    goto end
failure    syspot = "failure"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "failure\ndone\n");
}

TEST_F(ControlFlowTest, SuccessOnlyGoto)
{
    std::string program = R"(
start    str = "hello"
    str "hello", s(success)
    syspot = "continued"
    goto end
success    syspot = "success"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "success\ndone\n");
}

TEST_F(ControlFlowTest, SuccessOnlyGoto_NoMatch)
{
    std::string program = R"(
start    str = "hello"
    str "goodbye", s(success)
    syspot = "continued"
    goto end
success    syspot = "success"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "continued\ndone\n");
}

TEST_F(ControlFlowTest, FailureOnlyGoto)
{
    std::string program = R"(
start    str = "hello"
    str "goodbye", f(failure)
    syspot = "continued"
    goto end
failure    syspot = "failure"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "failure\ndone\n");
}

TEST_F(ControlFlowTest, FailureOnlyGoto_Match)
{
    std::string program = R"(
start    str = "hello"
    str "hello", f(failure)
    syspot = "continued"
    goto end
failure    syspot = "failure"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "continued\ndone\n");
}

TEST_F(ControlFlowTest, PatternMatchWithGoto)
{
    std::string program = R"(
start    str = "test"
    str "test", (found, notfound)
found    syspot = "found"
    goto end
notfound    syspot = "not found"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(ControlFlowTest, AssignmentWithGoto)
{
    std::string program = R"(
start    x = "10", next
next    syspot = x
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

// ============================================================================
// Control Flow Pattern Tests
// ============================================================================

TEST_F(ControlFlowTest, LoopWithGoto)
{
    std::string program = R"(
start    count = "0"
loop    count = count + "1"
    count = "5", (done, loop)
done    syspot = count
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\nend\n");
}

TEST_F(ControlFlowTest, ConditionalExecution)
{
    std::string program = R"(
start    x = "10"
    x = "0", (zero, nonzero)
zero    syspot = "zero"
    goto end
nonzero    syspot = "nonzero"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "nonzero\ndone\n");
}

TEST_F(ControlFlowTest, ConditionalExecution_ZeroCase)
{
    std::string program = R"(
start    x = "0"
    x = "0", (zero, nonzero)
zero    syspot = "zero"
    goto end
nonzero    syspot = "nonzero"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "zero\ndone\n");
}

TEST_F(ControlFlowTest, EarlyExitPattern)
{
    std::string program = R"(
start    syspot = "start"
    goto check
check    x = "0"
    x = "0", (exit, continue)
exit    syspot = "exit"
    goto end
continue    syspot = "continue"
end    syspot = "end"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nexit\nend\n");
}

TEST_F(ControlFlowTest, SuccessFailureChaining)
{
    std::string program = R"(
start    str = "test"
    str "test", (first, fail)
first    str "t", (second, fail)
second    syspot = "both matched"
    goto end
fail    syspot = "failed"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "both matched\ndone\n");
}

TEST_F(ControlFlowTest, NestedConditionals)
{
    std::string program = R"(
start    x = "5"
    x = "0", (zero, checkpos)
zero    syspot = "zero"
    goto end
checkpos    x = "0", (neg, pos)
neg    syspot = "negative"
    goto end
pos    syspot = "positive"
end    syspot = "done"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "positive\ndone\n");
}

TEST_F(ControlFlowTest, ComplexControlFlow)
{
    std::string program = R"(
start    count = "0"
loop    count = count + "1"
    count = "3", (done, loop)
done    syspot = count
    count = "5", (end, more)
more    syspot = "more"
    goto end
end    syspot = "finished"
end
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "3\nfinished\n");
}
