#include "sno.h"

//
// Extend a balanced pattern match (handles nested parentheses).
// Extends the match position forward while maintaining balanced parentheses.
// Returns the number of characters matched, or 0 on failure.
//
int bextend(node_t *str, node_t *last)
{
    node_t *a, *s;
    int b;
    node_t *c;
    int d;
    char_class_t class_val;

    s = str;
    c = s->p1;
    if (c == NULL)
        goto bad;
    b = 0; // Parenthesis balance counter
    d = 0; // Character count
    a = s->p2;
    if (a == NULL) {
        a = c;
        goto eb2;
    }
eb1:
    if (a == last)
        goto bad;
    a = a->p1;
eb2:
    d++;
    class_val = char_class(a->ch);
    if (class_val == CHAR_CLASS_RPAREN) { /* rp - right parenthesis */
        if (b == 0)
            goto bad;
        b--;
        goto eb3;
    }
    if (class_val == CHAR_CLASS_LPAREN) { /* lp - left parenthesis */
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
int ubextend(node_t *str, node_t *last)
{
    node_t *a, *b, *s;

    s = str;
    a = s->p1;
    if (a == NULL)
        goto bad;
    b = s->p2;
    if (b == NULL)
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
node_t *search(node_t *arg, node_t *r)
{
    node_t *list, *back, *str, *etc, *next, *last, *base, *e;
    node_t *a, *b, *var;
    int c, d;

    // Initialize pattern matching state
    a    = arg->p2;        // Start of pattern component list
    list = base = alloc(); // Base of matching state list
    last        = NULL;    // End of subject string (set later)
    next        = NULL;    // Next position to match from
    goto badv1;
badvanc:
    // Build pattern matching state from pattern components
    a = a->p1;
    if (a->typ == TOKEN_END) {
        // End of pattern - initialize search
        list->p1 = NULL;
        if (rfail == 1) {
            a = NULL;
            goto fail;
        }
        list = base;
        if (r == NULL)
            next = last = NULL;
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
    if (c < 2) {
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
    if (e == NULL)
        etc->p1 = NULL; // No left side
    else
        etc->p1 = eval(e, 0); // Evaluate left pattern
    e = b->p2;
    if (e == NULL)
        etc->p2 = NULL; // No right side
    else {
        e       = eval(e, 1);                          // Evaluate right pattern (length)
        etc->p2 = (node_t *)(long)(intptr_t)strbin(e); // Store as integer
        delete_string(e);
    }
    goto badvanc;

retard:
    // Backtrack to previous pattern component
    a = back->p1;
    if (a == NULL) {
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
    if (var->typ == TOKEN_UNANCHORED) { // Balanced pattern (value 1)
        if (bextend(str, last) == 0)
            goto retard;
        goto adv0;
    }
    if (ubextend(str, last) == 0) // Unbalanced pattern
        goto retard;
adv0:
    // Advance to next position
    a = str->p2;
adv01:
    if (a == last)
        next = NULL;
    else
        next = a->p1;
advanc:
    // Process next pattern component
    a = list->p1;
    if (a == NULL) {
        // End of pattern - check if match succeeded
        a = alloc();
        if (r == NULL) {
            a->p1 = a->p2 = NULL;
            goto fail;
        }
        b     = r->p1;
        a->p1 = b;
        if (next == NULL) {
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
    back = list->p2;
    var  = back->p2;
    d    = list->typ;
    if (d < 2) {
        // Simple pattern - match string directly
        if (var == NULL)
            goto advanc;
        if (next == NULL)
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
    str->p2 = NULL;
    c       = (int)(intptr_t)etc->p2;   // Length constraint
    if (var->typ == TOKEN_UNANCHORED) { // Balanced pattern (value 1)
        d = bextend(str, last);
        if (d == 0)
            goto retard;
        if (c == 0) // No length constraint
            goto adv0;
        // Match with length constraint
        while (1) {
            c = -d;
            if (c == 0)
                goto adv0;
            if (c < 0)
                goto retard;
            d = bextend(str, last);
            if (d == 0)
                goto retard;
        }
    }
    if (c == 0) {                     // No length constraint
        if (d == 3 && next != NULL) { // Concatenated pattern
            str->p2 = last;
            goto adv0;
        }
        goto advanc;
    }
    // Match with specific length
    while (c--)
        if (ubextend(str, last) == 0)
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
    if (b == NULL)
        return (a);
    list = b;
f1:
    back = list->p2;
    var  = back->p2;
    if (list->typ < TOKEN_ALTERNATION) {
        // Simple pattern - no assignment needed
        delete_string(var);
        goto fadv;
    }
    // Complex pattern - assign matched substring
    str = var->p1;
    etc = var->p2;
    if (a != NULL && etc->p1 != NULL) {
        // Assign matched substring to variable
        if (str->p2 == NULL) {
            free_node(str);
            str = NULL;
        }
        assign(etc->p1, copy(str));
    }
    if (str)
        free_node(str);
    free_node(etc);
    free_node(var);
    goto fadv;
}
