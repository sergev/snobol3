#include "test_helpers.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

extern "C" {
#include "sno.h"
}

// Helper function to convert node_t string back to std::string
std::string node_to_string(node_t *str)
{
    if (str == nullptr) {
        return "";
    }
    std::string result;
    node_t *a = str;
    node_t *b = str->p2;
    while (a != b) {
        a = a->p1;
        result += a->ch;
    }
    return result;
}

// Helper function to compare node_t string with C string
bool node_equals_cstr(node_t *str, const char *cstr)
{
    if (str == nullptr && cstr == nullptr) {
        return true;
    }
    if (str == nullptr || cstr == nullptr) {
        return false;
    }
    return node_to_string(str) == cstr;
}
// Run a Snobol program by executing the sno binary
SnobolTestResult run_snobol_program(const std::string &program, const std::string &input)
{
    SnobolTestResult result;
    result.success   = false;
    result.exit_code = -1;

    // Create temporary file for the program
    char tmp_program[] = "/tmp/snobol_test_XXXXXX";
    int fd_program     = mkstemp(tmp_program);
    if (fd_program < 0) {
        return result;
    }

    // Write program to temp file
    write(fd_program, program.c_str(), program.length());
    close(fd_program);

    // Create temporary file for input (if provided)
    char tmp_input[] = "/tmp/snobol_input_XXXXXX";
    int fd_input     = -1;
    if (!input.empty()) {
        fd_input = mkstemp(tmp_input);
        if (fd_input < 0) {
            unlink(tmp_program);
            return result;
        }
        write(fd_input, input.c_str(), input.length());
        close(fd_input);
    }

    // Create temporary files for output capture
    char tmp_stdout[] = "/tmp/snobol_stdout_XXXXXX";
    char tmp_stderr[] = "/tmp/snobol_stderr_XXXXXX";
    int fd_stdout     = mkstemp(tmp_stdout);
    int fd_stderr     = mkstemp(tmp_stderr);

    if (fd_stdout < 0 || fd_stderr < 0) {
        unlink(tmp_program);
        if (fd_input >= 0)
            unlink(tmp_input);
        if (fd_stdout >= 0)
            close(fd_stdout);
        if (fd_stderr >= 0)
            close(fd_stderr);
        return result;
    }
    close(fd_stdout);
    close(fd_stderr);

    // Find the sno executable (in build directory)
    // BUILD_DIR is defined by CMake as a string
    std::string sno_exe = std::string(BUILD_DIR) + "/sno";

    // Fork and execute
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        // Redirect stdin from input file if provided, otherwise /dev/null
        if (fd_input >= 0) {
            int input_fd = open(tmp_input, O_RDONLY);
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        } else {
            int null_fd = open("/dev/null", O_RDONLY);
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }

        // Redirect stdout and stderr
        int stdout_fd = open(tmp_stdout, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        int stderr_fd = open(tmp_stderr, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(stdout_fd, STDOUT_FILENO);
        dup2(stderr_fd, STDERR_FILENO);
        close(stdout_fd);
        close(stderr_fd);

        // Execute sno
        execl(sno_exe.c_str(), "sno", tmp_program, (char *)NULL);
        exit(1);
    } else if (pid > 0) {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        result.exit_code = WEXITSTATUS(status);
        result.success   = (result.exit_code == 0);

        // Read stdout
        std::ifstream stdout_file(tmp_stdout);
        result.stdout_output = std::string((std::istreambuf_iterator<char>(stdout_file)),
                                           std::istreambuf_iterator<char>());
        stdout_file.close();

        // Read stderr
        std::ifstream stderr_file(tmp_stderr);
        result.stderr_output = std::string((std::istreambuf_iterator<char>(stderr_file)),
                                           std::istreambuf_iterator<char>());
        stderr_file.close();

        // Cleanup
        unlink(tmp_program);
        if (fd_input >= 0)
            unlink(tmp_input);
        unlink(tmp_stdout);
        unlink(tmp_stderr);
    } else {
        // Fork failed
        unlink(tmp_program);
        if (fd_input >= 0)
            unlink(tmp_input);
        unlink(tmp_stdout);
        unlink(tmp_stderr);
    }

    return result;
}
