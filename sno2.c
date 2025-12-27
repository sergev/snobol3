#include "sno.h"

//
// Parse the next component (token) from the input stream.
// Returns a node representing the token with appropriate type code.
// Handles operators, literals, identifiers, and special characters.
//
node_t *compon(snobol_context_t *ctx)
{
    node_t *a, *b;
    int c;
    static int next;

    if (next == 0)
        ctx->schar = getc_char(ctx);
    else
        next = 0;
    if (ctx->schar == NULL) {
        (a = alloc(ctx))->typ = TOKEN_END;
        return (a);
    }
    switch (char_class(ctx->schar->ch)) {
    case CHAR_CLASS_RPAREN: // Right parenthesis
        ctx->schar->typ = TOKEN_RPAREN;
        return (ctx->schar);

    case CHAR_CLASS_LPAREN: // Left parenthesis
        ctx->schar->typ = TOKEN_LPAREN;
        return (ctx->schar);

    case CHAR_CLASS_WHITESPACE: // Whitespace
        a = ctx->schar;
        for (;;) {
            ctx->schar = getc_char(ctx);
            if (ctx->schar == NULL) {
                a->typ = TOKEN_END;
                return (a);
            }
            if (char_class(ctx->schar->ch) != CHAR_CLASS_WHITESPACE)
                break;
            free_node(ctx, ctx->schar);
        }
        next   = 1;
        a->typ = TOKEN_WHITESPACE;
        return (a);

    case CHAR_CLASS_PLUS: // Plus operator
        ctx->schar->typ = TOKEN_PLUS;
        return (ctx->schar);

    case CHAR_CLASS_MINUS: // Minus operator
        ctx->schar->typ = TOKEN_MINUS;
        return (ctx->schar);

    case CHAR_CLASS_ASTERISK: // Asterisk - could be multiplication or unanchored search
        a          = ctx->schar;
        ctx->schar = getc_char(ctx);
        if (char_class(ctx->schar->ch) == CHAR_CLASS_WHITESPACE)
            a->typ = TOKEN_MULT; // Multiplication (followed by space)
        else
            a->typ = TOKEN_UNANCHORED; // Unanchored search
        next = 1;
        return (a);

    case CHAR_CLASS_SLASH: // Division - could be pattern alternation
        a          = ctx->schar;
        ctx->schar = getc_char(ctx);
        if (char_class(ctx->schar->ch) == CHAR_CLASS_WHITESPACE)
            a->typ = TOKEN_DIV; // Division (followed by space)
        else
            a->typ = TOKEN_ALTERNATION; // Pattern alternation
        next = 1;
        return (a);

    case CHAR_CLASS_DOLLAR: // Dollar sign (pattern immediate value)
        ctx->schar->typ = TOKEN_DOLLAR;
        return (ctx->schar);

    case CHAR_CLASS_STRING_DELIM: // String literal delimiter
        c = ctx->schar->ch;
        a = getc_char(ctx);
        if (a == NULL)
            goto lerr;
        b = ctx->schar;
        if (a->ch == c) {
            // Empty string
            free_node(ctx, ctx->schar);
            a->typ = TOKEN_STRING;
            a->p1  = NULL;
            return (a);
        }
        b->p1 = a;
        for (;;) {
            ctx->schar = getc_char(ctx);
            if (ctx->schar == NULL) {
                // End of input reached without closing quote
                goto lerr;
            }
            if (ctx->schar->ch == c) {
                // Found closing quote - break out of loop
                break;
            }
            // Add this character to the string
            a->p1 = ctx->schar;
            a     = ctx->schar;
        }
        b->p2           = a;
        ctx->schar->typ = TOKEN_STRING;
        ctx->schar->p1  = b;
        return (ctx->schar);
    lerr:
        writes(ctx, "illegal literal string");
        return NULL; // Never reached, but needed for compilation

    case CHAR_CLASS_EQUALS: // Equals sign
        ctx->schar->typ = TOKEN_EQUALS;
        return (ctx->schar);

    case CHAR_CLASS_COMMA: // Comma
        ctx->schar->typ = TOKEN_COMMA;
        return (ctx->schar);

    default: // CHAR_CLASS_OTHER - fall through to identifier/keyword handling
        break;
    }
    // Identifier or keyword - collect characters until delimiter
    b     = alloc(ctx);
    b->p1 = a  = ctx->schar;
    ctx->schar = getc_char(ctx);
    while (ctx->schar != NULL && char_class(ctx->schar->ch) == CHAR_CLASS_OTHER) {
        a->p1      = ctx->schar;
        a          = ctx->schar;
        ctx->schar = getc_char(ctx);
    }
    b->p2 = a;
    next  = 1;
    a     = look(ctx, b);
    delete_string(ctx, b);
    b      = alloc(ctx);
    b->typ = TOKEN_VARIABLE; // Variable reference
    b->p1  = a;
    return (b);
}

//
// Get the next non-space component (skip whitespace tokens).
//
node_t *nscomp(snobol_context_t *ctx)
{
    node_t *c;

    while ((c = compon(ctx))->typ == TOKEN_WHITESPACE)
        free_node(ctx, c);
    return (c);
}

//
// Push an element onto a stack (implemented as a linked list).
// Returns the new top of stack.
//
node_t *push(snobol_context_t *ctx, node_t *stack)
{
    node_t *a;

    (a = alloc(ctx))->p2 = stack;
    return (a);
}

//
// Pop an element from a stack.
// Returns the new top of stack after removing the top element.
//
node_t *pop(snobol_context_t *ctx, node_t *stack)
{
    node_t *a, *s;

    s = stack;
    if (s == NULL)
        writes(ctx, "pop");
    a = s->p2;
    free_node(ctx, s);
    return (a);
}

//
// Parse an expression using operator precedence parsing (Shunting Yard algorithm).
// Handles infix operators, function calls, and parentheses.
// Returns the compiled expression tree.
//
node_t *expr(snobol_context_t *ctx, node_t *start, int eof, node_t *e)
{
    node_t *stack, *list, *comp;
    int operand, op, op1;
    node_t *space_ptr;
    int space_flag;
    node_t *a, *b, *c;
    int d;

    // Initialize expression parser
    list       = alloc(ctx); // Output list (postfix expression)
    e->p2      = list;
    stack      = push(ctx, NULL);   // Operator stack
    stack->typ = (token_type_t)eof; // End-of-expression marker (lowest precedence)
    operand    = 0;                 // Flag: expecting operand (1) or operator (0)
    space_ptr  = start;             // Deferred component (for concatenation)
    space_flag = 0;                 // Flag: space seen (implies concatenation)
l1:
    if (space_ptr) {
        comp      = space_ptr;
        space_ptr = NULL;
    } else
        comp = compon(ctx);

l3:
    op = comp->typ;
    switch (op) {
    case TOKEN_WHITESPACE: // Whitespace - used for concatenation
        space_flag = 1;    /* Mark that we had a space */
        free_node(ctx, comp);
        comp = compon(ctx);
        goto l3;

    case TOKEN_MULT: // Multiplication or something else
        if (space_flag == 0) {
            comp->typ = TOKEN_UNANCHORED; // Treat as multiplication else if no space
            goto l3;
        }

    case TOKEN_DIV: // Division or pattern alternation
        if (space_flag == 0) {
            comp->typ = TOKEN_ALTERNATION; // Treat as division if no space
            goto l3;
        }

    case TOKEN_PLUS:  // Addition
    case TOKEN_MINUS: // Subtraction
        if (operand == 0)
            writes(ctx, "no operand preceding operator");
        operand = 0;
        goto l5;

    case TOKEN_VARIABLE: // Variable
    case TOKEN_STRING:   // String literal
        if (operand == 0) {
            operand = 1;
            goto l5;
        }
        if (space_flag == 0)
            goto l7;
        goto l4; // Space means concatenation

    case TOKEN_DOLLAR: // Pattern immediate value ($)
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
    l7:
        writes(ctx, "illegal juxtaposition of operands");

    case TOKEN_LPAREN: // Left parenthesis - function call or grouping
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
        b  = compon(ctx);
        op = comp->typ = EXPR_CALL; // Function call
        if (b->typ == TOKEN_RPAREN) {
            // Empty argument list
            comp->p1 = NULL;
            goto l10;
        }
        comp->p1 = a = alloc(ctx);
        b            = expr(ctx, b, 6, a);    // Parse first argument
        while ((d = b->typ) == TOKEN_COMMA) { // Comma - more arguments
            a->p1 = b;
            a     = b;
            b     = expr(ctx, NULL, 6, a);
        }
        if (d != TOKEN_RPAREN) // Should end with right parenthesis
            writes(ctx, "error in function");
        a->p1 = NULL;
    l10:
        free_node(ctx, b);
        goto l6;

    l4: // Implicit concatenation (space between operands)
        space_ptr  = comp;
        op         = TOKEN_WHITESPACE;
        operand    = 0;
        space_flag = 0;
        goto l6;
    }
    if (operand == 0)
        writes(ctx, "no operand at end of expression");
l5:
    space_flag = 0;
l6:
    // Operator precedence handling
    op1 = stack->typ;
    if (op > op1) {
        // Push operator onto stack
        stack = push(ctx, stack);
        if (op == TOKEN_LPAREN)
            op = EXPR_SPECIAL; // Treat left paren as low precedence
        stack->typ = (token_type_t)op;
        stack->p1  = comp;
        goto l1;
    }
    // Pop and process operators
    c     = stack->p1;
    stack = pop(ctx, stack);
    if (stack == NULL) {
        list->typ = TOKEN_END;
        return (comp);
    }
    if (op1 == EXPR_SPECIAL) {  // Left parenthesis marker
        if (op != TOKEN_RPAREN) // Should match right parenthesis
            writes(ctx, "too many ('s");
        goto l1;
    }
    if (op1 == TOKEN_WHITESPACE) // Concatenation operator
        c = alloc(ctx);
    list->typ = (token_type_t)op1;
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
node_t *match(snobol_context_t *ctx, node_t *start, node_t *m)
{
    node_t *list, *comp, *term;
    node_t *a;
    int b, bal;

    term  = NULL;
    bal   = 0;
    list  = alloc(ctx);
    m->p2 = list;
    comp  = start;
    if (!comp)
        comp = compon(ctx);
    goto l2;

l3:
    list->p1 = a = alloc(ctx);
    list         = a;
l2:
    switch (comp->typ) {
    case TOKEN_WHITESPACE: // Whitespace - skip
        free_node(ctx, comp);
        comp = compon(ctx);
        goto l2;

    case TOKEN_DOLLAR:   // Pattern immediate ($)
    case TOKEN_VARIABLE: // Variable
    case TOKEN_STRING:   // String literal
    case TOKEN_LPAREN:   // Left parenthesis
        term      = NULL;
        comp      = expr(ctx, comp, 6, list); // Parse as expression
        list->typ = TOKEN_UNANCHORED;         // Pattern component
        goto l3;

    case TOKEN_UNANCHORED: // Multiplication operator - pattern alternation
        free_node(ctx, comp);
        comp = compon(ctx);
        bal  = 0;
        if (comp->typ == TOKEN_LPAREN) {
            // Balanced pattern (parenthesized)
            bal = 1;
            free_node(ctx, comp);
            comp = compon(ctx);
        }
        a = alloc(ctx);
        b = comp->typ;
        if (b == TOKEN_ALTERNATION || b == TOKEN_RPAREN || b == TOKEN_MULT || b == TOKEN_UNANCHORED)
            a->p1 = NULL; // No left side
        else {
            comp  = expr(ctx, comp, 11, a); // Parse left side
            a->p1 = a->p2;
        }
        if (comp->typ != TOKEN_ALTERNATION) {
            a->p2 = NULL; // No right side
        } else {
            free_node(ctx, comp);
            comp = expr(ctx, NULL, 6, a); // Parse right side
        }
        if (bal) {
            if (comp->typ != TOKEN_RPAREN) // Should end with right paren
                goto merr;
            free_node(ctx, comp);
            comp = compon(ctx);
        }
        b = comp->typ;
        if (b != TOKEN_UNANCHORED && b != TOKEN_MULT) // Should be alternation or equals
            goto merr;
        list->p2  = a;
        list->typ = TOKEN_ALTERNATION; // Alternation pattern
        a->typ    = (token_type_t)bal;
        free_node(ctx, comp);
        comp = compon(ctx);
        if (bal)
            term = NULL;
        else
            term = list; // Mark for potential concatenation
        goto l3;

    case TOKEN_END:
    case TOKEN_ALTERNATION: // Pattern alternation or end of pattern (goto)
    case TOKEN_COMMA:       // Comma (goto)
    case TOKEN_EQUALS:      // Equals (assignment)
    case TOKEN_RPAREN:      // Right paren (used in expressions)
        // End of pattern - fall through to normal return
        break;

    default:
        // Other token types not valid in pattern context
        goto merr;
    }
    if (term)
        term->typ = TOKEN_EQUALS; // Mark term as concatenated
    list->typ = TOKEN_END;
    return (comp);

merr:
    writes(ctx, "unrecognized component in match");
    return NULL;
}

//
// Compile a single Snobol statement.
// Handles labels, assignments, pattern matching, goto statements, and function definitions.
// Returns a compiled statement node.
//
node_t *compile(snobol_context_t *ctx)
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
    comp = compon(ctx);
    a    = comp->typ;
    // Check for optional label
    if (a == TOKEN_VARIABLE) {
        l = comp->p1;
        free_node(ctx, comp);
        comp = compon(ctx);
        a    = comp->typ;
    }
    if (a != TOKEN_WHITESPACE)
        writes(ctx, "no space beginning statement");
    free_node(ctx, comp);
    // Check for function definition
    if (l == ctx->lookdef)
        goto def;
    // Parse expression (subject of statement)
    comp = expr(ctx, NULL, 11, r = alloc(ctx));
    a    = comp->typ;
    if (a == TOKEN_END) // End of statement
        goto asmble;
    if (a == TOKEN_ALTERNATION) // Comma - goto statement
        goto xfer;
    if (a == TOKEN_EQUALS) // Equals - assignment
        goto assig;
    // Pattern matching statement
    m    = alloc(ctx);
    comp = match(ctx, comp, m);
    a    = comp->typ;
    if (a == TOKEN_END)
        goto asmble;
    if (a == TOKEN_ALTERNATION)
        goto xfer;
    if (a == TOKEN_EQUALS)
        goto assig;
    writes(ctx, "unrecognized component in match");
    return NULL;

assig:
    // Parse assignment value
    free_node(ctx, comp);
    comp = expr(ctx, NULL, 6, as = alloc(ctx));
    a    = comp->typ;
    if (a == TOKEN_END)
        goto asmble;
    if (a == TOKEN_ALTERNATION)
        goto xfer;
    writes(ctx, "unrecognized component in assignment");
    return NULL;

xfer:
    // Parse goto target(s)
    free_node(ctx, comp);
    comp = compon(ctx);
    a    = comp->typ;
    if (a == TOKEN_LPAREN) // Left paren - both success and failure targets
        goto xboth;
    if (a == TOKEN_END) { // End of statement - no goto
        if (xs != NULL || xf != NULL)
            goto asmble;
        goto xerr;
    }
    if (a != TOKEN_VARIABLE) // Should be a label
        goto xerr;
    b = comp->p1;
    free_node(ctx, comp);
    if (b == ctx->looks) // "s" - success goto
        goto xsuc;
    if (b == ctx->lookf) // "f" - failure goto
        goto xfail;

xerr:
    writes(ctx, "unrecognized component in goto");
    return NULL;

xboth:
    // Parse both success and failure goto targets: (success, failure)
    free_node(ctx, comp);
    xs   = alloc(ctx);
    xf   = alloc(ctx);
    comp = expr(ctx, NULL, 6, xs); // Parse success target
    if (comp->typ != TOKEN_RPAREN) // Should end with right paren
        goto xerr;
    xf->p2 = xs->p2; // Share expression list
    comp   = compon(ctx);
    if (comp->typ != TOKEN_END)
        goto xerr;
    goto asmble;

xsuc:
    // Parse success goto: s(label)
    if (xs)
        goto xerr;
    comp = compon(ctx);
    if (comp->typ != TOKEN_LPAREN)
        goto xerr;
    comp = expr(ctx, NULL, 6, xs = alloc(ctx));
    if (comp->typ != TOKEN_RPAREN)
        goto xerr;
    goto xfer;

xfail:
    // Parse failure goto: f(label)
    if (xf)
        goto xerr;
    comp = compon(ctx);
    if (comp->typ != TOKEN_LPAREN)
        goto xerr;
    comp = expr(ctx, NULL, 6, xf = alloc(ctx));
    if (comp->typ != TOKEN_RPAREN)
        goto xerr;
    goto xfer;

asmble:
    // Assemble the compiled statement
    if (l) {
        if (l->typ)
            writes(ctx, "name doubly defined");
        l->p2  = comp;
        l->typ = EXPR_LABEL; /* type label;*/
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
    (g = alloc(ctx))->p1 = NULL;
    if (xs) {
        g->p1 = xs->p2; // Success goto target (expression list)
        free_node(ctx, xs);
    }
    g->p2 = NULL;
    if (xf) {
        g->p2 = xf->p2; // Failure goto target (expression list)
        free_node(ctx, xf);
    }
    r->p1     = g;               // Link goto structure to statement
    comp->typ = (token_type_t)t; // Statement type: 0=simple, 1=match, 2=assign
    comp->ch  = ctx->lc;         // Store line number
    return (comp);

def:
    // Parse function definition: define name(params) body
    r = nscomp(ctx);
    if (r->typ != TOKEN_VARIABLE) // Should be function name
        goto derr;
    l = r->p1;
    if (l->typ)
        writes(ctx, "name doubly defined");
    l->typ = EXPR_FUNCTION; /*type function;*/
    {
        node_t *a_ptr = r;
        l->p2         = a_ptr;
        r             = nscomp(ctx);
        l             = r;
        a_ptr->p1     = l;
        if (r->typ == TOKEN_END) // No parameters
            goto d4;
        if (r->typ != TOKEN_LPAREN) // Should start with left paren
            goto derr;

    d2:
        // Parse parameter list
        r = nscomp(ctx);
        if (r->typ != TOKEN_VARIABLE) // Should be parameter name
            goto derr;
        a_ptr->p2 = r;
        r->typ    = EXPR_VAR_REF;
        a_ptr     = r;
        r         = nscomp(ctx);
        if (r->typ == TOKEN_COMMA) { // Comma - more parameters
            free_node(ctx, r);
            goto d2;
        }
        if (r->typ != TOKEN_RPAREN) // Should end with right paren
            goto derr;
        free_node(ctx, r);
        if ((r = compon(ctx))->typ != TOKEN_END) // Should be end of statement
            goto derr;
        free_node(ctx, r);

    d4:
        // Compile function body
        r         = compile(ctx);
        a_ptr->p2 = NULL;
    }
    l->p1 = r; // Link function body
    l->p2 = NULL;
    return (r);

derr:
    writes(ctx, "illegal component in define");
    return NULL;
}
