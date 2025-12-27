#include <memory>

#include "sno.h"

//
// Main entry point for the Snobol III interpreter.
// Opens input file if provided, initializes built-in symbols, compiles program,
// and executes it starting from the "start" label if defined.
//
int main(int argc, char *argv[])
{
    node_t *a, *b, *c;
    std::unique_ptr<std::ifstream> file_input;
    std::istream *input_stream = &std::cin;

    // Open input file if provided
    if (argc > 1) {
        file_input = std::make_unique<std::ifstream>(argv[1]);
        if (!file_input->is_open()) {
            std::cerr << "cannot open input" << std::endl;
            return 1;
        }
        input_stream = file_input.get();
    }

    // Create context with stream references
    SnobolContext ctx(*input_stream, std::cout);

    // Initialize built-in symbols
    ctx.lookf     = ctx.init("f", 0);
    ctx.looks     = ctx.init("s", 0);
    ctx.lookend   = ctx.init("end", 0);
    ctx.lookstart = ctx.init("start", 0);
    ctx.lookdef   = ctx.init("define", 0);
    ctx.lookret   = ctx.init("return", 0);
    ctx.lookfret  = ctx.init("freturn", 0);
    ctx.init("syspit", 3);
    ctx.init("syspot", 4);

    // Compile all statements until "end" is encountered
    // Link statements together in a list
    a = c = ctx.compile();
    while (ctx.lookend->typ != EXPR_LABEL) {
        a->p1 = b = ctx.compile();
        a         = b;
    }
    ctx.cfail = 1;       // Enable compilation failure mode
    a->p1     = nullptr; // Terminate statement list

    // Start execution from "start" label if defined, otherwise from first statement
    if (ctx.lookstart->typ == EXPR_LABEL)
        c = ctx.lookstart->p2;
    while ((c = ctx.execute(c)) != nullptr)
        ;
    ctx.flush();
    return 0;
}
