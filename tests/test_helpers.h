#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Structure to hold test results
struct SnobolTestResult {
    std::string stdout_output;
    std::string stderr_output;
    int exit_code;
    bool success;
};

// Run a Snobol program from a string with optional input
// Returns the output captured from stdout
SnobolTestResult run_snobol_program(const std::string &program, const std::string &input = "");

#endif // TEST_HELPERS_H
