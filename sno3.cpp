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
// RETURN VALUE STRUCTURE:
// When a match is found, search() returns a Node* (typically called 'd' in execute())
// with the following structure:
//
//   d->p1: Character node BEFORE the match start
//          - nullptr if match starts at beginning of string
//          - Points to the character node immediately before match_start_pos
//          - Used by replacement logic to determine what comes before the match
//
//   d->p2: Character node AFTER the match end
//          - Points to the character node immediately after the match
//          - If match ends at end of string: d->p2 = r->p2 (end marker)
//          - Otherwise: d->p2 = next (character node after match)
//          - Used by replacement logic to determine what comes after the match
//
// EXAMPLE: For string "hello world" matching "world":
//   - match_start_pos = 'w' (first char of "world")
//   - next = nullptr (match is at end)
//   - d->p1 = ' ' (space before "world")
//   - d->p2 = r->p2 (end marker, since match ends at end)
//
Node *SnobolContext::search(const Node &arg, Node *r)
{
    Node *list, *back, *str, *etc, *next, *last, *base, *e;
    Node *a, *b, *var;
    Token c;
    Token d_token;
    int d{}, len;
    Node *match_start = nullptr; // Track where current match attempt started

    // Initialize pattern matching state
    a    = arg.p2;          // Start of pattern component list
    list = base = &alloc(); // Base of matching state list
    last        = nullptr;  // End of subject string (set later)
    next        = nullptr;  // Next position to match from
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
            next        = r->p1; // Start of subject string
            last        = r->p2; // End of subject string
            match_start = next;  // Track where this match attempt starts
        }
        goto adv1;
    }
    b        = &alloc();
    list->p1 = b;
    list     = b;
badv1:
    // Set up backtracking structure for this pattern component
    list->p2 = back = &alloc();
    back->p1        = last;
    b               = a->p2;
    c               = a->typ;
    list->typ       = c;
    if (static_cast<int>(c) < static_cast<int>(Token::STMT_ASSIGN)) {
        // Simple pattern component - evaluate and store
        back->p2 = eval(*b, 1);
        goto badvanc;
    }
    // Complex pattern component - set up match state
    last     = list;
    str      = &alloc(); // Match position tracker
    etc      = &alloc(); // Pattern metadata
    back->p2 = var = &alloc();
    var->typ       = b->typ; // Pattern type (1=balanced, 2=unbalanced, 3=concatenated)
    var->p1        = str;
    var->p2        = etc;
    e              = b->p1;
    if (e == nullptr)
        etc->p1 = nullptr; // No left side
    else
        etc->p1 = eval(*e, 0); // Evaluate left pattern
    e = b->p2;
    if (e == nullptr)
        etc->p2 = nullptr; // No right side
    else {
        e       = eval(*e, 1);             // Evaluate right pattern (length)
        etc->p2 = (Node *)(long)strbin(e); // Store as integer
        delete_string(e);
    }
    goto badvanc;

retard:
    // Backtrack to previous pattern component
    a = back->p1;
    if (a == nullptr) {
        // No previous pattern component - try next position in subject for unanchored search
        if (next == nullptr || next == last) {
            rfail = 1;
            goto fail;
        }
        // Advance to next position and try again
        next        = next->p1;
        match_start = next; // Reset match start for new attempt
        list        = base;
        goto adv1;
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
        a = &alloc();
        if (r == nullptr) {
            a->p1 = a->p2 = nullptr;
            goto fail;
        }
        // Set a->p1 to character BEFORE match start (or nullptr if match at start)
        if (match_start == r->p1) {
            // Match starts at beginning of string
            a->p1 = nullptr;
        } else {
            // Find character before match_start
            b = r->p1;
            while (b != nullptr && b->p1 != match_start && b != r->p2) {
                b = b->p1;
            }
            if (b != nullptr && b->p1 == match_start) {
                a->p1 = b; // Character before match start
            } else {
                a->p1 = nullptr; // Fallback
            }
        }
        // Set a->p2 to character AFTER match end (or r->p2 if match at end)
        if (next == nullptr) {
            // Match ends at end of string
            a->p2 = r->p2;
        } else {
            // next points to character after match end
            a->p2 = next;
        }
        goto fail;
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
        // Track where this match attempt starts (for first pattern component)
        if (match_start == nullptr || (list == base->p1 && match_start != next)) {
            match_start = next;
        }
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
    len     = (long)etc->p2;                   // Length constraint
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
    free_node(*back);
    b = list->p1;
    free_node(*list);
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
            free_node(*str);
            str = nullptr;
        }
        assign(*etc->p1, *copy(str));
    }
    if (str)
        free_node(*str);
    free_node(*etc);
    free_node(*var);
    goto fadv;
}
