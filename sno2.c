#include "sno.h"

//
// Parse the next component (token) from the input stream.
// Returns a node representing the token with appropriate type code.
// Handles operators, literals, identifiers, and special characters.
//
node_t *compon(void)
{
    node_t *a, *b;
    int c;
    static int next;

    if (next == 0)
        schar = getc_char();
    else
        next = 0;
    if (schar == NULL) {
        (a = alloc())->typ = 0;
        return (a);
    }
    switch (char_class(schar->ch)) {
    case 1: // Right parenthesis
        schar->typ = 5;
        return (schar);

    case 2: // Left parenthesis
        schar->typ = 16;
        return (schar);

    case 3: // Whitespace
        a = schar;
        for (;;) {
            schar = getc_char();
            if (schar == NULL) {
                a->typ = 0;
                return (a);
            }
            if (char_class(schar->ch) != 3)
                break;
            free_node(schar);
        }
        next   = 1;
        a->typ = 7;
        return (a);

    case 4: // Plus operator
        schar->typ = 8;
        return (schar);

    case 5: // Minus operator
        schar->typ = 9;
        return (schar);

    case 6: // Asterisk - could be multiplication or unanchored search
        a     = schar;
        schar = getc_char();
        if (char_class(schar->ch) == 3)
            a->typ = 10; // Multiplication (followed by space)
        else
            a->typ = 1; // Unanchored search
        next = 1;
        return (a);

    case 7: // Division - could be pattern alternation
        a     = schar;
        schar = getc_char();
        if (char_class(schar->ch) == 3)
            a->typ = 11; // Division (followed by space)
        else
            a->typ = 2; // Pattern alternation
        next = 1;
        return (a);

    case 8: // Dollar sign (pattern immediate value)
        schar->typ = 12;
        return (schar);

    case 9: // String literal delimiter
        c = schar->ch;
        a = getc_char();
        if (a == NULL)
            goto lerr;
        b = schar;
        if (a->ch == c) {
            // Empty string
            free_node(schar);
            a->typ = 15;
            a->p1  = NULL;
            return (a);
        }
        b->p1 = a;
        for (;;) {
            schar = getc_char();
            if (schar == NULL) {
                // End of input reached without closing quote
                goto lerr;
            }
            if (schar->ch == c) {
                // Found closing quote - break out of loop
                break;
            }
            // Add this character to the string
            a->p1 = schar;
            a     = schar;
        }
        b->p2      = a;
        schar->typ = 15;
        schar->p1  = b;
        return (schar);
    lerr:
        writes("illegal literal string");
        return NULL; // Never reached, but needed for compilation

    case 10: // Equals sign
        schar->typ = 3;
        return (schar);

    case 11: // Comma
        schar->typ = 4;
        return (schar);
    }
    // Identifier or keyword - collect characters until delimiter
    b     = alloc();
    b->p1 = a = schar;
    schar     = getc_char();
    while (schar != NULL && !char_class(schar->ch)) {
        a->p1 = schar;
        a     = schar;
        schar = getc_char();
    }
    b->p2 = a;
    next  = 1;
    a     = look(b);
    delete_string(b);
    b      = alloc();
    b->typ = 14; // Variable reference
    b->p1  = a;
    return (b);
}

//
// Get the next non-space component (skip whitespace tokens).
//
node_t *nscomp(void)
{
    node_t *c;

    while ((c = compon())->typ == 7)
        free_node(c);
    return (c);
}

//
// Push an element onto a stack (implemented as a linked list).
// Returns the new top of stack.
//
node_t *push(node_t *stack)
{
    node_t *a;

    (a = alloc())->p2 = stack;
    return (a);
}

//
// Pop an element from a stack.
// Returns the new top of stack after removing the top element.
//
node_t *pop(node_t *stack)
{
    node_t *a, *s;

    s = stack;
    if (s == NULL)
        writes("pop");
    a = s->p2;
    free_node(s);
    return (a);
}

//
// Parse an expression using operator precedence parsing (Shunting Yard algorithm).
// Handles infix operators, function calls, and parentheses.
// Returns the compiled expression tree.
//
node_t *expr(node_t *start, int eof, node_t *e)
{
    node_t *stack, *list, *comp;
    int operand, op, op1;
    node_t *space_ptr;
    int space_flag;
    node_t *a, *b, *c;
    int d;

    // Initialize expression parser
    list       = alloc(); // Output list (postfix expression)
    e->p2      = list;
    stack      = push(NULL); // Operator stack
    stack->typ = eof;        // End-of-expression marker (lowest precedence)
    operand    = 0;          // Flag: expecting operand (1) or operator (0)
    space_ptr  = start;      // Deferred component (for concatenation)
    space_flag = 0;          // Flag: space seen (implies concatenation)
l1:
    if (space_ptr) {
        comp      = space_ptr;
        space_ptr = NULL;
    } else
        comp = compon();

l3:
    op = comp->typ;
    switch (op) {
    case 7:             // Whitespace - used for concatenation
        space_flag = 1; /* Mark that we had a space */
        free_node(comp);
        comp = compon();
        goto l3;

    case 10: // Multiplication or something else
        if (space_flag == 0) {
            comp->typ = 1; // Treat as multiplication else if no space
            goto l3;
        }

    case 11: // Division or pattern alternation
        if (space_flag == 0) {
            comp->typ = 2; // Treat as division if no space
            goto l3;
        }

    case 8: // Addition
    case 9: // Subtraction
        if (operand == 0)
            writes("no operand preceding operator");
        operand = 0;
        goto l5;

    case 14: // Variable
    case 15: // String literal
        if (operand == 0) {
            operand = 1;
            goto l5;
        }
        if (space_flag == 0)
            goto l7;
        goto l4; // Space means concatenation

    case 12: // Pattern immediate value ($)
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
    l7:
        writes("illegal juxtaposition of operands");

    case 16: // Left parenthesis - function call or grouping
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
        b  = compon();
        op = comp->typ = 13; // Function call
        if (b->typ == 5) {
            // Empty argument list
            comp->p1 = NULL;
            goto l10;
        }
        comp->p1 = a = alloc();
        b            = expr(b, 6, a); // Parse first argument
        while ((d = b->typ) == 4) {   // Comma - more arguments
            a->p1 = b;
            a     = b;
            b     = expr(NULL, 6, a);
        }
        if (d != 5) // Should end with right parenthesis
            writes("error in function");
        a->p1 = NULL;
    l10:
        free_node(b);
        goto l6;

    l4: // Implicit concatenation (space between operands)
        space_ptr  = comp;
        op         = 7;
        operand    = 0;
        space_flag = 0;
        goto l6;
    }
    if (operand == 0)
        writes("no operand at end of expression");
l5:
    space_flag = 0;
l6:
    // Operator precedence handling
    op1 = stack->typ;
    if (op > op1) {
        // Push operator onto stack
        stack = push(stack);
        if (op == 16)
            op = 6; // Treat left paren as low precedence
        stack->typ = op;
        stack->p1  = comp;
        goto l1;
    }
    // Pop and process operators
    c     = stack->p1;
    stack = pop(stack);
    if (stack == NULL) {
        list->typ = 0;
        return (comp);
    }
    if (op1 == 6) {  // Left parenthesis marker
        if (op != 5) // Should match right parenthesis
            writes("too many ('s");
        goto l1;
    }
    if (op1 == 7) // Concatenation operator
        c = alloc();
    list->typ = op1;
    list->p2  = c->p1;
    list->p1  = c;
    list      = c;
    goto l6;
}

//
// Parse a pattern (match statement pattern).
// Handles pattern components, alternation, and grouping.
// Returns the compiled pattern structure.
//
node_t *match(node_t *start, node_t *m)
{
    node_t *list, *comp, *term;
    node_t *a;
    int b, bal;

    term  = NULL;
    bal   = 0;
    list  = alloc();
    m->p2 = list;
    comp  = start;
    if (!comp)
        comp = compon();
    goto l2;

l3:
    list->p1 = a = alloc();
    list         = a;
l2:
    switch (comp->typ) {
    case 7: // Whitespace - skip
        free_node(comp);
        comp = compon();
        goto l2;

    case 12: // Pattern immediate ($)
    case 14: // Variable
    case 15: // String literal
    case 16: // Left parenthesis
        term      = NULL;
        comp      = expr(comp, 6, list); // Parse as expression
        list->typ = 1;                   // Pattern component
        goto l3;

    case 1: // Multiplication operator - pattern alternation
        free_node(comp);
        comp = compon();
        bal  = 0;
        if (comp->typ == 16) {
            // Balanced pattern (parenthesized)
            bal = 1;
            free_node(comp);
            comp = compon();
        }
        a = alloc();
        b = comp->typ;
        if (b == 2 || b == 5 || b == 10 || b == 1)
            a->p1 = NULL; // No left side
        else {
            comp  = expr(comp, 11, a); // Parse left side
            a->p1 = a->p2;
        }
        if (comp->typ != 2) {
            a->p2 = NULL; // No right side
        } else {
            free_node(comp);
            comp = expr(NULL, 6, a); // Parse right side
        }
        if (bal) {
            if (comp->typ != 5) // Should end with right paren
                goto merr;
            free_node(comp);
            comp = compon();
        }
        b = comp->typ;
        if (b != 1 && b != 10) // Should be alternation or equals
            goto merr;
        list->p2  = a;
        list->typ = 2; // Alternation pattern
        a->typ    = bal;
        free_node(comp);
        comp = compon();
        if (bal)
            term = NULL;
        else
            term = list; // Mark for potential concatenation
        goto l3;
    }
    if (term)
        term->typ = 3; // Mark term as concatenated
    list->typ = 0;
    return (comp);

merr:
    writes("unrecognized component in match");
    return NULL;
}

//
// Compile a single Snobol statement.
// Handles labels, assignments, pattern matching, goto statements, and function definitions.
// Returns a compiled statement node.
//
node_t *compile(void)
{
    node_t *b, *comp;
    node_t *r, *l, *xs, *xf, *g;
    int a;
    node_t *m, *as;
    int t;

    m    = NULL; // Match pattern
    l    = NULL; // Label
    as   = NULL; // Assignment target
    xs   = NULL; // Success goto
    xf   = NULL; // Failure goto
    t    = 0;    // Statement type
    comp = compon();
    a    = comp->typ;
    // Check for optional label
    if (a == 14) {
        l = comp->p1;
        free_node(comp);
        comp = compon();
        a    = comp->typ;
    }
    if (a != 7)
        writes("no space beginning statement");
    free_node(comp);
    // Check for function definition
    if (l == lookdef)
        goto def;
    // Parse expression (subject of statement)
    comp = expr(NULL, 11, r = alloc());
    a    = comp->typ;
    if (a == 0) // End of statement
        goto asmble;
    if (a == 2) // Comma - goto statement
        goto xfer;
    if (a == 3) // Equals - assignment
        goto assig;
    // Pattern matching statement
    m    = alloc();
    comp = match(comp, m);
    a    = comp->typ;
    if (a == 0)
        goto asmble;
    if (a == 2)
        goto xfer;
    if (a == 3)
        goto assig;
    writes("unrecognized component in match");
    return NULL;

assig:
    // Parse assignment value
    free_node(comp);
    comp = expr(NULL, 6, as = alloc());
    a    = comp->typ;
    if (a == 0)
        goto asmble;
    if (a == 2)
        goto xfer;
    writes("unrecognized component in assignment");
    return NULL;

xfer:
    // Parse goto target(s)
    free_node(comp);
    comp = compon();
    a    = comp->typ;
    if (a == 16) // Left paren - both success and failure targets
        goto xboth;
    if (a == 0) { // End of statement - no goto
        if (xs != NULL || xf != NULL)
            goto asmble;
        goto xerr;
    }
    if (a != 14) // Should be a label
        goto xerr;
    b = comp->p1;
    free_node(comp);
    if (b == looks) // "s" - success goto
        goto xsuc;
    if (b == lookf) // "f" - failure goto
        goto xfail;

xerr:
    writes("unrecognized component in goto");
    return NULL;

xboth:
    // Parse both success and failure goto targets: (success, failure)
    free_node(comp);
    xs   = alloc();
    xf   = alloc();
    comp = expr(NULL, 6, xs); // Parse success target
    if (comp->typ != 5)       // Should end with right paren
        goto xerr;
    xf->p2 = xs->p2; // Share expression list
    comp   = compon();
    if (comp->typ != 0)
        goto xerr;
    goto asmble;

xsuc:
    // Parse success goto: s(label)
    if (xs)
        goto xerr;
    comp = compon();
    if (comp->typ != 16)
        goto xerr;
    comp = expr(NULL, 6, xs = alloc());
    if (comp->typ != 5)
        goto xerr;
    goto xfer;

xfail:
    // Parse failure goto: f(label)
    if (xf)
        goto xerr;
    comp = compon();
    if (comp->typ != 16)
        goto xerr;
    comp = expr(NULL, 6, xf = alloc());
    if (comp->typ != 5)
        goto xerr;
    goto xfer;

asmble:
    // Assemble the compiled statement
    if (l) {
        if (l->typ)
            writes("name doubly defined");
        l->p2  = comp;
        l->typ = 2; /* type label;*/
    }
    comp->p2 = r; // Link to expression
    if (m) {
        t++; // Type 1: pattern matching statement
        r->p1 = m;
        r     = m;
    }
    if (as) {
        t     = 2; // Type 2: assignment statement
        r->p1 = as;
        r     = as;
    }
    // Build goto structure: g->p1 = success goto, g->p2 = failure goto
    (g = alloc())->p1 = NULL;
    if (xs) {
        g->p1 = xs->p2; // Success goto target (expression list)
        free_node(xs);
    }
    g->p2 = NULL;
    if (xf) {
        g->p2 = xf->p2; // Failure goto target (expression list)
        free_node(xf);
    }
    r->p1     = g;  // Link goto structure to statement
    comp->typ = t;  // Statement type: 0=simple, 1=match, 2=assign
    comp->ch  = lc; // Store line number
    return (comp);

def:
    // Parse function definition: define name(params) body
    r = nscomp();
    if (r->typ != 14) // Should be function name
        goto derr;
    l = r->p1;
    if (l->typ)
        writes("name doubly defined");
    l->typ = 5; /*type function;*/
    {
        node_t *a_ptr = r;
        l->p2         = a_ptr;
        r             = nscomp();
        l             = r;
        a_ptr->p1     = l;
        if (r->typ == 0) // No parameters
            goto d4;
        if (r->typ != 16) // Should start with left paren
            goto derr;

    d2:
        // Parse parameter list
        r = nscomp();
        if (r->typ != 14) // Should be parameter name
            goto derr;
        a_ptr->p2 = r;
        r->typ    = 0;
        a_ptr     = r;
        r         = nscomp();
        if (r->typ == 4) { // Comma - more parameters
            free_node(r);
            goto d2;
        }
        if (r->typ != 5) // Should end with right paren
            goto derr;
        free_node(r);
        if ((r = compon())->typ != 0) // Should be end of statement
            goto derr;
        free_node(r);

    d4:
        // Compile function body
        r         = compile();
        a_ptr->p2 = NULL;
    }
    l->p1 = r; // Link function body
    l->p2 = NULL;
    return (r);

derr:
    writes("illegal component in define");
    return NULL;
}
