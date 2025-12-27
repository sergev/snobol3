#include "sno.h"

//
// Evaluate an operand from the evaluation stack.
// Handles variable references, function calls, and special values.
// Returns the value as a string node.
//
node_t *eval_operand(snobol_context_t *ctx, node_t *ptr)
{
    node_t *a, *p;

    p = ptr;
    a = p->p1;
    if (p->typ == EXPR_VAR_REF) {
        // Variable reference - get its value
        switch (a->typ) {
        case EXPR_VAR_REF:       // Uninitialized variable
            a->typ = EXPR_VALUE; // Initialize to empty string
            /* fall through */
        case EXPR_VALUE: // String variable
            goto l1;
        case EXPR_SYSPIT: // System function syspit (input)
            flush();
            return (syspit(ctx));
        case EXPR_FUNCTION: // Function - get function body
            a = a->p2->p1;
            goto l1;
        case EXPR_SPECIAL: // Special value - free space count
            return (binstr(ctx, nfree(ctx)));
        default:
            writes(ctx, "attempt to take an illegal value");
            goto l1;
        }
    l1:
        a = copy(ctx, a->p2); // Copy variable's value
    }
    return (a);
}

//
// Evaluate an expression tree using postfix evaluation.
// Processes operators and operands from the compiled expression.
// Returns the result as a string node.
//
node_t *eval(snobol_context_t *ctx, node_t *e, int t)
{
    node_t *list, *a3, *a4, *a3base;
    node_t *a1, *stack;
    int op;
    node_t *a2;

    // Postfix expression evaluation using a stack
    if (ctx->rfail == 1)
        return (NULL);
    stack = NULL; // Evaluation stack: holds operands and operators
    list  = e;    // Current position in expression list
    goto l1;
advanc:
    list = list->p1;
l1:
    op = list->typ;
    switch (op) {
    default:
    case TOKEN_END: // End of expression
        if (t == 1) {
            // Return value mode
            a1 = eval_operand(ctx, stack);
            goto e1;
        }
        // Assignment mode - get variable reference
        if (stack->typ == EXPR_VALUE)
            writes(ctx, "attempt to store in a value");
        a1 = stack->p1;
    e1:
        stack = pop(ctx, stack);
        if (stack)
            writes(ctx, "phase error");
        return (a1);
    case TOKEN_DOLLAR: // Pattern immediate value ($)
        a1        = eval_operand(ctx, stack);
        stack->p1 = look(ctx, a1); // Look up variable
        delete_string(ctx, a1);
        stack->typ = EXPR_VAR_REF; // Mark as variable reference
        goto advanc;
    case EXPR_CALL: // Function call
        if (stack->typ)
            writes(ctx, "illegal function");
        a1 = stack->p1;
        if (a1->typ != EXPR_FUNCTION)
            writes(ctx, "illegal function");
        a1 = a1->p2;
        {
            node_t *op_ptr = a1->p1; // Function body
            a3base = a3 = alloc(ctx);
            a3->p2      = op_ptr->p2; // Save return address
            op_ptr->p2  = NULL;
            a1          = a1->p2;   // Parameter list
            a2          = list->p2; // Argument list
        f1:
            // Match parameters to arguments
            if (a1 != NULL && a2 != NULL)
                goto f2;
            if (a1 != a2)
                writes(ctx, "parameters do not match");
            op_ptr = op_ptr->p1;
            goto f3;
        f2:
            // Bind parameter to argument value
            a3->p1 = a4 = alloc(ctx);
            a3          = a4;
            a3->p2      = eval_operand(ctx, a1);       // Save old parameter value
            assign(ctx, a1->p1, eval(ctx, a2->p2, 1)); /* recursive */
            a1 = a1->p2;
            a2 = a2->p1;
            goto f1;
        f3:
            // Execute function body
            op_ptr = execute(ctx, op_ptr); /* recursive */
            if (op_ptr)
                goto f3;
            // Restore parameter values
            a1 = stack->p1->p2;
            {
                node_t *op_ptr2 = a1->p1;
                a3              = a3base;
                stack->p1       = op_ptr2->p2; // Get return value
                stack->typ      = EXPR_VALUE;
                op_ptr2->p2     = a3->p2; // Restore return address
            f4:
                // Restore each parameter
                a4 = a3->p1;
                free_node(ctx, a3);
                a3 = a4;
                a1 = a1->p2;
                if (a1 == NULL)
                    goto advanc;
                assign(ctx, a1->p1, a3->p2);
                goto f4;
            }
        }
    case TOKEN_DIV:        // Division
    case TOKEN_MULT:       // Multiplication
    case TOKEN_MINUS:      // Subtraction
    case TOKEN_PLUS:       // Addition
    case TOKEN_WHITESPACE: // Concatenation
        // Binary operator - evaluate both operands
        a1    = eval_operand(ctx, stack);
        stack = pop(ctx, stack);
        a2    = eval_operand(ctx, stack);
        a3    = doop(ctx, op, a2, a1);
        delete_string(ctx, a1);
        delete_string(ctx, a2);
        stack->p1  = a3;
        stack->typ = EXPR_VALUE;
        goto advanc;
    case TOKEN_STRING: // String literal
        a1 = copy(ctx, list->p2);
        {
            stack      = push(ctx, stack);
            stack->p1  = a1;
            stack->typ = EXPR_VALUE; // Mark as value
            goto advanc;
        }
    case TOKEN_VARIABLE: // Variable reference
        a1 = list->p2;
        {
            stack      = push(ctx, stack);
            stack->p1  = a1;
            stack->typ = EXPR_VAR_REF; // Mark as variable reference
            goto advanc;
        }
        goto advanc;
    }
    return NULL;
}

//
// Execute a binary operator on two string operands.
// Converts strings to numbers for arithmetic operations.
//
node_t *doop(snobol_context_t *ctx, int op, node_t *arg1, node_t *arg2)
{
    switch (op) {
    case TOKEN_DIV: // Division
        return (divide(ctx, arg1, arg2));
    case TOKEN_MULT: // Multiplication
        return (mult(ctx, arg1, arg2));
    case TOKEN_PLUS: // Addition
        return (add(ctx, arg1, arg2));
    case TOKEN_MINUS: // Subtraction
        return (sub(ctx, arg1, arg2));
    case TOKEN_WHITESPACE: // Concatenation
        return (cat(ctx, arg1, arg2));
    }
    return (NULL);
}

//
// Execute a compiled statement.
// Handles simple statements, pattern matching, assignments, and goto operations.
// Returns the next statement to execute, or NULL to stop.
//
node_t *execute(snobol_context_t *ctx, node_t *e)
{
    node_t *r, *b, *c;
    node_t *m, *ca, *d, *a;

    r       = e->p2; // Statement data
    ctx->lc = e->ch; // Line number
    switch (e->typ) {
    case STMT_SIMPLE: /*  r g - Simple statement: evaluate expression and goto */
        a = r->p1;
        delete_string(ctx, eval(ctx, r->p2, 1));
        goto xsuc;
    case STMT_MATCH:             /*  r m g - Pattern matching: match pattern against subject */
        m = r->p1;               // Match pattern
        a = m->p1;               // Goto structure
        b = eval(ctx, r->p2, 1); // Evaluate subject
        c = search(ctx, m, b);   // Search for pattern
        delete_string(ctx, b);
        if (c == NULL)
            goto xfail;
        free_node(ctx, c);
        goto xsuc;
    case STMT_ASSIGN:                         /*  r a g - Assignment: assign value to variable */
        ca = r->p1;                           // Assignment structure
        a  = ca->p1;                          // Goto structure
        b  = eval(ctx, r->p2, 0);             // Get variable reference
        assign(ctx, b, eval(ctx, ca->p2, 1)); // Assign value
        goto xsuc;
    case TOKEN_EQUALS:              /*  r m a g - Pattern matching with assignment (value 3) */
        m  = r->p1;                 // Match pattern
        ca = m->p1;                 // Assignment structure
        a  = ca->p1;                // Goto structure
        b  = eval(ctx, r->p2, 0);   // Get variable reference
        d  = search(ctx, m, b->p2); // Search pattern in variable's value
        if (d == NULL)
            goto xfail;
        c = eval(ctx, ca->p2, 1); // Evaluate replacement value
        if (d->p1 == NULL) {
            // Match at end - append
            free_node(ctx, d);
            assign(ctx, b, cat(ctx, c, b->p2));
            delete_string(ctx, c);
            goto xsuc;
        }
        if (d->p2 == b->p2->p2) {
            // Match entire string - replace
            assign(ctx, b, c);
            free_node(ctx, d);
            goto xsuc;
        }
        // Match in middle - replace matched portion
        (r = alloc(ctx))->p1 = d->p2->p1;
        r->p2                = b->p2->p2;
        assign(ctx, b, cat(ctx, c, r));
        free_node(ctx, d);
        free_node(ctx, r);
        delete_string(ctx, c);
        goto xsuc;

    default:
        // Invalid statement type
        writes(ctx, "invalid statement type");
        return NULL;
    }
xsuc:
    // Success path - check for goto
    if (ctx->rfail)
        goto xfail;
    b = a->p1; // Success goto
    goto xboth;
xfail:
    // Failure path - check for goto
    ctx->rfail = 0;
    b          = a->p2; // Failure goto
xboth:
    if (b == NULL) {
        // No goto - continue to next statement
        return (e->p1);
    }
    // Evaluate goto target
    b = eval(ctx, b, 0);
    if (b == ctx->lookret) // Return statement
        return (NULL);
    if (b == ctx->lookfret) { // Failure return
        ctx->rfail = 1;
        return (NULL);
    }
    if (b->typ != EXPR_LABEL) // Should be a label
        writes(ctx, "attempt to transfer to non-label");
    return (b->p2); // Return label's statement
}

//
// Assign a value to a variable or output location.
// Handles variable assignment, output, and function parameter assignment.
//
void assign(snobol_context_t *ctx, node_t *adr, node_t *val)
{
    node_t *a, *addr, *value;

    addr  = adr;
    value = val;
    if (ctx->rfail == 1) {
        // Don't assign on failure
        delete_string(ctx, value);
        return;
    }
    switch (addr->typ) {
    default:
        writes(ctx, "attempt to make an illegal assignment");
        return;
    case EXPR_VAR_REF: // Uninitialized variable
        addr->typ = EXPR_VALUE;
        /* fall through */
    case EXPR_VALUE: // String variable
        delete_string(ctx, addr->p2);
        addr->p2 = value;
        return;
    case EXPR_SYSPOT: // Output (syspot)
        sysput(ctx, value);
        return;
    case EXPR_FUNCTION: // Function parameter
        a = addr->p2->p1;
        delete_string(ctx, a->p2);
        a->p2 = value;
        return;
    }
}
