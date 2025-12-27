#include "sno.h"

//
// Main entry point for the Snobol III interpreter.
// Opens input file if provided, initializes built-in symbols, compiles program,
// and executes it starting from the "start" label if defined.
//
int main(int argc, char *argv[])
{
    node_t *a, *b, *c;
    snobol_context_t *ctx;

    // Create and initialize context
    ctx = snobol_context_create();
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create interpreter context\n");
        exit(1);
    }

    // Open input file if provided
    if (argc > 1) {
        ctx->fin = open(argv[1], 0);
        if (ctx->fin < 0) {
            mes(ctx, "cannot open input");
            exit(1);
        }
        // Redirect stdin to read from the file
        dup2(ctx->fin, 0);
        close(ctx->fin);
        ctx->fin = 0;
    }
    ctx->fout = dup(1);

    // Initialize built-in symbols
    ctx->lookf     = init(ctx, "f", 0);
    ctx->looks     = init(ctx, "s", 0);
    ctx->lookend   = init(ctx, "end", 0);
    ctx->lookstart = init(ctx, "start", 0);
    ctx->lookdef   = init(ctx, "define", 0);
    ctx->lookret   = init(ctx, "return", 0);
    ctx->lookfret  = init(ctx, "freturn", 0);
    init(ctx, "syspit", 3);
    init(ctx, "syspot", 4);

    // Compile all statements until "end" is encountered
    // Link statements together in a list
    a = c = compile(ctx);
    while (ctx->lookend->typ != EXPR_LABEL) {
        a->p1 = b = compile(ctx);
        a         = b;
    }
    ctx->cfail = 1; // Enable compilation failure mode
    a->p1      = 0; // Terminate statement list

    // Start execution from "start" label if defined, otherwise from first statement
    if (ctx->lookstart->typ == EXPR_LABEL)
        c = ctx->lookstart->p2;
    while ((c = execute(ctx, c)) != NULL)
        ;
    flush();
    return 0;
}
