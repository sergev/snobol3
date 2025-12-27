/*
 *   Snobol III
 */
#include "sno.h"

//
// Create and initialize a new Snobol interpreter context
//
snobol_context_t *snobol_context_create(void)
{
    snobol_context_t *ctx = (snobol_context_t *)malloc(sizeof(snobol_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    // Initialize memory management fields
    ctx->freesize          = 0;
    ctx->freespace         = NULL;
    ctx->freespace_current = NULL;
    ctx->freespace_end     = NULL;
    ctx->freelist          = NULL;

    // Initialize symbol table
    ctx->namelist  = NULL;
    ctx->lookf     = NULL;
    ctx->looks     = NULL;
    ctx->lookend   = NULL;
    ctx->lookstart = NULL;
    ctx->lookdef   = NULL;
    ctx->lookret   = NULL;
    ctx->lookfret  = NULL;

    // Initialize execution state
    ctx->cfail = 0;
    ctx->rfail = 0;
    ctx->lc    = 0;
    ctx->schar = NULL;

    // Initialize I/O
    ctx->fin  = 0;
    ctx->fout = 1;

    return ctx;
}

//
// Print a message string to output.
//
void mes(snobol_context_t *ctx, const char *s)
{
    sysput(ctx, cstr_to_node(ctx, s));
}

//
// Initialize a symbol in the name table with a given type.
// Creates a node from the string, looks it up (or creates it), and sets its type.
//
node_t *init(snobol_context_t *ctx, const char *s, int t)
{
    node_t *a, *b;

    a = cstr_to_node(ctx, s);
    b = look(ctx, a);
    delete_string(ctx, a);
    b->typ = t;
    return (b);
}

//
// System function to read a line from input (syspit).
// Reads characters until newline or EOF, returns a string node or NULL on failure.
//
node_t *syspit(snobol_context_t *ctx)
{
    node_t *b, *c, *d;
    int a;

    a = getchar();
    if (a == '\n')
        return (NULL);
    if (a == EOF) {
        ctx->rfail = 1;
        return (NULL);
    }
    b = c = alloc(ctx);
    while (a != '\n') {
        c->p1 = d = alloc(ctx);
        c         = d;
    l:
        c->ch = a;
        if (a == EOF || a == '\0') {
            // Handle EOF: close file if open, then read from stdin
            if (ctx->fin && a == '\0') {
                close(ctx->fin);
                ctx->fin = 0;
                a        = getchar();
                goto l;
            }
            ctx->rfail = 1;
            break;
        }
        a = getchar();
        if (a == EOF) {
            ctx->rfail = 1;
            break;
        }
    }
    b->p2 = c;
    if (ctx->rfail) {
        delete_string(ctx, b);
        b = NULL;
    }
    return (b);
}

//
// System function to write a string to output (syspot).
// Outputs the string followed by a newline character.
//
void syspot(snobol_context_t *ctx __attribute__((unused)), node_t *string)
{
    node_t *a, *b, *s;

    s = string;
    if (s != NULL) {
        a = s;
        b = s->p2;
        while (a != b) {
            a = a->p1;
            putchar(a->ch);
        }
    }
    putchar('\n');
}

//
// Convert a C string to a Snobol string node.
// Creates a linked list of nodes representing the string characters.
//
node_t *cstr_to_node(snobol_context_t *ctx, const char *s)
{
    int c;
    node_t *e, *f, *d;

    // Build linked list: d is head, f tracks tail, e is new node
    d = f = alloc(ctx);
    while ((c = *s++) != '\0') {
        (e = alloc(ctx))->ch = c;
        f->p1                = e;
        f                    = e;
    }
    d->p2 = e; // Store end marker in head node
    return (d);
}

//
// Classify a character for lexical analysis.
// Returns a numeric code representing the character's syntactic role.
//
char_class_t char_class(int c)
{
    switch (c) {
    case ')':
        return (CHAR_CLASS_RPAREN); // Right parenthesis
    case '(':
        return (CHAR_CLASS_LPAREN); // Left parenthesis
    case '\t':
    case ' ':
        return (CHAR_CLASS_WHITESPACE); // Whitespace
    case '+':
        return (CHAR_CLASS_PLUS); // Plus operator
    case '-':
        return (CHAR_CLASS_MINUS); // Minus operator
    case '*':
        return (CHAR_CLASS_ASTERISK); // Asterisk operator
    case '/':
        return (CHAR_CLASS_SLASH); // Division operator
    case '$':
        return (CHAR_CLASS_DOLLAR); // Dollar sign
    case '"':
    case '\'':
        return (CHAR_CLASS_STRING_DELIM); // String delimiter
    case '=':
        return (CHAR_CLASS_EQUALS); // Equals sign
    case ',':
        return (CHAR_CLASS_COMMA); // Comma
    }
    return (CHAR_CLASS_OTHER); // Other character
}

//
// Allocate a new node from the memory pool.
// Uses a free list if available, otherwise allocates from the current memory block.
// Allocates a new block of 200 nodes when the current block is exhausted.
//
node_t *alloc(snobol_context_t *ctx)
{
    node_t *f;
    size_t alloc_size;

    if (ctx->freelist == NULL) {
        if (ctx->freespace_current == NULL || ctx->freespace_current >= ctx->freespace_end) {
            alloc_size = 200 * sizeof(node_t);
            if (ctx->freespace == NULL) {
                ctx->freespace = (node_t *)malloc(alloc_size);
                if (ctx->freespace == NULL) {
                    flush();
                    write(ctx->fout, "Out of free space\n", 18);
                    exit(1);
                }
                ctx->freespace_current = ctx->freespace;
                ctx->freespace_end     = ctx->freespace + 200;
                ctx->freesize          = 200;
            } else {
                /* Allocate new block and append */
                node_t *new_block = (node_t *)malloc(alloc_size);
                if (new_block == NULL) {
                    flush();
                    write(ctx->fout, "Out of free space\n", 18);
                    exit(1);
                }
                ctx->freespace_current = new_block;
                ctx->freespace_end     = new_block + 200;
                ctx->freesize          = 200;
            }
        }
        f = ctx->freespace_current++;
        ctx->freesize--;
        return (f);
    }
    // Reuse node from free list
    f             = ctx->freelist;
    ctx->freelist = ctx->freelist->p1;
    return (f);
}

//
// Free a node by adding it to the free list for reuse.
//
void free_node(snobol_context_t *ctx, node_t *pointer)
{
    pointer->p1   = ctx->freelist;
    ctx->freelist = pointer;
}

//
// Count the number of free nodes available (both in current block and free list).
//
int nfree(snobol_context_t *ctx)
{
    int i;
    node_t *a;

    i = ctx->freesize;
    a = ctx->freelist;
    while (a) {
        a = a->p1;
        i++;
    }
    return (i);
}

//
// Look up a symbol in the name table, creating it if it doesn't exist.
// Returns a pointer to the symbol's value node.
//
node_t *look(snobol_context_t *ctx, node_t *string)
{
    node_t *i, *j, *k;

    k = NULL;
    i = ctx->namelist;
    // Search existing symbols
    while (i) {
        j = i->p1;
        if (equal(j->p1, string) == 0)
            return (j);
        i = (k = i)->p2;
    }
    // Symbol not found, create new entry
    i     = alloc(ctx);
    i->p2 = NULL;
    if (k)
        k->p2 = i;
    else
        ctx->namelist = i;
    j      = alloc(ctx);
    i->p1  = j;
    j->p1  = copy(ctx, string);
    j->p2  = NULL;
    j->typ = EXPR_VAR_REF;
    return (j);
}

//
// Create a copy of a string node.
// Allocates new nodes and copies all characters from the source string.
//
node_t *copy(snobol_context_t *ctx, node_t *string)
{
    node_t *j, *l, *m;
    node_t *i, *k;

    if (string == NULL)
        return (NULL);
    i = l = alloc(ctx);
    j     = string;
    k     = string->p2;
    while (j != k) {
        m     = alloc(ctx);
        m->ch = (j = j->p1)->ch;
        l->p1 = m;
        l     = m;
    }
    i->p2 = l;
    return (i);
}

//
// Compare two strings lexicographically.
// Returns 0 if equal, 1 if string1 > string2, -1 if string1 < string2.
//
int equal(node_t *string1, node_t *string2)
{
    node_t *i, *j, *k;
    node_t *l;
    int n, m;

    if (string1 == NULL) {
        if (string2 == NULL)
            return (0);
        return (-1);
    }
    if (string2 == NULL)
        return (1);
    // Compare character by character
    i = string1;
    j = string1->p2; // End marker for string1
    k = string2;
    l = string2->p2; // End marker for string2
    for (;;) {
        m = (i = i->p1)->ch; // Next char from string1
        n = (k = k->p1)->ch; // Next char from string2
        if (m > n)
            return (1);
        if (m < n)
            return (-1);
        if (i == j) {
            if (k == l)
                return (0);
            return (-1);
        }
        if (k == l)
            return (1);
    }
}

//
// Convert a string node representing a number to an integer.
// Handles negative numbers and validates digit characters.
//
int strbin(snobol_context_t *ctx, node_t *string)
{
    int n, m, sign;
    node_t *p, *q, *s;

    s = string;
    n = 0;
    if (s == NULL)
        return (0);
    p    = s->p1;
    q    = s->p2;
    sign = 1;
    if (char_class(p->ch) == CHAR_CLASS_MINUS) { /* minus */
        sign = -1;
        if (p == q)
            return (0);
        p = p->p1;
    }
loop:
    m = p->ch - '0';
    if (m > 9 || m < 0)
        writes(ctx, "bad integer string");
    n = n * 10 + m;
    if (p == q)
        return (n * sign);
    p = p->p1;
    goto loop;
}

//
// Convert an integer to a string node.
// Builds the string representation digit by digit, handling negative numbers.
//
node_t *binstr(snobol_context_t *ctx, int binary)
{
    int n, sign;
    node_t *m, *p, *q;

    n    = binary;
    p    = alloc(ctx);
    q    = alloc(ctx);
    sign = 1;
    if (binary < 0) {
        sign = -1;
        n    = -binary;
    }
    p->p2 = q;
loop:
    q->ch = n % 10 + '0';
    n     = n / 10;
    if (n == 0) {
        if (sign < 0) {
            m     = alloc(ctx);
            m->p1 = q;
            q     = m;
            q->ch = '-';
        }
        p->p1 = q;
        return (p);
    }
    m     = alloc(ctx);
    m->p1 = q;
    q     = m;
    goto loop;
}

//
// Add two numeric strings and return the result as a string.
//
node_t *add(snobol_context_t *ctx, node_t *string1, node_t *string2)
{
    return (binstr(ctx, strbin(ctx, string1) + strbin(ctx, string2)));
}

//
// Subtract two numeric strings and return the result as a string.
//
node_t *sub(snobol_context_t *ctx, node_t *string1, node_t *string2)
{
    return (binstr(ctx, strbin(ctx, string1) - strbin(ctx, string2)));
}

//
// Multiply two numeric strings and return the result as a string.
//
node_t *mult(snobol_context_t *ctx, node_t *string1, node_t *string2)
{
    return (binstr(ctx, strbin(ctx, string1) * strbin(ctx, string2)));
}

//
// Divide two numeric strings and return the result as a string.
//
node_t *divide(snobol_context_t *ctx, node_t *string1, node_t *string2)
{
    return (binstr(ctx, strbin(ctx, string1) / strbin(ctx, string2)));
}

//
// Concatenate two strings, creating new copies.
// Returns a new string node containing the concatenation.
//
node_t *cat(snobol_context_t *ctx, node_t *string1, node_t *string2)
{
    node_t *a, *b;

    if (string1 == NULL)
        return (copy(ctx, string2));
    if (string2 == NULL)
        return (copy(ctx, string1));
    a         = copy(ctx, string1);
    b         = copy(ctx, string2);
    a->p2->p1 = b->p1;
    a->p2     = b->p2;
    free_node(ctx, b);
    return (a);
}

//
// Concatenate two strings and delete the original strings (destructive concatenation).
// Used when the original strings are no longer needed.
//
node_t *dcat(snobol_context_t *ctx, node_t *a, node_t *b)
{
    node_t *c;

    c = cat(ctx, a, b);
    delete_string(ctx, a);
    delete_string(ctx, b);
    return (c);
}

//
// Delete a string by freeing all its component nodes.
// Traverses the linked list and returns each node to the free list.
//
void delete_string(snobol_context_t *ctx, node_t *string)
{
    node_t *a, *b, *c;

    if (string == NULL)
        return;
    a = string;
    b = string->p2;
    while (a != b) {
        c = a->p1;
        free_node(ctx, a);
        a = c;
    }
    free_node(ctx, a);
}

//
// Output a string and then delete it (system put with cleanup).
//
void sysput(snobol_context_t *ctx, node_t *string)
{
    syspot(ctx, string);
    delete_string(ctx, string);
}

//
// Dump the entire symbol table for debugging.
//
void dump(snobol_context_t *ctx)
{
    dump1(ctx, ctx->namelist);
}

//
// Recursively dump symbol table entries starting from a base node.
// Outputs symbol names, types, and values for debugging purposes.
//
void dump1(snobol_context_t *ctx, node_t *base)
{
    node_t *b, *c, *e;
    node_t *d;

    while (base) {
        b = base->p1;
        c = binstr(ctx, b->typ);
        d = cstr_to_node(ctx, "  ");
        e = dcat(ctx, c, d);
        sysput(ctx, cat(ctx, e, b->p1));
        delete_string(ctx, e);
        if (b->typ == EXPR_VALUE) {
            c = cstr_to_node(ctx, "   ");
            sysput(ctx, cat(ctx, c, b->p2));
            delete_string(ctx, c);
        }
        base = base->p2;
    }
}

//
// Write an error message with line number and handle error recovery.
// If compilation is in progress, skips to end of statement and recompiles.
//
void writes(snobol_context_t *ctx, const char *s)
{
    sysput(ctx, dcat(ctx, binstr(ctx, ctx->lc),
                     dcat(ctx, cstr_to_node(ctx, "\t"), cstr_to_node(ctx, s))));
    flush();
    if (ctx->cfail) {
        dump(ctx);
        flush();
        exit(1);
    }
    // Error recovery: skip to end of current statement
    while (getc_char(ctx) != NULL)
        ;
    while (compile(ctx) != NULL)
        ;
    flush();
    exit(1);
}

//
// Get the next character from the current input line.
// Reads a new line when the current one is exhausted.
// Returns NULL at end of line (after all characters have been consumed).
//
node_t *getc_char(snobol_context_t *ctx)
{
    node_t *a;
    static node_t *line;
    static int linflg;

    while (line == NULL) {
        line = syspit(ctx);
        if (ctx->rfail) {
            ctx->cfail++;
            writes(ctx, "eof on input");
        }
        ctx->lc++;
    }
    if (linflg) {
        line   = NULL;
        linflg = 0;
        return (NULL);
    }
    a = line->p1;
    if (a == line->p2) {
        free_node(ctx, line);
        linflg++;
    } else
        line->p1 = a->p1;
    return (a);
}

//
// Flush the output buffer.
//
void flush(void)
{
    fflush(stdout);
}
