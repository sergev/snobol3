#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// Forward declaration
struct node;
typedef struct node node_t;

// Structure to hold test results
struct SnobolTestResult {
    std::string stdout_output;
    std::string stderr_output;
    int exit_code;
    bool success;
};

// Run a Snobol program from a string with optional input
// Returns the output captured from stdout
SnobolTestResult run_snobol_program(const std::string& program, const std::string& input = "");

// Helper to convert node_t string to std::string (from basic_tests.cpp)
std::string node_to_string(node_t *str);

// Helper to compare node_t string with C string
bool node_equals_cstr(node_t *str, const char *cstr);

#endif // TEST_HELPERS_H

