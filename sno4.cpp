#include <iostream>

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
    a       = p.head;
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
            a = a->tail->head;
            goto l1;
        default:
            writes("attempt to take an illegal value");
            goto l1;
        }
    l1:
        a = copy(a->tail); // Copy variable's value
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
    list = list->head;
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
        a1 = stack->head;
    e1:
        stack = pop(stack);
        if (stack)
            writes("phase error");
        return (a1);
    case Token::TOKEN_DOLLAR: // Pattern immediate value ($)
        a1          = eval_operand(*stack);
        stack->head = &look(*a1); // Look up variable
        delete_string(a1);
        stack->typ = Token::EXPR_VAR_REF; // Mark as variable reference
        goto advanc;
    case Token::TOKEN_CALL: // Function call
        if (!stack || stack->typ != Token::EXPR_VAR_REF)
            writes("illegal function");
        a1 = stack->head;
        // If a1 is a variable node (TOKEN_VARIABLE), follow head to get the symbol
        if (a1->typ == Token::TOKEN_VARIABLE) {
            a1 = a1->head;
        }
        if (!a1 || a1->typ != Token::EXPR_FUNCTION)
            writes("illegal function");
        a1 = a1->tail; // Get a_ptr (variable node) from function symbol
        {
            Node *op_ptr =
                a1->head
                    ->head; // Function body (a_ptr->head is symbol, symbol->head is function body)
            a3base = a3  = &alloc();
            a3->tail     = op_ptr->tail; // Save return address
            op_ptr->tail = nullptr;
            a1           = a1->tail;   // Parameter list
            a2           = list->tail; // Argument list
        f1:
            // Match parameters to arguments
            if (a1 != nullptr && a2 != nullptr)
                goto f2;
            if (a1 != a2)
                writes("parameters do not match");
            op_ptr = op_ptr->head;
            goto f3;
        f2:
            // Bind parameter to argument value
            a3->head = a4 = &alloc();
            a3            = a4;
            a3->tail      = eval_operand(*a1);      // Save old parameter value
            assign(*a1->head, *eval(*a2->tail, 1)); // recursive
            a1 = a1->tail;
            a2 = a2->head;
            goto f1;
        f3:
            // Execute function body
            op_ptr = execute(*op_ptr); // recursive
            if (op_ptr)
                goto f3;
            // Restore parameter values
            // Get a_ptr: stack->head is function name, need to get back to a_ptr
            // stack->head is TOKEN_VARIABLE node, stack->head->head is symbol (EXPR_FUNCTION)
            // symbol->tail is a_ptr
            a1 = stack->head;
            if (a1->typ == Token::TOKEN_VARIABLE) {
                a1 = a1->head; // Get symbol
            }
            a1 = a1->tail; // Get a_ptr
            {
                Node *op_ptr2 = a1->head;
                a3            = a3base;
                stack->head   = op_ptr2->tail; // Get return value
                stack->typ    = Token::EXPR_VALUE;
                op_ptr2->tail = a3->tail; // Restore return address
            f4:
                // Restore each parameter
                a4 = a3->head;
                free_node(*a3);
                a3 = a4;
                a1 = a1->tail;
                if (a1 == nullptr)
                    goto advanc;
                assign(*a1->head, *a3->tail);
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
        stack->head = a3;
        stack->typ  = Token::EXPR_VALUE;
        goto advanc;
    case Token::TOKEN_STRING: // String literal
        a1 = copy(list->tail);
        {
            stack       = &push(stack);
            stack->head = a1;
            stack->typ  = Token::EXPR_VALUE; // Mark as value
            goto advanc;
        }
    case Token::TOKEN_VARIABLE: // Variable reference
        a1 = list->tail;
        {
            stack       = &push(stack);
            stack->head = a1;
            stack->typ  = Token::EXPR_VAR_REF; // Mark as variable reference
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

    r  = e.tail; // Statement data
    lc = e.ch;   // Line number
    switch (e.typ) {
    case Token::STMT_SIMPLE: // r g - Simple statement: evaluate expression and goto
        a = r->head;
        delete_string(eval(*r->tail, 1));
        goto xsuc;
    case Token::STMT_MATCH:    // r m g - Pattern matching: match pattern against subject
        m = r->head;           // Match pattern
        a = m->head;           // Goto structure
        b = eval(*r->tail, 1); // Evaluate subject
        c = search(*m, b);     // Search for pattern
        delete_string(b);
        if (c == nullptr)
            goto xfail;
        free_node(*c);
        goto xsuc;
    case Token::STMT_ASSIGN:             // r a g - Assignment: assign value to variable
        ca = r->head;                    // Assignment structure
        a  = ca->head;                   // Goto structure
        b  = eval(*r->tail, 0);          // Get variable reference
        assign(*b, *eval(*ca->tail, 1)); // Assign value
        goto xsuc;
    case Token::STMT_REPLACE: // r m a g - Pattern replacement
        // search() returns: d->head = char before match (nullptr if at start), d->tail = char after
        // match (r->tail if at end)
        {
            Node *before_node, *after_node, *result_node;
            m  = r->head;             // Match pattern
            ca = m->head;             // Assignment structure
            a  = ca->head;            // Goto structure
            b  = eval(*r->tail, 0);   // Get variable reference
            d  = search(*m, b->tail); // Search pattern in variable's value
            if (d == nullptr)
                goto xfail;
            c = eval(*ca->tail, 1); // Evaluate replacement value
            // Check if match spans entire string
            if (d->head == nullptr && d->tail == b->tail->tail) {
                // Match entire string - replace
                assign(*b, *c);
                free_node(*d);
                delete_string(c);
                goto xsuc;
            }
            // Check if match at end
            if (d->tail == b->tail->tail) {
                // Match at end - replace: [before] + [replacement]
                before_node       = &alloc();
                before_node->head = b->tail->head; // First character
                before_node->tail = d->head;       // Last character before match
                result_node       = cat(before_node, c);
                free_node(*before_node);
                assign(*b, *result_node);
                free_node(*d);
                delete_string(c);
                goto xsuc;
            }
            // Check if match at start
            if (d->head == nullptr) {
                // Match at start - replace: [replacement] + [after]
                after_node       = &alloc();
                after_node->head = d->tail;       // First character after match
                after_node->tail = b->tail->tail; // End marker
                result_node      = cat(c, after_node);
                free_node(*after_node);
                assign(*b, *result_node);
                free_node(*d);
                delete_string(c);
                goto xsuc;
            }
            // Match in middle - replace: [before] + [replacement] + [after]
            before_node       = &alloc();
            before_node->head = b->tail->head; // First character
            before_node->tail = d->head;       // Last character before match
            after_node        = &alloc();
            after_node->head  = d->tail;       // First character after match
            after_node->tail  = b->tail->tail; // End marker
            result_node       = cat(before_node, c);
            free_node(*before_node);
            result_node = cat(result_node, after_node);
            free_node(*after_node);
            assign(*b, *result_node);
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
    b = a->head; // Success goto
    goto xboth;
xfail:
    // Failure path - check for goto
    rfail = 0;
    b     = a->tail; // Failure goto
xboth:
    if (b == nullptr) {
        // No goto - continue to next statement
        return (e.head);
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
    return (b->tail); // Return label's statement
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
        delete_string(addr.tail);
        addr.tail = &value;
        return;
    case Token::EXPR_SYSPOT: // Output (syspot)
        sysput(&value);
        return;
    case Token::EXPR_FUNCTION: // Function parameter
        a = addr.tail->head;
        delete_string(a->tail);
        a->tail = &value;
        return;
    }
}
