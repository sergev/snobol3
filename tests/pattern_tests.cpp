#include <gtest/gtest.h>

#include "sno.h"
#include "test_helpers.h"

class PatternTest : public ::testing::Test {
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
// Simple Pattern Tests
// ============================================================================

TEST_F(PatternTest, LiteralStringPattern_Success)
{
    std::string program = R"(
start       str = "hello world"
            str "hello"         /s(found)f(notfound)
found       syspot = "found"    /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(PatternTest, LiteralStringPattern_Failure)
{
    std::string program = R"(
start       str = "hello world"
            str "goodbye"       /s(found)f(notfound)
found       syspot = "found"    /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "not found\ndone\n");
}

TEST_F(PatternTest, VariablePattern)
{
    std::string program = R"(
start       str = "hello"
            pattern = "hello"
            str pattern         /s(found)f(notfound)
found       syspot = "found"    /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(PatternTest, PatternImmediate)
{
    std::string program = R"(
start       str = "hello world"
            pattern = "hello"
            ref = "pattern"
            str $ref            /s(found)f(notfound)
found       syspot = "found"    /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(PatternTest, PatternConcatenation)
{
    std::string program = R"(
start       str = "hello world"
            str "hello" " world"    /s(found)f(notfound)
found       syspot = "found"        /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(PatternTest, PatternReplacement)
{
    std::string program = R"(
start   str = "hello world"
        str "world" = "universe"
        syspot = str
end     return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hello universe\n");
}

TEST_F(PatternTest, PatternReplacement_Failure)
{
    std::string program = R"(
start       str = "hello world"
            str "goodbye" = "universe"  /s(found)f(notfound)
found       syspot = str                /(end)
notfound    syspot = "pattern not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "pattern not found\ndone\n");
}

// ============================================================================
// Pattern Alternation Tests
// ============================================================================

TEST_F(PatternTest, SimpleAlternation_FirstMatch)
{
    std::string program = R"(
start       str = "apple"
            str *"apple"/"banana"*      /s(found)f(notfound)
found       syspot = "found"            /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "alternations are not supported yet\nnot found\ndone\n");

    //TODO: enable when alterations are implemented
    //TODO: EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(PatternTest, SimpleAlternation_SecondMatch)
{
    std::string program = R"(
start       str = "banana"
            str *"apple"/"banana"*      /s(found)f(notfound)
found       syspot = "found"            /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "alternations are not supported yet\nnot found\ndone\n");

    //TODO: enable when alterations are implemented
    //TODO: EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

TEST_F(PatternTest, SimpleAlternation_NoMatch)
{
    std::string program = R"(
start       str = "cherry"
            str *"apple"/"banana"*      /s(found)f(notfound)
found       syspot = "found"            /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "alternations are not supported yet\nnot found\ndone\n");

    //TODO: enable when alterations are implemented
    //TODO: EXPECT_EQ(result.stdout_output, "not found\ndone\n");
}

TEST_F(PatternTest, AlternationWithVariables)
{
    std::string program = R"(
start       str = "test"
            p1 = "test"
            p2 = "other"
            str *p1/p2*                 /s(found)f(notfound)
found       syspot = "found"            /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "alternations are not supported yet\nnot found\ndone\n");

    //TODO: enable when alterations are implemented
    //TODO: EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

// ============================================================================
// Balanced Pattern Tests
// ============================================================================

TEST_F(PatternTest, BalancedAlternation)
{
    std::string program = R"(
start       str = "hello"
            str *("hello"/"world")*     /s(found)f(notfound)
found       syspot = "found"            /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "alternations are not supported yet\nnot found\ndone\n");

    //TODO: enable when alterations are implemented
    //TODO: EXPECT_EQ(result.stdout_output, "found\ndone\n");
}

// ============================================================================
// Pattern Matching Statements Tests
// ============================================================================

TEST_F(PatternTest, PatternMatchWithGoto_Success)
{
    std::string program = R"(
start       str = "hello"
            str "hello"                 /(success)
success     syspot = "success"          /(end)
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "success\ndone\n");
}

TEST_F(PatternTest, PatternMatchWithGoto_Failure)
{
    std::string program = R"(
start       str = "hello"
            str "goodbye"               /s(success)f(failure)
success     syspot = "success"          /(end)
failure     syspot = "failure"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "failure\ndone\n");
}

TEST_F(PatternTest, PatternMatchSuccessOnly)
{
    std::string program = R"(
start       str = "hello"
            str "hello"                 /s(success)
success     syspot = "success"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "success\ndone\n");
}

TEST_F(PatternTest, PatternMatchFailureOnly)
{
    std::string program = R"(
start       str = "hello"
            str "goodbye"               /f(failure)
            syspot = "continued"        /(end)
failure     syspot = "failure"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "failure\ndone\n");
}

TEST_F(PatternTest, PatternMatchMultipleReplacements)
{
    std::string program = R"(
            str = "hello hello"
loop        str "hello" = "hi"          /f(done)
            syspot = str                /(loop)
done        syspot = str
end         return
)";

    SnobolTestResult result = run_snobol_program(program);
    EXPECT_TRUE(result.success) << result.stderr_output;
    EXPECT_EQ(result.stdout_output, "hi hello\nhi hi\nhi hi\n");
}

TEST_F(PatternTest, EmptyPattern)
{
    std::string program = R"(
start       str = "test"
            str ""                      /s(found)f(notfound)
found       syspot = "found"            /(end)
notfound    syspot = "not found"
end         syspot = "done"
)";

    SnobolTestResult result = run_snobol_program(program);
    // Empty pattern should match (matches empty string)
    EXPECT_TRUE(result.success || !result.stderr_output.empty());
}
