#include "sno.h"

//
// Parse the next component (token) from the input stream.
// Returns a node representing the token with appropriate type code.
// Handles operators, literals, identifiers, and special characters.
//
node_t *SnobolContext::compon()
{
    node_t *a, *b;
    int c;

    if (compon_next == 0)
        schar = getc_char();
    else
        compon_next = 0;
    if (schar == nullptr) {
        (a = alloc())->typ = TOKEN_END;
        return (a);
    }
    switch (char_class(schar->ch)) {
    case CHAR_CLASS_RPAREN: // Right parenthesis
        schar->typ = TOKEN_RPAREN;
        return (schar);

    case CHAR_CLASS_LPAREN: // Left parenthesis
        schar->typ = TOKEN_LPAREN;
        return (schar);

    case CHAR_CLASS_WHITESPACE: // Whitespace
        a = schar;
        for (;;) {
            schar = getc_char();
            if (schar == nullptr) {
                a->typ = TOKEN_END;
                return (a);
            }
            if (char_class(schar->ch) != CHAR_CLASS_WHITESPACE)
                break;
            free_node(schar);
        }
        compon_next = 1;
        a->typ      = TOKEN_WHITESPACE;
        return (a);

    case CHAR_CLASS_PLUS: // Plus operator
        schar->typ = TOKEN_PLUS;
        return (schar);

    case CHAR_CLASS_MINUS: // Minus operator
        schar->typ = TOKEN_MINUS;
        return (schar);

    case CHAR_CLASS_ASTERISK: // Asterisk - could be multiplication or unanchored search
        a     = schar;
        schar = getc_char();
        if (char_class(schar->ch) == CHAR_CLASS_WHITESPACE)
            a->typ = TOKEN_MULT; // Multiplication (followed by space)
        else
            a->typ = TOKEN_UNANCHORED; // Unanchored search
        compon_next = 1;
        return (a);

    case CHAR_CLASS_SLASH: // Division - could be pattern alternation
        a     = schar;
        schar = getc_char();
        if (char_class(schar->ch) == CHAR_CLASS_WHITESPACE)
            a->typ = TOKEN_DIV; // Division (followed by space)
        else
            a->typ = TOKEN_ALTERNATION; // Pattern alternation
        compon_next = 1;
        return (a);

    case CHAR_CLASS_DOLLAR: // Dollar sign (pattern immediate value)
        schar->typ = TOKEN_DOLLAR;
        return (schar);

    case CHAR_CLASS_STRING_DELIM: // String literal delimiter
        c = schar->ch;
        a = getc_char();
        if (a == nullptr)
            goto lerr;
        b = schar;
        if (a->ch == c) {
            // Empty string
            free_node(schar);
            a->typ = TOKEN_STRING;
            a->p1  = nullptr;
            return (a);
        }
        b->p1 = a;
        for (;;) {
            schar = getc_char();
            if (schar == nullptr) {
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
        schar->typ = TOKEN_STRING;
        schar->p1  = b;
        return (schar);
    lerr:
        writes("illegal literal string");
        return nullptr; // Never reached, but needed for compilation

    case CHAR_CLASS_EQUALS: // Equals sign
        schar->typ = TOKEN_EQUALS;
        return (schar);

    case CHAR_CLASS_COMMA: // Comma
        schar->typ = TOKEN_COMMA;
        return (schar);

    default: // CHAR_CLASS_OTHER - fall through to identifier/keyword handling
        break;
    }
    // Identifier or keyword - collect characters until delimiter
    b     = alloc();
    b->p1 = a = schar;
    schar     = getc_char();
    while (schar != nullptr && char_class(schar->ch) == CHAR_CLASS_OTHER) {
        a->p1 = schar;
        a     = schar;
        schar = getc_char();
    }
    b->p2       = a;
    compon_next = 1;
    a           = look(b);
    delete_string(b);
    b      = alloc();
    b->typ = TOKEN_VARIABLE; // Variable reference
    b->p1  = a;
    return (b);
}

//
// Get the next non-space component (skip whitespace tokens).
//
node_t *SnobolContext::nscomp()
{
    node_t *c;

    while ((c = compon())->typ == TOKEN_WHITESPACE)
        free_node(c);
    return (c);
}

//
// Push an element onto a stack (implemented as a linked list).
// Returns the new top of stack.
//
node_t *SnobolContext::push(node_t *stack)
{
    node_t *a;

    (a = alloc())->p2 = stack;
    return (a);
}

//
// Pop an element from a stack.
// Returns the new top of stack after removing the top element.
//
node_t *SnobolContext::pop(node_t *stack)
{
    node_t *a, *s;

    s = stack;
    if (s == nullptr)
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
node_t *SnobolContext::expr(node_t *start, int eof, node_t *e)
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
    stack      = push(nullptr);     // Operator stack
    stack->typ = (token_type_t)eof; // End-of-expression marker (lowest precedence)
    operand    = 0;                 // Flag: expecting operand (1) or operator (0)
    space_ptr  = start;             // Deferred component (for concatenation)
    space_flag = 0;                 // Flag: space seen (implies concatenation)
l1:
    if (space_ptr) {
        comp      = space_ptr;
        space_ptr = nullptr;
    } else
        comp = compon();

l3:
    op = comp->typ;
    switch (op) {
    case TOKEN_WHITESPACE: // Whitespace - used for concatenation
        space_flag = 1;    /* Mark that we had a space */
        free_node(comp);
        comp = compon();
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
            writes("no operand preceding operator");
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
        writes("illegal juxtaposition of operands");

    case TOKEN_LPAREN: // Left parenthesis - function call or grouping
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
        b  = compon();
        op = comp->typ = EXPR_CALL; // Function call
        if (b->typ == TOKEN_RPAREN) {
            // Empty argument list
            comp->p1 = nullptr;
            goto l10;
        }
        comp->p1 = a = alloc();
        b            = expr(b, 6, a);         // Parse first argument
        while ((d = b->typ) == TOKEN_COMMA) { // Comma - more arguments
            a->p1 = b;
            a     = b;
            b     = expr(nullptr, 6, a);
        }
        if (d != TOKEN_RPAREN) // Should end with right parenthesis
            writes("error in function");
        a->p1 = nullptr;
    l10:
        free_node(b);
        goto l6;

    l4: // Implicit concatenation (space between operands)
        space_ptr  = comp;
        op         = TOKEN_WHITESPACE;
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
        if (op == TOKEN_LPAREN)
            op = EXPR_SPECIAL; // Treat left paren as low precedence
        stack->typ = (token_type_t)op;
        stack->p1  = comp;
        goto l1;
    }
    // Pop and process operators
    c     = stack->p1;
    stack = pop(stack);
    if (stack == nullptr) {
        list->typ = TOKEN_END;
        return (comp);
    }
    if (op1 == EXPR_SPECIAL) {  // Left parenthesis marker
        if (op != TOKEN_RPAREN) // Should match right parenthesis
            writes("too many ('s");
        goto l1;
    }
    if (op1 == TOKEN_WHITESPACE) // Concatenation operator
        c = alloc();
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
node_t *SnobolContext::match(node_t *start, node_t *m)
{
    node_t *list, *comp, *term;
    node_t *a;
    int b, bal;

    term  = nullptr;
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
    case TOKEN_WHITESPACE: // Whitespace - skip
        free_node(comp);
        comp = compon();
        goto l2;

    case TOKEN_DOLLAR:   // Pattern immediate ($)
    case TOKEN_VARIABLE: // Variable
    case TOKEN_STRING:   // String literal
    case TOKEN_LPAREN:   // Left parenthesis
        term      = nullptr;
        comp      = expr(comp, 6, list); // Parse as expression
        list->typ = TOKEN_UNANCHORED;    // Pattern component
        goto l3;

    case TOKEN_UNANCHORED: // Multiplication operator - pattern alternation
        free_node(comp);
        comp = compon();
        bal  = 0;
        if (comp->typ == TOKEN_LPAREN) {
            // Balanced pattern (parenthesized)
            bal = 1;
            free_node(comp);
            comp = compon();
        }
        a = alloc();
        b = comp->typ;
        if (b == TOKEN_ALTERNATION || b == TOKEN_RPAREN || b == TOKEN_MULT || b == TOKEN_UNANCHORED)
            a->p1 = nullptr; // No left side
        else {
            comp  = expr(comp, 11, a); // Parse left side
            a->p1 = a->p2;
        }
        if (comp->typ != TOKEN_ALTERNATION) {
            a->p2 = nullptr; // No right side
        } else {
            free_node(comp);
            comp = expr(nullptr, 6, a); // Parse right side
        }
        if (bal) {
            if (comp->typ != TOKEN_RPAREN) // Should end with right paren
                goto merr;
            free_node(comp);
            comp = compon();
        }
        b = comp->typ;
        if (b != TOKEN_UNANCHORED && b != TOKEN_MULT) // Should be alternation or equals
            goto merr;
        list->p2  = a;
        list->typ = TOKEN_ALTERNATION; // Alternation pattern
        a->typ    = (token_type_t)bal;
        free_node(comp);
        comp = compon();
        if (bal)
            term = nullptr;
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
    writes("unrecognized component in match");
    return nullptr;
}

//
// Compile a single Snobol statement.
// Handles labels, assignments, pattern matching, goto statements, and function definitions.
// Returns a compiled statement node.
//
node_t *SnobolContext::compile()
{
    node_t *b, *comp;
    node_t *r, *l, *xs, *xf, *g;
    int a;
    node_t *m, *as;
    int t;

    m    = nullptr; // Match pattern
    l    = nullptr; // Label
    as   = nullptr; // Assignment target
    xs   = nullptr; // Success goto
    xf   = nullptr; // Failure goto
    t    = 0;       // Statement type
    comp = compon();
    a    = comp->typ;
    // Check for optional label
    if (a == TOKEN_VARIABLE) {
        l = comp->p1;
        free_node(comp);
        comp = compon();
        a    = comp->typ;
    }
    if (a != TOKEN_WHITESPACE)
        writes("no space beginning statement");
    free_node(comp);
    // Check for function definition
    if (l == lookdef)
        goto def;
    // Parse expression (subject of statement)
    comp = expr(nullptr, 11, r = alloc());
    a    = comp->typ;
    if (a == TOKEN_END) // End of statement
        goto asmble;
    if (a == TOKEN_ALTERNATION) // Comma - goto statement
        goto xfer;
    if (a == TOKEN_EQUALS) // Equals - assignment
        goto assig;
    // Pattern matching statement
    m    = alloc();
    comp = match(comp, m);
    a    = comp->typ;
    if (a == TOKEN_END)
        goto asmble;
    if (a == TOKEN_ALTERNATION)
        goto xfer;
    if (a == TOKEN_EQUALS)
        goto assig;
    writes("unrecognized component in match");
    return nullptr;

assig:
    // Parse assignment value
    free_node(comp);
    comp = expr(nullptr, 6, as = alloc());
    a    = comp->typ;
    if (a == TOKEN_END)
        goto asmble;
    if (a == TOKEN_ALTERNATION)
        goto xfer;
    writes("unrecognized component in assignment");
    return nullptr;

xfer:
    // Parse goto target(s)
    free_node(comp);
    comp = compon();
    a    = comp->typ;
    if (a == TOKEN_LPAREN) // Left paren - both success and failure targets
        goto xboth;
    if (a == TOKEN_END) { // End of statement - no goto
        if (xs != nullptr || xf != nullptr)
            goto asmble;
        goto xerr;
    }
    if (a != TOKEN_VARIABLE) // Should be a label
        goto xerr;
    b = comp->p1;
    free_node(comp);
    if (b == looks) // "s" - success goto
        goto xsuc;
    if (b == lookf) // "f" - failure goto
        goto xfail;

xerr:
    writes("unrecognized component in goto");
    return nullptr;

xboth:
    // Parse both success and failure goto targets: (success, failure)
    free_node(comp);
    xs   = alloc();
    xf   = alloc();
    comp = expr(nullptr, 6, xs);   // Parse success target
    if (comp->typ != TOKEN_RPAREN) // Should end with right paren
        goto xerr;
    xf->p2 = xs->p2; // Share expression list
    comp   = compon();
    if (comp->typ != TOKEN_END)
        goto xerr;
    goto asmble;

xsuc:
    // Parse success goto: s(label)
    if (xs)
        goto xerr;
    comp = compon();
    if (comp->typ != TOKEN_LPAREN)
        goto xerr;
    comp = expr(nullptr, 6, xs = alloc());
    if (comp->typ != TOKEN_RPAREN)
        goto xerr;
    goto xfer;

xfail:
    // Parse failure goto: f(label)
    if (xf)
        goto xerr;
    comp = compon();
    if (comp->typ != TOKEN_LPAREN)
        goto xerr;
    comp = expr(nullptr, 6, xf = alloc());
    if (comp->typ != TOKEN_RPAREN)
        goto xerr;
    goto xfer;

asmble:
    // Assemble the compiled statement
    if (l) {
        if (l->typ)
            writes("name doubly defined");
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
    (g = alloc())->p1 = nullptr;
    if (xs) {
        g->p1 = xs->p2; // Success goto target (expression list)
        free_node(xs);
    }
    g->p2 = nullptr;
    if (xf) {
        g->p2 = xf->p2; // Failure goto target (expression list)
        free_node(xf);
    }
    r->p1     = g;               // Link goto structure to statement
    comp->typ = (token_type_t)t; // Statement type: 0=simple, 1=match, 2=assign
    comp->ch  = lc;              // Store line number
    return (comp);

def:
    // Parse function definition: define name(params) body
    r = nscomp();
    if (r->typ != TOKEN_VARIABLE) // Should be function name
        goto derr;
    l = r->p1;
    if (l->typ)
        writes("name doubly defined");
    l->typ = EXPR_FUNCTION; /*type function;*/
    {
        node_t *a_ptr = r;
        l->p2         = a_ptr;
        r             = nscomp();
        l             = r;
        a_ptr->p1     = l;
        if (r->typ == TOKEN_END) // No parameters
            goto d4;
        if (r->typ != TOKEN_LPAREN) // Should start with left paren
            goto derr;

    d2:
        // Parse parameter list
        r = nscomp();
        if (r->typ != TOKEN_VARIABLE) // Should be parameter name
            goto derr;
        a_ptr->p2 = r;
        r->typ    = EXPR_VAR_REF;
        a_ptr     = r;
        r         = nscomp();
        if (r->typ == TOKEN_COMMA) { // Comma - more parameters
            free_node(r);
            goto d2;
        }
        if (r->typ != TOKEN_RPAREN) // Should end with right paren
            goto derr;
        free_node(r);
        if ((r = compon())->typ != TOKEN_END) // Should be end of statement
            goto derr;
        free_node(r);

    d4:
        // Compile function body
        r         = compile();
        a_ptr->p2 = nullptr;
    }
    l->p1 = r; // Link function body
    l->p2 = nullptr;
    return (r);

derr:
    writes("illegal component in define");
    return nullptr;
}
