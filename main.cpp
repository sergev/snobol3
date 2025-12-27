#include <fstream>

#include "sno.h"

//
// Main entry point for the Snobol III interpreter.
// Opens input file if provided, initializes built-in symbols, compiles program,
// and executes it starting from the "start" label if defined.
//
int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Usage: sno FILE\n";
        return 1;
    }

    // Open input file
    std::ifstream file_input(argv[1]);
    if (!file_input.is_open()) {
        std::cerr << "cannot open input" << std::endl;
        return 1;
    }

    // Create context with stream references
    SnobolContext ctx(std::cout);

    // Compile program from file and execute with input from stdin
    ctx.compile_program(file_input);
    ctx.execute_program(std::cin);

    return 0;
}
