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
    c = s->head;
    if (c == nullptr)
        goto bad;
    b = 0; // Parenthesis balance counter
    d = 0; // Character count
    a = s->tail;
    if (a == nullptr) {
        a = c;
        goto eb2;
    }
eb1:
    if (a == last)
        goto bad;
    a = a->head;
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
        s->tail = a;
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
    a = s->head;
    if (a == nullptr)
        goto bad;
    b = s->tail;
    if (b == nullptr)
        goto good;
    if (b == last)
        goto bad;
    a = b->head;
good:
    s->tail = a;
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
//   d->head: Character node BEFORE the match start
//          - nullptr if match starts at beginning of string
//
//   d->tail: Character node AFTER the match end
//          - Points to the character node immediately after the match
//          - If match ends at end of string: d->tail = r->tail (end marker)
//          - Otherwise: d->tail = next (character node after match)
//
// EXAMPLE: For string "hello world" matching "world":
//   - next = nullptr (match is at end)
//   - d->head = ' ' (space before "world")
//   - d->tail = r->tail (end marker, since match ends at end)
//
Node *SnobolContext::search(const Node &arg, Node *r)
{
    Node *list, *back, *str, *etc, *next, *last, *base, *e;
    Node *a, *b, *var;
    Token c;
    Token d_token;
    int d{}, len;
    Node *before_match_start = nullptr; // Track node before match_start

    // Initialize pattern matching state
    a    = arg.tail;        // Start of pattern component list
    list = base = &alloc(); // Base of matching state list
    last        = nullptr;  // End of subject string (set later)
    next        = nullptr;  // Next position to match from
    goto badv1;
badvanc:
    // Build pattern matching state from pattern components
    a = a->head;
    if (a->typ == Token::TOKEN_END) {
        // End of pattern - initialize search
        list->head = nullptr;
        if (rfail == 1) {
            a = nullptr;
            goto fail;
        }
        list = base;
        if (r == nullptr)
            next = last = nullptr;
        else {
            next               = r->head; // Start of subject string
            last               = r->tail; // End of subject string
            before_match_start = nullptr; // Match starts at beginning, nothing before it
        }
        goto adv1;
    }
    b          = &alloc();
    list->head = b;
    list       = b;
badv1:
    // Set up backtracking structure for this pattern component
    list->tail = back = &alloc();
    back->head        = last;
    b                 = a->tail;
    c                 = a->typ;
    list->typ         = c;
    if (c == Token::TOKEN_ALTERNATION) {
        mes("alternations are not supported yet");
        return nullptr;
    }
    if (c < Token::TOKEN_ALTERNATION) {
        // Simple pattern component - evaluate and store
        back->tail = eval(*b, 1);
        goto badvanc;
    }
    // Complex pattern component - set up match state
    last       = list;
    str        = &alloc(); // Match position tracker
    etc        = &alloc(); // Pattern metadata
    back->tail = var = &alloc();
    var->typ         = b->typ; // Pattern type (1=balanced, 2=unbalanced, 3=concatenated)
    var->head        = str;
    var->tail        = etc;
    e                = b->head;
    if (e == nullptr)
        etc->head = nullptr; // No left side
    else
        etc->head = eval(*e, 0); // Evaluate left pattern
    e = b->tail;
    if (e == nullptr)
        etc->tail = nullptr; // No right side
    else {
        e         = eval(*e, 1);             // Evaluate right pattern (length)
        etc->tail = (Node *)(long)strbin(e); // Store as integer
        delete_string(e);
    }
    goto badvanc;

retard:
    // Backtrack to previous pattern component
    a = back->head;
    if (a == nullptr) {
        // No previous pattern component - try next position in subject for unanchored search
        if (next == nullptr || next == last) {
            rfail = 1;
            goto fail;
        }

        // Advance to next position and try again
        before_match_start = next; // Current next will be before the new match_start
        next               = next->head;
        list               = base;
        goto adv1;
    }
    list = a;
    back = list->tail;
    var  = back->tail;
    str  = var->head;
    etc  = var->tail;
    if (etc->tail) // Has length constraint - need to retry
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
    a = str->tail;
adv01:
    if (a == last)
        next = nullptr;
    else
        next = a->head;
advanc:
    // Process next pattern component
    a = list->head;
    if (a == nullptr) {
        // End of pattern - check if match succeeded
        a = &alloc();
        if (r == nullptr) {
            a->head = a->tail = nullptr;
            goto fail;
        }

        // Set a->head to character BEFORE match start (or nullptr if match at start)
        a->head = before_match_start;

        // Set a->tail to character AFTER match end (or r->tail if match at end)
        if (next == nullptr) {
            // Match ends at end of string
            a->tail = r->tail;
        } else {
            // next points to character after match end
            a->tail = next;
        }
        goto fail;
    }
    list = a;
adv1:
    back    = list->tail;
    var     = back->tail;
    d_token = list->typ;
    if (d_token < Token::TOKEN_ALTERNATION) {
        // Simple pattern - match string directly
        if (var == nullptr)
            goto advanc;
        if (next == nullptr)
            goto retard;
        a = next;
        b = var->head;
        e = var->tail;
        while (1) {
            if (a->ch != b->ch)
                goto retard;
            if (b == e) // Matched entire string
                goto adv01;
            if (a == last) // Reached end of subject
                goto retard;
            a = a->head;
            b = b->head;
        }
    }
    // Complex pattern - handle alternation or concatenation
    str       = var->head;
    etc       = var->tail;
    str->head = next;
    str->tail = nullptr;
    len       = (long)etc->tail;               // Length constraint
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
            str->tail = last;
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
    b = list->head;
    free_node(*list);
    if (b == nullptr)
        return (a);
    list = b;
f1:
    back = list->tail;
    var  = back->tail;
    if (list->typ < Token::TOKEN_ALTERNATION) {
        // Simple pattern - no assignment needed
        delete_string(var);
        goto fadv;
    }
    // Complex pattern - assign matched substring
    str = var->head;
    etc = var->tail;
    if (a != nullptr && etc->head != nullptr) {
        // Assign matched substring to variable
        if (str->tail == nullptr) {
            free_node(*str);
            str = nullptr;
        }
        assign(*etc->head, *copy(str));
    }
    if (str)
        free_node(*str);
    free_node(*etc);
    free_node(*var);
    goto fadv;
}
