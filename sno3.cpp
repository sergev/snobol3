#include "sno.h"
#include <fstream>
#include <ctime>

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
//          - Special case: if match is at end, d->p1 = r->p2 (end marker)
//
//   d->p2: Character node AFTER the match end
//          - Points to the character node immediately after the match
//          - If match ends at end of string: d->p2 = r->p2 (end marker)
//          - Otherwise: d->p2 = next (character node after match)
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
    Node *match_start_pos = nullptr; // Track where current match attempt started

    // #region agent log
    {
        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            log_file << "{\"id\":\"log_search_" << now << "\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:85\",\"message\":\"search() called\",\"data\":{\"argP2IsNull\":" << (arg.p2 == nullptr ? 1 : 0) << ",\"rIsNull\":" << (r == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            if (arg.p2) {
                int componentCount = 0;
                Node* comp = arg.p2;
                while (comp && comp->typ != Token::TOKEN_END) {
                    componentCount++;
                    comp = comp->p1;
                }
                log_file << "{\"id\":\"log_search_" << now << "_2\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:85\",\"message\":\"Pattern component count\",\"data\":{\"componentCount\":" << componentCount << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            }
            log_file.close();
        }
    }
    // #endregion agent log
    // Initialize pattern matching state
    a    = arg.p2;          // Start of pattern component list
    list = base = &alloc(); // Base of matching state list
    last        = nullptr;  // End of subject string (set later)
    next        = nullptr;  // Next position to match from
    goto badv1;
badvanc:
    // Build pattern matching state from pattern components
    a = a->p1;
    // #region agent log
    {
        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            log_file << "{\"id\":\"log_search_" << now << "_3\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:102\",\"message\":\"Processing pattern component\",\"data\":{\"aIsNull\":" << (a == nullptr ? 1 : 0) << ",\"aTyp\":" << (a ? static_cast<int>(a->typ) : -1) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            log_file.close();
        }
    }
    // #endregion agent log
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
            match_start_pos = next; // Track initial match start position
        }
        // #region agent log
        {
            std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                log_file << "{\"id\":\"log_search_" << now << "_4\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:116\",\"message\":\"Initializing search\",\"data\":{\"nextIsNull\":" << (next == nullptr ? 1 : 0) << ",\"lastIsNull\":" << (last == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                log_file.close();
            }
        }
        // #endregion agent log
        goto adv1;
    }
    b        = &alloc();
    list->p1 = b;
    list     = b;
badv1:
    // Set up backtracking structure for this pattern component
    // #region agent log
    {
        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            log_file << "{\"id\":\"log_search_" << now << "_badv1\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:badv1\",\"message\":\"Setting up pattern component\",\"data\":{\"aIsNull\":" << (a == nullptr ? 1 : 0) << ",\"aTyp\":" << (a ? static_cast<int>(a->typ) : -1) << ",\"aP2IsNull\":" << (a && a->p2 == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            log_file.close();
        }
    }
    // #endregion agent log
    list->p2 = back = &alloc();
    back->p1        = last;
    b               = a->p2;
    c               = a->typ;
    list->typ       = c;
    // #region agent log
    {
        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            log_file << "{\"id\":\"log_search_" << now << "_5\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:127\",\"message\":\"Pattern component type\",\"data\":{\"c\":" << static_cast<int>(c) << ",\"isSimple\":" << (static_cast<int>(c) < static_cast<int>(Token::STMT_ASSIGN) ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            log_file.close();
        }
    }
    // #endregion agent log
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
    // #region agent log
    {
        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            log_file << "{\"id\":\"log_search_" << now << "_retard\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:retard\",\"message\":\"Backtracking\",\"data\":{},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            log_file.close();
        }
    }
    // #endregion agent log
    a = back->p1;
    if (a == nullptr) {
        // No previous pattern component - try next position in subject for unanchored search
        // #region agent log
        {
            std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                log_file << "{\"id\":\"log_search_" << now << "_no_prev\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:retard\",\"message\":\"No previous component, trying next position\",\"data\":{\"nextIsNull\":" << (next == nullptr ? 1 : 0) << ",\"lastIsNull\":" << (last == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                log_file.close();
            }
        }
        // #endregion agent log
        if (next == nullptr || next == last) {
            rfail = 1;
            goto fail;
        }
        // Advance to next position and try again
        next = next->p1;
        match_start_pos = nullptr; // Reset - will be set when we start new match attempt
        list = base;
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
        // #region agent log
        {
            std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                log_file << "{\"id\":\"log_search_" << now << "_6\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:189\",\"message\":\"End of pattern reached\",\"data\":{\"rIsNull\":" << (r == nullptr ? 1 : 0) << ",\"nextIsNull\":" << (next == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                log_file.close();
            }
        }
        // #endregion agent log
        if (r == nullptr) {
            a->p1 = a->p2 = nullptr;
            goto fail;
        }
        // For simple patterns: use original logic
        // a->p1 = character before match start (or nullptr if match at start)
        // a->p2 = character after match (next, or r->p2 if match at end)
        // #region agent log
        {
            std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                log_file << "{\"id\":\"log_search_" << now << "_find_match_start\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:find_match_start\",\"message\":\"Finding match start position\",\"data\":{\"matchStartPosIsNull\":" << (match_start_pos == nullptr ? 1 : 0) << ",\"matchStartPosEqR1\":" << (match_start_pos == r->p1 ? 1 : 0) << ",\"nextIsNull\":" << (next == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                log_file.close();
            }
        }
        // #endregion agent log
        b = r->p1;
        if (match_start_pos != nullptr && match_start_pos != r->p1) {
            // Match not at start - find character before match_start_pos
            Node* prev = nullptr;
            int safety = 0;
            // #region agent log
            {
                std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
                if (log_file.is_open()) {
                    auto now = std::time(nullptr);
                    char matchStartCh = match_start_pos ? match_start_pos->ch : 0;
                    log_file << "{\"id\":\"log_search_" << now << "_loop_start\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:loop_start\",\"message\":\"Starting loop to find prev\",\"data\":{\"matchStartCh\":" << static_cast<int>(matchStartCh) << ",\"matchStartAddr\":" << reinterpret_cast<uintptr_t>(match_start_pos) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                    log_file.close();
                }
            }
            // #endregion agent log
            while (b != match_start_pos && b != nullptr && b != r->p2 && safety < 10000) {
                prev = b;
                b = b->p1;
                safety++;
                if (safety % 1000 == 0) {
                    // #region agent log
                    {
                        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
                        if (log_file.is_open()) {
                            auto now = std::time(nullptr);
                            log_file << "{\"id\":\"log_search_" << now << "_loop_iter\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:loop_iter\",\"message\":\"Loop iteration\",\"data\":{\"safety\":" << safety << ",\"bIsNull\":" << (b == nullptr ? 1 : 0) << ",\"bEqR2\":" << (b == r->p2 ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                            log_file.close();
                        }
                    }
                    // #endregion agent log
                }
            }
            // #region agent log
            {
                std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
                if (log_file.is_open()) {
                    auto now = std::time(nullptr);
                    char prevCh = prev ? prev->ch : 0;
                    char bCh = b ? b->ch : 0;
                    log_file << "{\"id\":\"log_search_" << now << "_loop_end\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:loop_end\",\"message\":\"Loop ended\",\"data\":{\"safety\":" << safety << ",\"bEqMatchStart\":" << (b == match_start_pos ? 1 : 0) << ",\"prevIsNull\":" << (prev == nullptr ? 1 : 0) << ",\"prevCh\":" << static_cast<int>(prevCh) << ",\"bCh\":" << static_cast<int>(bCh) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                    log_file.close();
                }
            }
            // #endregion agent log
            if (b == match_start_pos && prev != nullptr) {
                a->p1 = prev;
                // #region agent log
                {
                    std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
                    if (log_file.is_open()) {
                        auto now = std::time(nullptr);
                        char aP1Ch = a->p1 ? a->p1->ch : 0;
                        log_file << "{\"id\":\"log_search_" << now << "_set_p1\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:set_p1\",\"message\":\"Set a->p1\",\"data\":{\"aP1Ch\":" << static_cast<int>(aP1Ch) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                        log_file.close();
                    }
                }
                // #endregion agent log
            } else {
                a->p1 = nullptr; // Fallback
            }
        } else {
            a->p1 = nullptr; // Match at start
        }
        // a->p2 should point to character after match, or end marker if match at end
        if (next == nullptr) {
            // Match at end - a->p2 should point to end marker
            a->p2 = r->p2;
        } else {
            a->p2 = next; // Character after match
        }
        goto fail;
    }
    list = a;
adv1:
    back    = list->p2;
    var     = back->p2;
    d_token = list->typ;
    // #region agent log
    {
        std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
        if (log_file.is_open()) {
            auto now = std::time(nullptr);
            log_file << "{\"id\":\"log_search_" << now << "_adv1\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:adv1\",\"message\":\"Starting matching phase\",\"data\":{\"d_token\":" << static_cast<int>(d_token) << ",\"isSimple\":" << (static_cast<int>(d_token) < 2 ? 1 : 0) << ",\"nextIsNull\":" << (next == nullptr ? 1 : 0) << ",\"lastIsNull\":" << (last == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
            log_file.close();
        }
    }
    // #endregion agent log
    if (static_cast<int>(d_token) < 2) {
        // Simple pattern - match string directly
        // Track where this match attempt starts
        if (match_start_pos == nullptr || match_start_pos != next) {
            match_start_pos = next; // Record where we're starting this match attempt
        }
        // #region agent log
        {
            std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                log_file << "{\"id\":\"log_search_" << now << "_simple\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:simple\",\"message\":\"Matching simple pattern\",\"data\":{\"varIsNull\":" << (var == nullptr ? 1 : 0) << ",\"nextIsNull\":" << (next == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                log_file.close();
            }
        }
        // #endregion agent log
        if (var == nullptr)
            goto advanc;
        if (next == nullptr)
            goto retard;
        a = next;
        b = var->p1;
        e = var->p2;
        // #region agent log
        {
            std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
            if (log_file.is_open()) {
                auto now = std::time(nullptr);
                int patternLen = 0;
                Node* tmp = b;
                while (tmp && tmp != e) {
                    patternLen++;
                    tmp = tmp->p1;
                }
                log_file << "{\"id\":\"log_search_" << now << "_match_start\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:match_loop\",\"message\":\"Starting character matching\",\"data\":{\"patternLen\":" << patternLen << ",\"bIsNull\":" << (b == nullptr ? 1 : 0) << ",\"eIsNull\":" << (e == nullptr ? 1 : 0) << ",\"aIsNull\":" << (a == nullptr ? 1 : 0) << ",\"lastIsNull\":" << (last == nullptr ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                log_file.close();
            }
        }
        // #endregion agent log
        while (1) {
            // #region agent log
            {
                std::ofstream log_file("/Users/vak/Project/Cursor/snobol3/.cursor/debug.log", std::ios::app);
                if (log_file.is_open()) {
                    auto now = std::time(nullptr);
                    log_file << "{\"id\":\"log_search_" << now << "_char\",\"timestamp\":" << (now * 1000) << ",\"location\":\"sno3.cpp:match_loop\",\"message\":\"Comparing characters\",\"data\":{\"aCh\":" << (a ? static_cast<int>(a->ch) : -1) << ",\"bCh\":" << (b ? static_cast<int>(b->ch) : -1) << ",\"bEqE\":" << (b == e ? 1 : 0) << ",\"aEqLast\":" << (a == last ? 1 : 0) << "},\"sessionId\":\"debug-session\",\"runId\":\"post-fix\"}\n";
                    log_file.close();
                }
            }
            // #endregion agent log
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
