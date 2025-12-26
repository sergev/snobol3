#include "sno.h"

//
// Main entry point for the Snobol III interpreter.
// Opens input file if provided, initializes built-in symbols, compiles program,
// and executes it starting from the "start" label if defined.
//
int main(int argc, char *argv[])
{
    node_t *a, *b, *c;

    // Open input file if provided
    if (argc > 1) {
        fin = open(argv[1], 0);
        if (fin < 0) {
            mes("cannot open input");
            exit(1);
        }
    }
    fout = dup(1);
    // Initialize built-in symbols
    lookf     = init("f", 0);
    looks     = init("s", 0);
    lookend   = init("end", 0);
    lookstart = init("start", 0);
    lookdef   = init("define", 0);
    lookret   = init("return", 0);
    lookfret  = init("freturn", 0);
    init("syspit", 3);
    init("syspot", 4);
    // Compile all statements until "end" is encountered
    // Link statements together in a list
    a = c = compile();
    while (lookend->typ != 2) {
        a->p1 = b = compile();
        a         = b;
    }
    cfail = 1; // Enable compilation failure mode
    a->p1 = 0; // Terminate statement list
    // Start execution from "start" label if defined, otherwise from first statement
    if (lookstart->typ == 2)
        c = lookstart->p2;
    while ((c = execute(c)) != NULL)
        ;
    flush();
    return 0;
}
