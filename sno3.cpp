#include "sno.h"

//
// Extend a balanced pattern match (handles nested parentheses).
// Extends the match position forward while maintaining balanced parentheses.
// Returns the number of characters matched, or 0 on failure.
//
int Node::bextend(const Node *last)
{
    Node *a, *s;
    int b;
    Node *c;
    int d;
    CharClass class_val;

    s = this;
    c = s->p1;
    if (c == nullptr)
        goto bad;
    b = 0; // Parenthesis balance counter
    d = 0; // Character count
    a = s->p2;
    if (a == nullptr) {
        a = c;
        goto eb2;
    }
eb1:
    if (a == last)
        goto bad;
    a = a->p1;
eb2:
    d++;
    class_val = SnobolContext::char_class(a->ch);
    if (class_val == CharClass::RPAREN) { // rp - right parenthesis
        if (b == 0)
            goto bad;
        b--;
        goto eb3;
    }
    if (class_val == CharClass::LPAREN) { // lp - left parenthesis
        b++;
        goto eb1;
    }
eb3:
    if (b == 0) { // Balanced - found end of pattern
        s->p2 = a;
        return (d);
    }
    goto eb1;
bad:
    return (0);
}

//
// Extend an unbalanced pattern match (simple extension by one character).
// Advances the match position by one character.
// Returns 1 on success, 0 on failure.
//
int Node::ubextend(const Node *last)
{
    Node *a, *b, *s;

    s = this;
    a = s->p1;
    if (a == nullptr)
        goto bad;
    b = s->p2;
    if (b == nullptr)
        goto good;
    if (b == last)
        goto bad;
    a = b->p1;
good:
    s->p2 = a;
    return (1);
bad:
    return (0);
}

//
// Search for a pattern match in the subject string.
// Implements backtracking pattern matching algorithm for Snobol patterns.
// Returns a match result node on success, NULL on failure.
//
Node *SnobolContext::search(Node *arg, Node *r)
{
    Node *list, *back, *str, *etc, *next, *last, *base, *e;
    Node *a, *b, *var;
    Token c;
    Token d_token;
    int d;
    intptr_t len;

    // Initialize pattern matching state
    a    = arg->p2;        // Start of pattern component list
    list = base = alloc(); // Base of matching state list
    last        = nullptr; // End of subject string (set later)
    next        = nullptr; // Next position to match from
    goto badv1;
badvanc:
    // Build pattern matching state from pattern components
    a = a->p1;
    if (a->typ == Token::TOKEN_END) {
        // End of pattern - initialize search
        list->p1 = nullptr;
        if (rfail == 1) {
            a = nullptr;
            goto fail;
        }
        list = base;
        if (r == nullptr)
            next = last = nullptr;
        else {
            next = r->p1; // Start of subject string
            last = r->p2; // End of subject string
        }
        goto adv1;
    }
    b        = alloc();
    list->p1 = b;
    list     = b;
badv1:
    // Set up backtracking structure for this pattern component
    list->p2 = back = alloc();
    back->p1        = last;
    b               = a->p2;
    c               = a->typ;
    list->typ       = c;
    if (static_cast<int>(c) < static_cast<int>(Token::STMT_ASSIGN)) {
        // Simple pattern component - evaluate and store
        back->p2 = eval(b, 1);
        goto badvanc;
    }
    // Complex pattern component - set up match state
    last     = list;
    str      = alloc(); // Match position tracker
    etc      = alloc(); // Pattern metadata
    back->p2 = var = alloc();
    var->typ       = b->typ; // Pattern type (1=balanced, 2=unbalanced, 3=concatenated)
    var->p1        = str;
    var->p2        = etc;
    e              = b->p1;
    if (e == nullptr)
        etc->p1 = nullptr; // No left side
    else
        etc->p1 = eval(e, 0); // Evaluate left pattern
    e = b->p2;
    if (e == nullptr)
        etc->p2 = nullptr; // No right side
    else {
        e       = eval(e, 1);                        // Evaluate right pattern (length)
        etc->p2 = (Node *)(long)(intptr_t)strbin(e); // Store as integer
        delete_string(e);
    }
    goto badvanc;

retard:
    // Backtrack to previous pattern component
    a = back->p1;
    if (a == nullptr) {
        rfail = 1;
        goto fail;
    }
    list = a;
    back = list->p2;
    var  = back->p2;
    str  = var->p1;
    etc  = var->p2;
    if (etc->p2) // Has length constraint - need to retry
        goto retard;
    // Try to extend current match
    if (var->typ == Token::TOKEN_UNANCHORED) { // Balanced pattern (value 1)
        if (str->bextend(last) == 0)
            goto retard;
        goto adv0;
    }
    if (str->ubextend(last) == 0) // Unbalanced pattern
        goto retard;
adv0:
    // Advance to next position
    a = str->p2;
adv01:
    if (a == last)
        next = nullptr;
    else
        next = a->p1;
advanc:
    // Process next pattern component
    a = list->p1;
    if (a == nullptr) {
        // End of pattern - check if match succeeded
        a = alloc();
        if (r == nullptr) {
            a->p1 = a->p2 = nullptr;
            goto fail;
        }
        b     = r->p1;
        a->p1 = b;
        if (next == nullptr) {
            a->p2 = r->p2;
            goto fail;
        }
        // Find match end position
        while (1) {
            e = b->p1;
            if (e == next) {
                a->p2 = b;
                goto fail;
            }
            b = e;
        }
    }
    list = a;
adv1:
    back    = list->p2;
    var     = back->p2;
    d_token = list->typ;
    if (static_cast<int>(d_token) < 2) {
        // Simple pattern - match string directly
        if (var == nullptr)
            goto advanc;
        if (next == nullptr)
            goto retard;
        a = next;
        b = var->p1;
        e = var->p2;
        while (1) {
            if (a->ch != b->ch)
                goto retard;
            if (b == e) // Matched entire string
                goto adv01;
            if (a == last) // Reached end of subject
                goto retard;
            a = a->p1;
            b = b->p1;
        }
    }
    // Complex pattern - handle alternation or concatenation
    str     = var->p1;
    etc     = var->p2;
    str->p1 = next;
    str->p2 = nullptr;
    len     = (intptr_t)etc->p2;               // Length constraint
    if (var->typ == Token::TOKEN_UNANCHORED) { // Balanced pattern (value 1)
        d = str->bextend(last);
        if (d == 0)
            goto retard;
        if (len == 0) // No length constraint
            goto adv0;
        // Match with length constraint
        while (1) {
            len = -d;
            if (len == 0)
                goto adv0;
            if (len < 0)
                goto retard;
            d = str->bextend(last);
            if (d == 0)
                goto retard;
        }
    }
    if (len == 0) {                      // No length constraint
        if (d == 3 && next != nullptr) { // Concatenated pattern
            str->p2 = last;
            goto adv0;
        }
        goto advanc;
    }
    // Match with specific length
    while (len--)
        if (str->ubextend(last) == 0)
            goto retard;
    goto adv0;

fail:
    // Cleanup and assign matched substrings to variables
    list = base;
    goto f1;
fadv:
    free_node(back);
    b = list->p1;
    free_node(list);
    if (b == nullptr)
        return (a);
    list = b;
f1:
    back = list->p2;
    var  = back->p2;
    if (static_cast<int>(list->typ) < static_cast<int>(Token::TOKEN_ALTERNATION)) {
        // Simple pattern - no assignment needed
        delete_string(var);
        goto fadv;
    }
    // Complex pattern - assign matched substring
    str = var->p1;
    etc = var->p2;
    if (a != nullptr && etc->p1 != nullptr) {
        // Assign matched substring to variable
        if (str->p2 == nullptr) {
            free_node(str);
            str = nullptr;
        }
        assign(etc->p1, copy(str));
    }
    if (str)
        free_node(str);
    free_node(etc);
    free_node(var);
    goto fadv;
}
