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
start   syspot = "start"    /(target)
target  syspot = "target"
end     syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\ntarget\nend\n");
}

TEST_F(ControlFlowTest, DISABLED_GotoAfterAssignment)
{
    std::string program = R"(
start   x = "10", next
next    syspot = x
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "10\n");
}

TEST_F(ControlFlowTest, DISABLED_GotoAfterExpression)
{
    std::string program = R"(
start   x = "5"
        y = "10"
        x + y, next
next    syspot = "done"
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "done\n");
}

TEST_F(ControlFlowTest, MultipleGotos)
{
    std::string program = R"(
start   syspot = "1"        /(two)
two     syspot = "2"        /(three)
three   syspot = "3"
end     syspot = "end"
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
start   str = "hello"
        str "hello"             /s(success)f(failure)
success syspot = "success"      /(end)
failure syspot = "failure"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "success\ndone\n");
}

TEST_F(ControlFlowTest, SuccessFailureGoto_FailurePath)
{
    std::string program = R"(
start   str = "hello"
        str "goodbye"           /s(success)f(failure)
success syspot = "success"      /(end)
failure syspot = "failure"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "failure\ndone\n");
}

TEST_F(ControlFlowTest, SuccessOnlyGoto)
{
    std::string program = R"(
start   str = "hello"
        str "hello"             /s(success)
        syspot = "continued"    /(end)
success syspot = "success"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "success\ndone\n");
}

TEST_F(ControlFlowTest, SuccessOnlyGoto_NoMatch)
{
    std::string program = R"(
start   str = "hello"
        str "goodbye"           /s(success)
        syspot = "continued"    /(end)
success syspot = "success"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "continued\ndone\n");
}

TEST_F(ControlFlowTest, FailureOnlyGoto)
{
    std::string program = R"(
start   str = "hello"
        str "goodbye"           /f(failure)
        syspot = "continued"    /(end)
failure syspot = "failure"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "failure\ndone\n");
}

TEST_F(ControlFlowTest, FailureOnlyGoto_Match)
{
    std::string program = R"(
start   str = "hello"
        str "hello"             /f(failure)
        syspot = "continued"    /(end)
failure syspot = "failure"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "continued\ndone\n");
}

TEST_F(ControlFlowTest, PatternMatchWithGoto)
{
    std::string program = R"(
start       str = "test"
            str "test"              /s(found)f(notfound)
found       syspot = "found"        /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(ControlFlowTest, DISABLED_AssignmentWithGoto)
{
    std::string program = R"(
start    x = "10", next
next    syspot = x
end return
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
start   count = "0"
loop    count = count + "1"
        count = "5"             /s(done)f(loop)
done    syspot = count
end     syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "5\nend\n");
}

TEST_F(ControlFlowTest, DISABLED_ConditionalExecution)
{
    std::string program = R"(
start   x = "10"
        x = "0"                 /s(zero)f(nonzero)
zero    syspot = "zero"         /(end)
nonzero syspot = "nonzero"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "nonzero\ndone\n");
}

TEST_F(ControlFlowTest, ConditionalExecution_ZeroCase)
{
    std::string program = R"(
start   x = "0"
        x = "0"                 /s(zero)f(nonzero)
zero    syspot = "zero"         /(end)
nonzero syspot = "nonzero"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "zero\ndone\n");
}

TEST_F(ControlFlowTest, EarlyExitPattern)
{
    std::string program = R"(
start       syspot = "start"        /(check)
check       x = "0"
            x = "0"                 /s(exit)f(continue)
exit        syspot = "exit"         /(end)
continue    syspot = "continue"
end         syspot = "end"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "start\nexit\nend\n");
}

TEST_F(ControlFlowTest, SuccessFailureChaining)
{
    std::string program = R"(
start   str = "test"
        str "test"                  /s(first)f(fail)
first   str "t"                     /s(second)f(fail)
second  syspot = "both matched"     /(end)
fail    syspot = "failed"
end     syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "both matched\ndone\n");
}

TEST_F(ControlFlowTest, DISABLED_NestedConditionals)
{
    std::string program = R"(
start       x = "5"
            x = "0"                 /s(zero)f(checkpos)
zero        syspot = "zero"         /(end)
checkpos    x = "0"                 /s(neg)f(pos)
neg         syspot = "negative"     /(end)
pos         syspot = "positive"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "positive\ndone\n");
}

TEST_F(ControlFlowTest, ComplexControlFlow)
{
    std::string program = R"(
start   count = "0"
loop    count = count + "1"
        count = "3"                 /s(done)f(loop)
done    syspot = count
        count = "5"                 /s(end)f(more)
more    syspot = "more"             /(end)
end     syspot = "finished"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "3\nfinished\n");
}
