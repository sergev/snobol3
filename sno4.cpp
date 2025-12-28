#include "sno.h"

//
// Evaluate an operand from the evaluation stack.
// Handles variable references, function calls, and special values.
// Returns the value as a string node.
//
Node *SnobolContext::eval_operand(const Node &ptr)
{
    Node *a;
    Node &p = const_cast<Node &>(ptr);
    a       = p.p1;
    if (p.typ == Token::EXPR_VAR_REF) {
        // Variable reference - get its value
        switch (a->typ) {
        case Token::EXPR_VAR_REF:       // Uninitialized variable
            a->typ = Token::EXPR_VALUE; // Initialize to empty string
            // fall through
        case Token::EXPR_VALUE: // String variable
            goto l1;
        case Token::EXPR_SYSPIT: // System function syspit (input)
            flush();
            return (syspit());
        case Token::EXPR_FUNCTION: // Function - get function body
            a = a->p2->p1;
            goto l1;
        case Token::EXPR_SPECIAL: // Special value - free space count
            return &binstr(nfree());
        default:
            writes("attempt to take an illegal value");
            goto l1;
        }
    l1:
        a = copy(a->p2); // Copy variable's value
    }
    return (a);
}

//
// Evaluate an expression tree using postfix evaluation.
// Processes operators and operands from the compiled expression.
// Returns the result as a string node.
//
Node *SnobolContext::eval(const Node &e, int t)
{
    Node *list, *a3, *a4, *a3base;
    Node *a1, *stack;
    Token op;
    Node *a2;

    // Postfix expression evaluation using a stack
    if (rfail == 1)
        return (nullptr);
    stack = nullptr;                // Evaluation stack: holds operands and operators
    list  = const_cast<Node *>(&e); // Current position in expression list
    goto l1;
advanc:
    list = list->p1;
l1:
    op = list->typ;
    switch (op) {
    default:
    case Token::TOKEN_END: // End of expression
        if (t == 1) {
            // Return value mode
            a1 = eval_operand(*stack);
            goto e1;
        }
        // Assignment mode - get variable reference
        if (stack->typ == Token::EXPR_VALUE)
            writes("attempt to store in a value");
        a1 = stack->p1;
    e1:
        stack = pop(stack);
        if (stack)
            writes("phase error");
        return (a1);
    case Token::TOKEN_DOLLAR: // Pattern immediate value ($)
        a1        = eval_operand(*stack);
        stack->p1 = &look(*a1); // Look up variable
        delete_string(a1);
        stack->typ = Token::EXPR_VAR_REF; // Mark as variable reference
        goto advanc;
    case Token::EXPR_CALL: // Function call
        if (stack->typ != Token::TOKEN_END)
            writes("illegal function");
        a1 = stack->p1;
        if (a1->typ != Token::EXPR_FUNCTION)
            writes("illegal function");
        a1 = a1->p2;
        {
            Node *op_ptr = a1->p1; // Function body
            a3base = a3 = &alloc();
            a3->p2      = op_ptr->p2; // Save return address
            op_ptr->p2  = nullptr;
            a1          = a1->p2;   // Parameter list
            a2          = list->p2; // Argument list
        f1:
            // Match parameters to arguments
            if (a1 != nullptr && a2 != nullptr)
                goto f2;
            if (a1 != a2)
                writes("parameters do not match");
            op_ptr = op_ptr->p1;
            goto f3;
        f2:
            // Bind parameter to argument value
            a3->p1 = a4 = &alloc();
            a3          = a4;
            a3->p2      = eval_operand(*a1);    // Save old parameter value
            assign(*a1->p1, *eval(*a2->p2, 1)); // recursive
            a1 = a1->p2;
            a2 = a2->p1;
            goto f1;
        f3:
            // Execute function body
            op_ptr = execute(*op_ptr); // recursive
            if (op_ptr)
                goto f3;
            // Restore parameter values
            a1 = stack->p1->p2;
            {
                Node *op_ptr2 = a1->p1;
                a3            = a3base;
                stack->p1     = op_ptr2->p2; // Get return value
                stack->typ    = Token::EXPR_VALUE;
                op_ptr2->p2   = a3->p2; // Restore return address
            f4:
                // Restore each parameter
                a4 = a3->p1;
                free_node(*a3);
                a3 = a4;
                a1 = a1->p2;
                if (a1 == nullptr)
                    goto advanc;
                assign(*a1->p1, *a3->p2);
                goto f4;
            }
        }
    case Token::TOKEN_DIV:        // Division
    case Token::TOKEN_MULT:       // Multiplication
    case Token::TOKEN_MINUS:      // Subtraction
    case Token::TOKEN_PLUS:       // Addition
    case Token::TOKEN_WHITESPACE: // Concatenation
        // Binary operator - evaluate both operands
        a1    = eval_operand(*stack);
        stack = pop(stack);
        a2    = eval_operand(*stack);
        a3    = doop(op, *a2, *a1);
        delete_string(a1);
        delete_string(a2);
        stack->p1  = a3;
        stack->typ = Token::EXPR_VALUE;
        goto advanc;
    case Token::TOKEN_STRING: // String literal
        a1 = copy(list->p2);
        {
            stack      = &push(stack);
            stack->p1  = a1;
            stack->typ = Token::EXPR_VALUE; // Mark as value
            goto advanc;
        }
    case Token::TOKEN_VARIABLE: // Variable reference
        a1 = list->p2;
        {
            stack      = &push(stack);
            stack->p1  = a1;
            stack->typ = Token::EXPR_VAR_REF; // Mark as variable reference
            goto advanc;
        }
        goto advanc;
    }
    return nullptr;
}

//
// Execute a binary operator on two string operands.
// Converts strings to numbers for arithmetic operations.
//
Node *SnobolContext::doop(Token op, const Node &arg1, const Node &arg2)
{
    switch (op) {
    case Token::TOKEN_DIV: // Division
        return &divide(arg1, arg2);
    case Token::TOKEN_MULT: // Multiplication
        return &mult(arg1, arg2);
    case Token::TOKEN_PLUS: // Addition
        return &add(arg1, arg2);
    case Token::TOKEN_MINUS: // Subtraction
        return &sub(arg1, arg2);
    case Token::TOKEN_WHITESPACE: // Concatenation
        return (cat(&arg1, &arg2));
    default:
        return (nullptr);
    }
}

//
// Execute a compiled statement.
// Handles simple statements, pattern matching, assignments, and goto operations.
// Returns the next statement to execute, or NULL to stop.
//
Node *SnobolContext::execute(const Node &e)
{
    Node *r, *b, *c;
    Node *m, *ca, *d, *a;

    r  = e.p2; // Statement data
    lc = e.ch; // Line number
    switch (e.typ) {
    case Token::STMT_SIMPLE: // r g - Simple statement: evaluate expression and goto
        a = r->p1;
        delete_string(eval(*r->p2, 1));
        goto xsuc;
    case Token::STMT_MATCH:  // r m g - Pattern matching: match pattern against subject
        m = r->p1;           // Match pattern
        a = m->p1;           // Goto structure
        b = eval(*r->p2, 1); // Evaluate subject
        c = search(*m, b);   // Search for pattern
        delete_string(b);
        if (c == nullptr)
            goto xfail;
        free_node(*c);
        goto xsuc;
    case Token::STMT_ASSIGN:           // r a g - Assignment: assign value to variable
        ca = r->p1;                    // Assignment structure
        a  = ca->p1;                   // Goto structure
        b  = eval(*r->p2, 0);          // Get variable reference
        assign(*b, *eval(*ca->p2, 1)); // Assign value
        goto xsuc;
    case Token::STMT_REPLACE:   // r m a g - Pattern replacement
        // PATTERN REPLACEMENT LOGIC:
        // After search() returns match result 'd', use d->p1 and d->p2 to determine
        // which parts of the string to keep:
        //
        //   d->p1: Character BEFORE match start (nullptr if match at start)
        //   d->p2: Character AFTER match end (r->p2 if match at end)
        //
        // REPLACEMENT CASES:
        //   1. Match spans entire string:
        //      Condition: (d->p1 == nullptr || d->p1 == b->p2->p1) && d->p2 == b->p2->p2
        //      Action: Replace entire string with replacement value
        //
        //   2. Match at start only:
        //      Condition: (d->p1 == nullptr || d->p1 == b->p2->p1) && d->p2 != b->p2->p2
        //      Action: replacement + part after match
        //
        //   3. Match at end only:
        //      Condition: d->p1 != nullptr && d->p1 != b->p2->p1 && d->p2 == b->p2->p2
        //      Action: part before match + replacement
        //
        //   4. Match in middle:
        //      Condition: d->p1 != nullptr && d->p1 != b->p2->p1 && d->p2 != b->p2->p2
        //      Action: part before match + replacement + part after match
        //
        m  = r->p1;             // Match pattern
        ca = m->p1;             // Assignment structure
        a  = ca->p1;            // Goto structure
        b  = eval(*r->p2, 0);   // Get variable reference
        d  = search(*m, b->p2); // Search pattern in variable's value
        if (d == nullptr)
            goto xfail;
        c = eval(*ca->p2, 1); // Evaluate replacement value
        // Check if match spans entire string (starts at beginning AND ends at end)
        if ((d->p1 == nullptr || d->p1 == b->p2->p1) && d->p2 == b->p2->p2) {
            // Match entire string - replace
            assign(*b, *c);
            free_node(*d);
            delete_string(c);
            goto xsuc;
        }
        {
            // Build result: [before] + [replacement] + [after]
            Node* result = c;
            // Add part before match if match doesn't start at beginning
            if (d->p1 != nullptr && d->p1 != b->p2->p1) {
                Node* before = &alloc();
                before->p1 = b->p2->p1;  // First character
                before->p2 = d->p1;       // Last character before match
                result = cat(before, result);
                free_node(*before);
            }
            // Add part after match if match doesn't end at end
            if (d->p2 != nullptr && d->p2 != b->p2->p2) {
                Node* after = &alloc();
                after->p1 = d->p2;        // First character after match
                after->p2 = b->p2->p2;    // Last character (end marker)
                result = cat(result, after);
                free_node(*after);
            }
            assign(*b, *result);
            free_node(*d);
            delete_string(c);
            goto xsuc;
        }

    default:
        // Invalid statement type
        writes("invalid statement type");
        return nullptr;
    }
xsuc:
    // Success path - check for goto
    if (rfail)
        goto xfail;
    b = a->p1; // Success goto
    goto xboth;
xfail:
    // Failure path - check for goto
    rfail = 0;
    b     = a->p2; // Failure goto
xboth:
    if (b == nullptr) {
        // No goto - continue to next statement
        return (e.p1);
    }
    // Evaluate goto target
    b = eval(*b, 0);
    if (b == lookret) // Return statement
        return (nullptr);
    if (b == lookfret) { // Failure return
        rfail = 1;
        return (nullptr);
    }
    if (b->typ != Token::EXPR_LABEL) // Should be a label
        writes("attempt to transfer to non-label");
    return (b->p2); // Return label's statement
}

//
// Assign a value to a variable or output location.
// Handles variable assignment, output, and function parameter assignment.
//
void SnobolContext::assign(Node &addr, Node &value)
{
    Node *a;

    if (rfail == 1) {
        // Don't assign on failure
        delete_string(&value);
        return;
    }
    switch (addr.typ) {
    default:
        writes("attempt to make an illegal assignment");
        return;
    case Token::EXPR_VAR_REF: // Uninitialized variable
        addr.typ = Token::EXPR_VALUE;
        // fall through
    case Token::EXPR_VALUE: // String variable
        delete_string(addr.p2);
        addr.p2 = &value;
        return;
    case Token::EXPR_SYSPOT: // Output (syspot)
        sysput(&value);
        return;
    case Token::EXPR_FUNCTION: // Function parameter
        a = addr.p2->p1;
        delete_string(a->p2);
        a->p2 = &value;
        return;
    }
}
