#include <memory>

#include "sno.h"

//
// Main entry point for the Snobol III interpreter.
// Opens input file if provided, initializes built-in symbols, compiles program,
// and executes it starting from the "start" label if defined.
//
int main(int argc, char *argv[])
{
    Node *a, *b, *c;
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
    ctx.lookf     = ctx.init("f", EXPR_VAR_REF);
    ctx.looks     = ctx.init("s", EXPR_VAR_REF);
    ctx.lookend   = ctx.init("end", EXPR_VAR_REF);
    ctx.lookstart = ctx.init("start", EXPR_VAR_REF);
    ctx.lookdef   = ctx.init("define", EXPR_VAR_REF);
    ctx.lookret   = ctx.init("return", EXPR_VAR_REF);
    ctx.lookfret  = ctx.init("freturn", EXPR_VAR_REF);
    ctx.init("syspit", EXPR_SYSPIT);
    ctx.init("syspot", EXPR_SYSPOT);

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
