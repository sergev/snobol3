/*
 *   Snobol III
 */
#include "sno.h"

int freesize              = 0;
node_t *freespace         = NULL;
node_t *freespace_current = NULL;
node_t *freespace_end     = NULL;

int fin  = 0;
int fout = 1;

/* Global variables */
int cfail         = 0;
int rfail         = 0;
node_t *freelist  = NULL;
node_t *namelist  = NULL;
int lc            = 0;
node_t *schar     = NULL;
node_t *lookf     = NULL;
node_t *looks     = NULL;
node_t *lookend   = NULL;
node_t *lookstart = NULL;
node_t *lookdef   = NULL;
node_t *lookret   = NULL;
node_t *lookfret  = NULL;

//
// Print a message string to output.
//
void mes(const char *s)
{
    sysput(cstr_to_node(s));
}

//
// Initialize a symbol in the name table with a given type.
// Creates a node from the string, looks it up (or creates it), and sets its type.
//
node_t *init(const char *s, int t)
{
    node_t *a, *b;

    a = cstr_to_node(s);
    b = look(a);
    delete_string(a);
    b->typ = t;
    return (b);
}

//
// System function to read a line from input (syspit).
// Reads characters until newline or EOF, returns a string node or NULL on failure.
//
node_t *syspit(void)
{
    node_t *b, *c, *d;
    int a;

    if ((a = getchar()) == '\n')
        return (NULL);
    b = c = alloc();
    while (a != '\n') {
        c->p1 = d = alloc();
        c         = d;
    l:
        c->ch = a;
        if (a == '\0') {
            // Handle EOF: close file if open, then read from stdin
            if (fin) {
                close(fin);
                fin = 0;
                a   = getchar();
                goto l;
            }
            rfail = 1;
            break;
        }
        a = getchar();
    }
    b->p2 = c;
    if (rfail) {
        delete_string(b);
        b = NULL;
    }
    return (b);
}

//
// System function to write a string to output (syspot).
// Outputs the string followed by a newline character.
//
void syspot(node_t *string)
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
node_t *cstr_to_node(const char *s)
{
    int c;
    node_t *e, *f, *d;

    // Build linked list: d is head, f tracks tail, e is new node
    d = f = alloc();
    while ((c = *s++) != '\0') {
        (e = alloc())->ch = c;
        f->p1             = e;
        f                 = e;
    }
    d->p2 = e; // Store end marker in head node
    return (d);
}

//
// Classify a character for lexical analysis.
// Returns a numeric code representing the character's syntactic role.
//
int char_class(int c)
{
    switch (c) {
    case ')':
        return (1); // Right parenthesis
    case '(':
        return (2); // Left parenthesis
    case '\t':
    case ' ':
        return (3); // Whitespace
    case '+':
        return (4); // Plus operator
    case '-':
        return (5); // Minus operator
    case '*':
        return (6); // Asterisk operator
    case '/':
        return (7); // Division operator
    case '$':
        return (8); // Dollar sign
    case '"':
    case '\'':
        return (9); // String delimiter
    case '=':
        return (10); // Equals sign
    case ',':
        return (11); // Comma
    }
    return (0); // Other character
}

//
// Allocate a new node from the memory pool.
// Uses a free list if available, otherwise allocates from the current memory block.
// Allocates a new block of 200 nodes when the current block is exhausted.
//
node_t *alloc(void)
{
    node_t *f;
    size_t alloc_size;

    if (freelist == NULL) {
        if (freespace_current == NULL || freespace_current >= freespace_end) {
            alloc_size = 200 * sizeof(node_t);
            if (freespace == NULL) {
                freespace = (node_t *)malloc(alloc_size);
                if (freespace == NULL) {
                    flush();
                    write(fout, "Out of free space\n", 18);
                    exit(1);
                }
                freespace_current = freespace;
                freespace_end     = freespace + 200;
                freesize          = 200;
            } else {
                /* Allocate new block and append */
                node_t *new_block = (node_t *)malloc(alloc_size);
                if (new_block == NULL) {
                    flush();
                    write(fout, "Out of free space\n", 18);
                    exit(1);
                }
                freespace_current = new_block;
                freespace_end     = new_block + 200;
                freesize          = 200;
            }
        }
        f = freespace_current++;
        freesize--;
        return (f);
    }
    // Reuse node from free list
    f        = freelist;
    freelist = freelist->p1;
    return (f);
}

//
// Free a node by adding it to the free list for reuse.
//
void free_node(node_t *pointer)
{
    pointer->p1 = freelist;
    freelist    = pointer;
}

//
// Count the number of free nodes available (both in current block and free list).
//
int nfree(void)
{
    int i;
    node_t *a;

    i = freesize;
    a = freelist;
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
node_t *look(node_t *string)
{
    node_t *i, *j, *k;

    k = NULL;
    i = namelist;
    // Search existing symbols
    while (i) {
        j = i->p1;
        if (equal(j->p1, string) == 0)
            return (j);
        i = (k = i)->p2;
    }
    // Symbol not found, create new entry
    i     = alloc();
    i->p2 = NULL;
    if (k)
        k->p2 = i;
    else
        namelist = i;
    j      = alloc();
    i->p1  = j;
    j->p1  = copy(string);
    j->p2  = NULL;
    j->typ = 0;
    return (j);
}

//
// Create a copy of a string node.
// Allocates new nodes and copies all characters from the source string.
//
node_t *copy(node_t *string)
{
    node_t *j, *l, *m;
    node_t *i, *k;

    if (string == NULL)
        return (NULL);
    i = l = alloc();
    j     = string;
    k     = string->p2;
    while (j != k) {
        m     = alloc();
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
int strbin(node_t *string)
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
    if (char_class(p->ch) == 5) { /* minus */
        sign = -1;
        if (p == q)
            return (0);
        p = p->p1;
    }
loop:
    m = p->ch - '0';
    if (m > 9 || m < 0)
        writes("bad integer string");
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
node_t *binstr(int binary)
{
    int n, sign;
    node_t *m, *p, *q;

    n    = binary;
    p    = alloc();
    q    = alloc();
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
            m     = alloc();
            m->p1 = q;
            q     = m;
            q->ch = '-';
        }
        p->p1 = q;
        return (p);
    }
    m     = alloc();
    m->p1 = q;
    q     = m;
    goto loop;
}

//
// Add two numeric strings and return the result as a string.
//
node_t *add(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) + strbin(string2)));
}

//
// Subtract two numeric strings and return the result as a string.
//
node_t *sub(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) - strbin(string2)));
}

//
// Multiply two numeric strings and return the result as a string.
//
node_t *mult(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) * strbin(string2)));
}

//
// Divide two numeric strings and return the result as a string.
//
node_t *divide(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) / strbin(string2)));
}

//
// Concatenate two strings, creating new copies.
// Returns a new string node containing the concatenation.
//
node_t *cat(node_t *string1, node_t *string2)
{
    node_t *a, *b;

    if (string1 == NULL)
        return (copy(string2));
    if (string2 == NULL)
        return (copy(string1));
    a         = copy(string1);
    b         = copy(string2);
    a->p2->p1 = b->p1;
    a->p2     = b->p2;
    free_node(b);
    return (a);
}

//
// Concatenate two strings and delete the original strings (destructive concatenation).
// Used when the original strings are no longer needed.
//
node_t *dcat(node_t *a, node_t *b)
{
    node_t *c;

    c = cat(a, b);
    delete_string(a);
    delete_string(b);
    return (c);
}

//
// Delete a string by freeing all its component nodes.
// Traverses the linked list and returns each node to the free list.
//
void delete_string(node_t *string)
{
    node_t *a, *b, *c;

    if (string == NULL)
        return;
    a = string;
    b = string->p2;
    while (a != b) {
        c = a->p1;
        free_node(a);
        a = c;
    }
    free_node(a);
}

//
// Output a string and then delete it (system put with cleanup).
//
void sysput(node_t *string)
{
    syspot(string);
    delete_string(string);
}

//
// Dump the entire symbol table for debugging.
//
void dump(void)
{
    dump1(namelist);
}

//
// Recursively dump symbol table entries starting from a base node.
// Outputs symbol names, types, and values for debugging purposes.
//
void dump1(node_t *base)
{
    node_t *b, *c, *e;
    node_t *d;

    while (base) {
        b = base->p1;
        c = binstr(b->typ);
        d = cstr_to_node("  ");
        e = dcat(c, d);
        sysput(cat(e, b->p1));
        delete_string(e);
        if (b->typ == 1) {
            c = cstr_to_node("   ");
            sysput(cat(c, b->p2));
            delete_string(c);
        }
        base = base->p2;
    }
}

//
// Write an error message with line number and handle error recovery.
// If compilation is in progress, skips to end of statement and recompiles.
//
void writes(const char *s)
{
    sysput(dcat(binstr(lc), dcat(cstr_to_node("\t"), cstr_to_node(s))));
    flush();
    if (cfail) {
        dump();
        flush();
        exit(1);
    }
    // Error recovery: skip to end of current statement
    while (getc_char() != NULL)
        ;
    while (compile() != NULL)
        ;
    flush();
    exit(1);
}

//
// Get the next character from the current input line.
// Reads a new line when the current one is exhausted.
// Returns NULL at end of line (after all characters have been consumed).
//
node_t *getc_char(void)
{
    node_t *a;
    static node_t *line;
    static int linflg;

    while (line == NULL) {
        line = syspit();
        if (rfail) {
            cfail++;
            writes("eof on input");
        }
        lc++;
    }
    if (linflg) {
        line   = NULL;
        linflg = 0;
        return (NULL);
    }
    a = line->p1;
    if (a == line->p2) {
        free_node(line);
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
