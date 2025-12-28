#include "sno.h"

//
// Parse the next component (token) from the input stream.
// Returns a reference to a node representing the token with appropriate type code.
// Handles operators, literals, identifiers, and special characters.
//
Node &SnobolContext::compon()
{
    Node *a, *b;
    int c;

    if (compon_next == 0)
        schar = getc_char();
    else
        compon_next = 0;
    if (schar == nullptr) {
        a      = &alloc();
        a->typ = Token::TOKEN_END;
        return *a;
    }
    switch (char_class(schar->ch)) {
    case CharClass::RPAREN: // Right parenthesis
        schar->typ = Token::TOKEN_RPAREN;
        return *schar;

    case CharClass::LPAREN: // Left parenthesis
        schar->typ = Token::TOKEN_LPAREN;
        return *schar;

    case CharClass::WHITESPACE: // Whitespace
        a = schar;
        for (;;) {
            schar = getc_char();
            if (schar == nullptr) {
                a->typ = Token::TOKEN_END;
                return *a;
            }
            if (char_class(schar->ch) != CharClass::WHITESPACE)
                break;
            free_node(*schar);
        }
        compon_next = 1;
        a->typ      = Token::TOKEN_WHITESPACE;
        return *a;

    case CharClass::PLUS: // Plus operator
        schar->typ = Token::TOKEN_PLUS;
        return *schar;

    case CharClass::MINUS: // Minus operator
        schar->typ = Token::TOKEN_MINUS;
        return *schar;

    case CharClass::ASTERISK: // Asterisk - could be multiplication or unanchored search
        a     = schar;
        schar = getc_char();
        if (char_class(schar->ch) == CharClass::WHITESPACE)
            a->typ = Token::TOKEN_MULT; // Multiplication (followed by space)
        else
            a->typ = Token::TOKEN_UNANCHORED; // Unanchored search
        compon_next = 1;
        return *a;

    case CharClass::SLASH: // Division - could be pattern alternation
        a     = schar;
        schar = getc_char();
        if (char_class(schar->ch) == CharClass::WHITESPACE)
            a->typ = Token::TOKEN_DIV; // Division (followed by space)
        else
            a->typ = Token::TOKEN_ALTERNATION; // Pattern alternation
        compon_next = 1;
        return *a;

    case CharClass::DOLLAR: // Dollar sign (pattern immediate value)
        schar->typ = Token::TOKEN_DOLLAR;
        return *schar;

    case CharClass::STRING_DELIM: // String literal delimiter
        c = schar->ch;
        a = getc_char();
        if (a == nullptr)
            goto lerr;
        b = schar;
        if (a->ch == c) {
            // Empty string
            free_node(*schar);
            a->typ = Token::TOKEN_STRING;
            a->p1  = nullptr;
            return *a;
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
        schar->typ = Token::TOKEN_STRING;
        schar->p1  = b;
        return *schar;
    lerr:
        writes("illegal literal string");
        // Never reached, but needed for compilation
        a = &alloc();
        return *a;

    case CharClass::EQUALS: // Equals sign
        schar->typ = Token::TOKEN_EQUALS;
        return *schar;

    case CharClass::COMMA: // Comma
        schar->typ = Token::TOKEN_COMMA;
        return *schar;

    default: // CharClass::OTHER - fall through to identifier/keyword handling
        break;
    }
    // Identifier or keyword - collect characters until delimiter
    b     = &alloc();
    b->p1 = a = schar;
    schar     = getc_char();
    while (schar != nullptr && char_class(schar->ch) == CharClass::OTHER) {
        a->p1 = schar;
        a     = schar;
        schar = getc_char();
    }
    b->p2       = a;
    compon_next = 1;
    a           = &look(*b);
    delete_string(b);
    b      = &alloc();
    b->typ = Token::TOKEN_VARIABLE; // Variable reference
    b->p1  = a;
    return *b;
}

//
// Get the next non-space component (skip whitespace tokens).
//
Node &SnobolContext::nscomp()
{
    Node *c;

    c = &compon();
    while (c->typ == Token::TOKEN_WHITESPACE) {
        free_node(*c);
        c = &compon();
    }
    return *c;
}

//
// Push an element onto a stack (implemented as a linked list).
// Returns a reference to the new top of stack.
//
Node &SnobolContext::push(Node *stack)
{
    Node &a = alloc();
    a.p2    = stack;
    return a;
}

//
// Pop an element from a stack.
// Returns the new top of stack after removing the top element.
//
Node *SnobolContext::pop(Node *stack)
{
    Node *a, *s;

    s = stack;
    if (s == nullptr)
        writes("pop");
    a = s->p2;
    free_node(*s);
    return (a);
}

//
// Parse an expression using operator precedence parsing (Shunting Yard algorithm).
// Handles infix operators, function calls, and parentheses.
// Returns a reference to the compiled expression tree.
//
Node &SnobolContext::expr(Node *start, Token eof, Node &e)
{
    Node *stack, *list, *comp;
    int operand;
    Token op, op1;
    Node *space_ptr;
    int space_flag;
    Node *a, *b, *c;
    Token d_token;

    // Initialize expression parser
    list       = &alloc(); // Output list (postfix expression)
    e.p2       = list;
    stack      = &push(nullptr); // Operator stack
    stack->typ = eof;            // End-of-expression marker (lowest precedence)
    operand    = 0;              // Flag: expecting operand (1) or operator (0)
    space_ptr  = start;          // Deferred component (for concatenation)
    space_flag = 0;              // Flag: space seen (implies concatenation)
l1:
    if (space_ptr) {
        comp      = space_ptr;
        space_ptr = nullptr;
    } else
        comp = &compon();

l3:
    op = comp->typ;
    switch (op) {
    case Token::TOKEN_WHITESPACE: // Whitespace - used for concatenation
        space_flag = 1;           // Mark that we had a space
        free_node(*comp);
        comp = &compon();
        goto l3;

    case Token::TOKEN_MULT: // Multiplication or something else
        if (space_flag == 0) {
            comp->typ = Token::TOKEN_UNANCHORED; // Treat as multiplication else if no space
            goto l3;
        }
        [[fallthrough]];

    case Token::TOKEN_DIV: // Division or pattern alternation
        if (space_flag == 0) {
            comp->typ = Token::TOKEN_ALTERNATION; // Treat as division if no space
            goto l3;
        }
        [[fallthrough]];

    case Token::TOKEN_PLUS:  // Addition
    case Token::TOKEN_MINUS: // Subtraction
        if (operand == 0)
            writes("no operand preceding operator");
        operand = 0;
        goto l5;

    case Token::TOKEN_VARIABLE: // Variable
    case Token::TOKEN_STRING:   // String literal
        if (operand == 0) {
            operand = 1;
            goto l5;
        }
        if (space_flag == 0)
            goto l7;
        goto l4; // Space means concatenation

    case Token::TOKEN_DOLLAR: // Pattern immediate value ($)
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
    l7:
        writes("illegal juxtaposition of operands");
        break;

    case Token::TOKEN_LPAREN: // Left parenthesis - function call or grouping
        if (operand == 0)
            goto l5;
        if (space_flag)
            goto l4;
        b  = &compon();
        op = comp->typ = Token::EXPR_CALL; // Function call
        if (b->typ == Token::TOKEN_RPAREN) {
            // Empty argument list
            comp->p1 = nullptr;
            goto l10;
        }
        comp->p1 = a = &alloc();
        b            = &expr(b, Token::EXPR_SPECIAL, *a);  // Parse first argument
        while ((d_token = b->typ) == Token::TOKEN_COMMA) { // Comma - more arguments
            a->p1 = b;
            a     = b;
            b     = &expr(nullptr, Token::EXPR_SPECIAL, *a);
        }
        if (d_token != Token::TOKEN_RPAREN) // Should end with right parenthesis
            writes("error in function");
        a->p1 = nullptr;
    l10:
        free_node(*b);
        goto l6;

    l4: // Implicit concatenation (space between operands)
        space_ptr  = comp;
        op         = Token::TOKEN_WHITESPACE;
        operand    = 0;
        space_flag = 0;
        goto l6;
    default:
        break;
    }
    if (operand == 0)
        writes("no operand at end of expression");
l5:
    space_flag = 0;
l6:
    // Operator precedence handling
    op1 = stack->typ;
    if (static_cast<int>(op) > static_cast<int>(op1)) {
        // Push operator onto stack
        stack = &push(stack);
        if (op == Token::TOKEN_LPAREN)
            op = Token::EXPR_SPECIAL; // Treat left paren as low precedence
        stack->typ = op;
        stack->p1  = comp;
        goto l1;
    }
    // Pop and process operators
    c     = stack->p1;
    stack = pop(stack);
    if (stack == nullptr) {
        list->typ = Token::TOKEN_END;
        return *comp;
    }
    if (op1 == Token::EXPR_SPECIAL) {  // Left parenthesis marker
        if (op != Token::TOKEN_RPAREN) // Should match right parenthesis
            writes("too many ('s");
        goto l1;
    }
    if (op1 == Token::TOKEN_WHITESPACE) // Concatenation operator
        c = &alloc();
    list->typ = op1;
    list->p2  = c->p1;
    list->p1  = c;
    list      = c;
    goto l6;
}

//
// Parse a pattern (match statement pattern).
// Handles pattern components, alternation, and grouping.
// Returns a reference to the compiled pattern structure.
//
Node &SnobolContext::match(Node *start, Node &m)
{
    Node *list, *comp;
    Node *a;
    Token b_token;
    Token bal;

    bal  = Token::STMT_SIMPLE;
    list = &alloc();
    m.p2 = list;
    comp = start;
    if (!comp)
        comp = &compon();
    goto l2;

l3:
    list->p1 = a = &alloc();
    list         = a;
l2:
    switch (comp->typ) {
    case Token::TOKEN_WHITESPACE: // Whitespace - skip
        free_node(*comp);
        comp = &compon();
        goto l2;

    case Token::TOKEN_DOLLAR:   // Pattern immediate ($)
    case Token::TOKEN_VARIABLE: // Variable
    case Token::TOKEN_STRING:   // String literal
    case Token::TOKEN_LPAREN:   // Left parenthesis
        comp      = &expr(comp, Token::EXPR_SPECIAL, *list); // Parse as expression
        list->typ = Token::TOKEN_UNANCHORED;                 // Pattern component
        goto l3;

    case Token::TOKEN_UNANCHORED: // Multiplication operator - pattern alternation
        free_node(*comp);
        comp = &compon();
        bal  = Token::STMT_SIMPLE;
        if (comp->typ == Token::TOKEN_LPAREN) {
            // Balanced pattern (parenthesized)
            bal = Token::STMT_MATCH;
            free_node(*comp);
            comp = &compon();
        }
        a       = &alloc();
        b_token = comp->typ;
        if (b_token == Token::TOKEN_ALTERNATION || b_token == Token::TOKEN_RPAREN ||
            b_token == Token::TOKEN_MULT || b_token == Token::TOKEN_UNANCHORED)
            a->p1 = nullptr; // No left side
        else {
            comp  = &expr(comp, Token::TOKEN_DIV, *a); // Parse left side
            a->p1 = a->p2;
        }
        if (comp->typ != Token::TOKEN_ALTERNATION) {
            a->p2 = nullptr; // No right side
        } else {
            free_node(*comp);
            comp = &expr(nullptr, Token::EXPR_SPECIAL, *a); // Parse right side
        }
        if (bal != Token::STMT_SIMPLE) {
            if (comp->typ != Token::TOKEN_RPAREN) // Should end with right paren
                goto merr;
            free_node(*comp);
            comp = &compon();
        }
        b_token = comp->typ;
        if (b_token != Token::TOKEN_UNANCHORED &&
            b_token != Token::TOKEN_MULT) // Should be alternation or equals
            goto merr;
        list->p2  = a;
        list->typ = Token::TOKEN_ALTERNATION; // Alternation pattern
        a->typ    = bal;
        free_node(*comp);
        comp = &compon();
        goto l3;

    case Token::TOKEN_END:
    case Token::TOKEN_ALTERNATION: // Pattern alternation or end of pattern (goto)
    case Token::TOKEN_COMMA:       // Comma (goto)
    case Token::TOKEN_EQUALS:      // Equals (assignment)
    case Token::TOKEN_RPAREN:      // Right paren (used in expressions)
        // End of pattern - fall through to normal return
        break;

    default:
        // Other token types not valid in pattern context
        goto merr;
    }
    // Note: Pattern replacement is detected in compile() by checking if both m and as exist.
    list->typ = Token::TOKEN_END;
    return *comp;

merr:
    writes("unrecognized component in match");
    // Never reached, but needed for compilation
    a = &alloc();
    return *a;
}

//
// Compile a single Snobol statement.
// Handles labels, assignments, pattern matching, goto statements, and function definitions.
// Returns a compiled statement node.
//
Node *SnobolContext::compile()
{
    Node *b, *comp;
    Node *r, *l, *xs, *xf, *g;
    Token a;
    Node *m, *as;
    Token t;

    m    = nullptr;            // Match pattern
    l    = nullptr;            // Label
    as   = nullptr;            // Assignment target
    xs   = nullptr;            // Success goto
    xf   = nullptr;            // Failure goto
    t    = Token::STMT_SIMPLE; // Statement type
    comp = &compon();
    a    = comp->typ;
    // Check for optional label
    if (a == Token::TOKEN_VARIABLE) {
        l = comp->p1;
        free_node(*comp);
        comp = &compon();
        a    = comp->typ;
    }
    if (a != Token::TOKEN_WHITESPACE)
        writes("no space beginning statement");
    free_node(*comp);
    // Check for function definition
    if (l == lookdef)
        goto def;
    // Parse expression (subject of statement)
    r    = &alloc();
    comp = &expr(nullptr, Token::TOKEN_DIV, *r);
    a    = comp->typ;
    if (a == Token::TOKEN_END) // End of statement
        goto asmble;
    if (a == Token::TOKEN_ALTERNATION) // Slash - goto statement
        goto xfer;
    if (a == Token::TOKEN_EQUALS) // Equals - assignment
        goto assig;
    // Pattern matching statement
    m    = &alloc();
    comp = &match(comp, *m);
    a    = comp->typ;
    if (a == Token::TOKEN_END)
        goto asmble;
    if (a == Token::TOKEN_ALTERNATION)
        goto xfer;
    if (a == Token::TOKEN_EQUALS) {
        goto assig;
    }
    writes("unrecognized component in match");
    return nullptr;

assig:
    // Parse assignment value
    free_node(*comp);
    as   = &alloc();
    comp = &expr(nullptr, Token::EXPR_SPECIAL, *as);
    a    = comp->typ;
    if (a == Token::TOKEN_END)
        goto asmble;
    if (a == Token::TOKEN_ALTERNATION)
        goto xfer;
    writes("unrecognized component in assignment");
    return nullptr;

xfer:
    // Parse goto target(s)
    free_node(*comp);
    comp = &compon();
    a    = comp->typ;
    if (a == Token::TOKEN_LPAREN) // Left paren - both success and failure targets
        goto xboth;
    if (a == Token::TOKEN_END) { // End of statement - no goto
        if (xs != nullptr || xf != nullptr)
            goto asmble;
        goto xerr;
    }
    if (a != Token::TOKEN_VARIABLE) // Should be a label
        goto xerr;
    b = comp->p1;
    free_node(*comp);
    if (b == looks) // "s" - success goto
        goto xsuc;
    if (b == lookf) // "f" - failure goto
        goto xfail;

xerr:
    writes("unrecognized component in goto");
    return nullptr;

xboth:
    // Parse both success and failure goto targets: (success, failure)
    free_node(*comp);
    xs   = &alloc();
    xf   = &alloc();
    comp = &expr(nullptr, Token::EXPR_SPECIAL, *xs); // Parse success target
    if (comp->typ != Token::TOKEN_RPAREN)            // Should end with right paren
        goto xerr;
    xf->p2 = xs->p2; // Share expression list
    comp   = &compon();
    if (comp->typ != Token::TOKEN_END)
        goto xerr;
    goto asmble;

xsuc:
    // Parse success goto: s(label)
    if (xs)
        goto xerr;
    comp = &compon();
    if (comp->typ != Token::TOKEN_LPAREN)
        goto xerr;
    xs   = &alloc();
    comp = &expr(nullptr, Token::EXPR_SPECIAL, *xs);
    if (comp->typ != Token::TOKEN_RPAREN)
        goto xerr;
    goto xfer;

xfail:
    // Parse failure goto: f(label)
    if (xf)
        goto xerr;
    comp = &compon();
    if (comp->typ != Token::TOKEN_LPAREN)
        goto xerr;
    xf   = &alloc();
    comp = &expr(nullptr, Token::EXPR_SPECIAL, *xf);
    if (comp->typ != Token::TOKEN_RPAREN)
        goto xerr;
    goto xfer;

asmble:
    // Assemble the compiled statement
    if (l) {
        if (l->typ != Token::TOKEN_END)
            writes("name doubly defined");
        l->p2  = comp;
        l->typ = Token::EXPR_LABEL; // type label;
    }
    comp->p2 = r; // Link to expression
    // Check if this is a pattern replacement (both m and as exist)
    // Pattern replacement occurs when match() returns TOKEN_EQUALS and we then parse an assignment
    if (m && as) {
        // Pattern replacement: r->p1 = m, m->p1 = as, as->p1 = g
        t     = Token::STMT_REPLACE; // Type 3: pattern replacement
        r->p1 = m;
        m->p1 = as;
        r     = as; // Set r to as for goto structure linking
    } else if (m) {
        t     = Token::STMT_MATCH; // Type 1: pattern matching statement
        r->p1 = m;
        r     = m;
    } else if (as) {
        t     = Token::STMT_ASSIGN; // Type 2: assignment statement
        r->p1 = as;
        r     = as;
    }
    // Build goto structure: g->p1 = success goto, g->p2 = failure goto
    g     = &alloc();
    g->p1 = nullptr;
    if (xs) {
        g->p1 = xs->p2; // Success goto target (expression list)
        free_node(*xs);
    }
    g->p2 = nullptr;
    if (xf) {
        g->p2 = xf->p2; // Failure goto target (expression list)
        free_node(*xf);
    }
    r->p1     = g;  // Link goto structure to statement
    comp->typ = t;  // Statement type: 0=simple, 1=match, 2=assign, 3=replace
    comp->ch  = lc; // Store line number
    return (comp);

def:
    // Parse function definition: define name(params) body
    r = &nscomp();
    if (r->typ != Token::TOKEN_VARIABLE) // Should be function name
        goto derr;
    l = r->p1;
    if (l->typ != Token::TOKEN_END)
        writes("name doubly defined");
    l->typ = Token::EXPR_FUNCTION; // type function;
    {
        Node *a_ptr = r;
        l->p2       = a_ptr;
        r           = &nscomp();
        l           = r;
        a_ptr->p1   = l;
        if (r->typ == Token::TOKEN_END) // No parameters
            goto d4;
        if (r->typ != Token::TOKEN_LPAREN) // Should start with left paren
            goto derr;

    d2:
        // Parse parameter list
        r = &nscomp();
        if (r->typ != Token::TOKEN_VARIABLE) // Should be parameter name
            goto derr;
        a_ptr->p2 = r;
        r->typ    = Token::EXPR_VAR_REF;
        a_ptr     = r;
        r         = &nscomp();
        if (r->typ == Token::TOKEN_COMMA) { // Comma - more parameters
            free_node(*r);
            goto d2;
        }
        if (r->typ != Token::TOKEN_RPAREN) // Should end with right paren
            goto derr;
        free_node(*r);
        r = &compon();
        if (r->typ != Token::TOKEN_END) // Should be end of statement
            goto derr;
        free_node(*r);

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
