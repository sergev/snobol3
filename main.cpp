#include <fstream>

#include "sno.h"

//
// Main entry point for the Snobol III interpreter.
// Opens input file if provided, initializes built-in symbols, compiles program,
// and executes it starting from the "start" label if defined.
//
int main(int argc, char *argv[])
{
    Node *a, *b, *c;

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
    SnobolContext ctx(file_input, std::cout);

    // Initialize built-in symbols
    ctx.lookf     = &ctx.init("f", Token::EXPR_VAR_REF);
    ctx.looks     = &ctx.init("s", Token::EXPR_VAR_REF);
    ctx.lookend   = &ctx.init("end", Token::EXPR_VAR_REF);
    ctx.lookstart = &ctx.init("start", Token::EXPR_VAR_REF);
    ctx.lookdef   = &ctx.init("define", Token::EXPR_VAR_REF);
    ctx.lookret   = &ctx.init("return", Token::EXPR_VAR_REF);
    ctx.lookfret  = &ctx.init("freturn", Token::EXPR_VAR_REF);
    ctx.init("syspit", Token::EXPR_SYSPIT);
    ctx.init("syspot", Token::EXPR_SYSPOT);

    // Compile all statements until "end" is encountered
    // Link statements together in a list
    a = c = ctx.compile();
    while (ctx.lookend->typ != Token::EXPR_LABEL) {
        a->p1 = b = ctx.compile();
        a         = b;
    }
    ctx.cfail = 1;       // Enable compilation failure mode
    a->p1     = nullptr; // Terminate statement list

    // Start execution from "start" label if defined, otherwise from first statement
    if (ctx.lookstart->typ == Token::EXPR_LABEL)
        c = ctx.lookstart->p2;
    while ((c = ctx.execute(*c)) != nullptr)
        ;
    ctx.flush();
    return 0;
}
