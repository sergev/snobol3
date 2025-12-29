//
// Snobol III
//
#include <cstdint>
#include <iomanip>
#include <ios>

#include "sno.h"

//
// Constructor - initialize all fields including stream references
//
SnobolContext::SnobolContext(std::ostream &output) : fin(&std::cin), fout(output)
{
    // Initialize built-in symbols
    lookf     = &init("f", Token::EXPR_VAR_REF);
    looks     = &init("s", Token::EXPR_VAR_REF);
    lookend   = &init("end", Token::EXPR_VAR_REF);
    lookstart = &init("start", Token::EXPR_VAR_REF);
    lookdef   = &init("define", Token::EXPR_VAR_REF);
    lookret   = &init("return", Token::EXPR_VAR_REF);
    lookfret  = &init("freturn", Token::EXPR_VAR_REF);
    init("syspit", Token::EXPR_SYSPIT);
    init("syspot", Token::EXPR_SYSPOT);
}

void SnobolContext::compile_program(std::istream &input)
{
    Node *cur, *next;

    // Compile all statements until "end" is encountered
    // Link statements together in a list
    fin     = &input;
    program = compile();
    for (cur = program; lookend->typ != Token::EXPR_LABEL; cur = next) {
        next      = compile();
        cur->head = next;
    }
    cur->head = nullptr; // Terminate statement list
    cfail     = 1;       // Enable compilation failure mode
    fin       = &std::cin;
}

void SnobolContext::execute_program(std::istream &input)
{
    Node *c = program;

    // Start execution from "start" label if defined, otherwise from first statement
    if (lookstart->typ == Token::EXPR_LABEL)
        c = lookstart->tail;

    if (!c) {
        // Nothing to run.
        return;
    }

    fin = &input;
    while ((c = execute(*c)) != nullptr)
        ;
    flush();
    fin = &std::cin;
}

//
// Print a message string to output.
//
void SnobolContext::mes(const char *s)
{
    sysput(&cstr_to_node(s));
}

//
// Initialize a symbol in the name table with a given type.
// Creates a node from the string, looks it up (or creates it), and sets its type.
//
Node &SnobolContext::init(const char *s, Token t)
{
    Node &a = cstr_to_node(s);
    Node &b = look(a);
    delete_string(&a);
    b.typ = t;
    return b;
}

//
// System function to read a line from input (syspit).
// Reads characters until newline or EOF, returns a string node or NULL on failure.
//
Node *SnobolContext::syspit()
{
    Node *b, *c, *d;
    int a;

    a = fin->get();
    if (a == '\n')
        return (nullptr);
    if (fin->eof() || a == std::char_traits<char>::eof()) {
        rfail = 1;
        return (nullptr);
    }
    b = c = &alloc();
    while (a != '\n') {
        d       = &alloc();
        c->head = d;
        c       = d;
        c->ch   = a;
        a       = fin->get();
        if (fin->eof() || a == std::char_traits<char>::eof()) {
            rfail = 1;
            break;
        }
    }
    b->tail = c;
    if (rfail) {
        delete_string(b);
        b = nullptr;
    }
    return (b);
}

//
// System function to write a string to output (syspot).
// Outputs the string followed by a newline character.
//
void SnobolContext::syspot(Node *string) const
{
    Node *a, *b, *s;

    s = string;
    if (s != nullptr) {
        a = s;
        b = s->tail;
        while (a != b) {
            a = a->head;
            fout.put(a->ch);
        }
    }
    fout.put('\n');
}

//
// Convert a C string to a Snobol string node.
// Creates a linked list of nodes representing the string characters.
//
Node &SnobolContext::cstr_to_node(const char *s)
{
    int c;
    Node *e{}, *f;
    Node &d = alloc();

    // Build linked list: d is head, f tracks tail, e is new node
    f = &d;
    while ((c = *s++) != '\0') {
        e       = &alloc();
        e->ch   = c;
        f->head = e;
        f       = e;
    }
    d.tail = e; // Store end marker in head node
    return d;
}

//
// Classify a character for lexical analysis.
// Returns a numeric code representing the character's syntactic role.
//
CharClass SnobolContext::char_class(int c)
{
    switch (c) {
    case ')':
        return (CharClass::RPAREN); // Right parenthesis
    case '(':
        return (CharClass::LPAREN); // Left parenthesis
    case '\t':
    case ' ':
        return (CharClass::WHITESPACE); // Whitespace
    case '+':
        return (CharClass::PLUS); // Plus operator
    case '-':
        return (CharClass::MINUS); // Minus operator
    case '*':
        return (CharClass::ASTERISK); // Asterisk operator
    case '/':
        return (CharClass::SLASH); // Division operator
    case '$':
        return (CharClass::DOLLAR); // Dollar sign
    case '"':
    case '\'':
        return (CharClass::STRING_DELIM); // String delimiter
    case '=':
        return (CharClass::EQUALS); // Equals sign
    case ',':
        return (CharClass::COMMA); // Comma
    }
    return (CharClass::OTHER); // Other character
}

//
// Allocate a new node from the memory pool.
// Uses a free list if available, otherwise allocates from the current memory block.
// Allocates a new block of 200 nodes when the current block is exhausted.
//
Node &SnobolContext::alloc()
{
    Node *f;

    if (freelist == nullptr) {
        // Allocate another node block and add it to the free list.
        mem_pool.push_back(std::make_unique<NodeBlock>());
        auto &block = *mem_pool.back();
        for (auto &item : block) {
            free_node(item);
        }
        freesize = BLOCK_SIZE;
    }

    // Reuse node from free list
    f        = freelist;
    freelist = freelist->head;
    return *f;
}

//
// Free a node by adding it to the free list for reuse.
//
void SnobolContext::free_node(Node &pointer)
{
    pointer.head = freelist;
    freelist     = &pointer;
}

//
// Look up a symbol in the name table, creating it if it doesn't exist.
// Returns a reference to the symbol's value node.
//
Node &SnobolContext::look(const Node &string)
{
    Node *i, *j, *k;

    k = nullptr;
    i = namelist;
    // Search existing symbols
    while (i) {
        j = i->head;
        if (j->head->equal(&string) == 0)
            return *j;
        i = (k = i)->tail;
    }
    // Symbol not found, create new entry
    i       = &alloc();
    i->tail = nullptr;
    if (k)
        k->tail = i;
    else
        namelist = i;
    j       = &alloc();
    i->head = j;
    j->head = copy(&string);
    j->tail = nullptr;
    j->typ  = Token::EXPR_VAR_REF;
    return *j;
}

//
// Create a copy of a string node.
// Allocates new nodes and copies all characters from the source string.
//
Node *SnobolContext::copy(const Node *string)
{
    Node *l, *m;
    Node *i, *k;
    const Node *j_src;

    if (string == nullptr)
        return (nullptr);
    i = l = &alloc();
    j_src = string;
    k     = const_cast<Node *>(string->tail);
    while (j_src != k) {
        m       = &alloc();
        m->ch   = (j_src = j_src->head)->ch;
        l->head = m;
        l       = m;
    }
    i->tail = l;
    return (i);
}

//
// Compare two strings lexicographically.
// Returns 0 if equal, 1 if string1 > string2, -1 if string1 < string2.
//
int Node::equal(const Node *string2) const
{
    const Node *i, *j, *k;
    const Node *l;
    int n, m;

    if (string2 == nullptr)
        return (1);

    // Compare character by character
    i = this;
    j = this->tail; // End marker for string1
    k = string2;
    l = string2->tail; // End marker for string2
    for (;;) {
        m = (i = i->head)->ch; // Next char from string1
        n = (k = k->head)->ch; // Next char from string2
        if (m > n)
            return (1);
        if (m < n)
            return (-1);
        if (i == j) {
            if (k == l)
                return (0);
            return (-1);
        }
        if (k == l)
            return (1);
    }
}

//
// Debug print a compact representation of the node tree.
// Prints token type and structure, handling cycles and depth limits.
//
void Node::debug_print(std::ostream &os, int depth, int max_depth) const
{
    if (depth > max_depth) {
        os << "...";
        return;
    }

    // Print indentation
    for (int i = 0; i < depth; i++) {
        os << "  ";
    }

    // Print token type name (use if-else since some tokens share enum values)
    int typ_val = static_cast<int>(typ);
    if (typ == Token::TOKEN_END || typ == Token::EXPR_VAR_REF || typ == Token::STMT_SIMPLE) {
        if (typ == Token::TOKEN_END)
            os << "END";
        else if (typ == Token::EXPR_VAR_REF)
            os << "VAR_REF";
        else
            os << "STMT_SIMPLE";
    } else if (typ == Token::EXPR_VALUE || typ == Token::TOKEN_UNANCHORED ||
               typ == Token::STMT_MATCH) {
        if (typ == Token::EXPR_VALUE)
            os << "VALUE";
        else if (typ == Token::TOKEN_UNANCHORED)
            os << "UNANCHORED";
        else
            os << "STMT_MATCH";
    } else if (typ == Token::EXPR_LABEL || typ == Token::TOKEN_ALTERNATION ||
               typ == Token::STMT_ASSIGN) {
        if (typ == Token::EXPR_LABEL)
            os << "LABEL";
        else if (typ == Token::TOKEN_ALTERNATION)
            os << "ALTERNATION";
        else
            os << "STMT_ASSIGN";
    } else if (typ == Token::EXPR_SYSPIT || typ == Token::TOKEN_EQUALS ||
               typ == Token::STMT_REPLACE) {
        if (typ == Token::EXPR_SYSPIT)
            os << "SYSPIT";
        else if (typ == Token::TOKEN_EQUALS)
            os << "EQUALS";
        else
            os << "STMT_REPLACE";
    } else if (typ == Token::EXPR_SYSPOT || typ == Token::TOKEN_COMMA) {
        if (typ == Token::EXPR_SYSPOT)
            os << "SYSPOT";
        else
            os << "COMMA";
    } else if (typ == Token::EXPR_FUNCTION || typ == Token::TOKEN_RPAREN) {
        if (typ == Token::EXPR_FUNCTION)
            os << "FUNCTION";
        else
            os << "RPAREN";
    } else if (typ == Token::TOKEN_MARKER) {
        os << "MARKER";
    } else if (typ == Token::TOKEN_WHITESPACE) {
        os << "WHITESPACE";
    } else if (typ == Token::TOKEN_PLUS) {
        os << "PLUS";
    } else if (typ == Token::TOKEN_MINUS) {
        os << "MINUS";
    } else if (typ == Token::TOKEN_MULT) {
        os << "MULT";
    } else if (typ == Token::TOKEN_DIV) {
        os << "DIV";
    } else if (typ == Token::TOKEN_DOLLAR) {
        os << "DOLLAR";
    } else if (typ == Token::TOKEN_CALL) {
        os << "CALL";
    } else if (typ == Token::TOKEN_VARIABLE) {
        os << "VARIABLE";
    } else if (typ == Token::TOKEN_STRING) {
        os << "STRING";
    } else if (typ == Token::TOKEN_LPAREN) {
        os << "LPAREN";
    } else {
        os << "UNKNOWN(" << typ_val << ")";
    }

    // Print character if it's a printable character node
    if (ch >= 32 && ch < 127 && (typ == Token::TOKEN_STRING || ch != 0)) {
        os << " '" << ch << "'";
    }

    os << "\n";

    // Print head subtree
    if (head != nullptr) {
        for (int i = 0; i < depth; i++) {
            os << "  ";
        }
        os << "  head-> ";
        head->debug_print(os, depth + 1, max_depth);
    } else {
        for (int i = 0; i < depth; i++) {
            os << "  ";
        }
        os << "  head-> NULL\n";
    }

    // Print tail subtree
    if (tail != nullptr) {
        for (int i = 0; i < depth; i++) {
            os << "  ";
        }
        os << "  tail-> ";
        tail->debug_print(os, depth + 1, max_depth);
    } else {
        for (int i = 0; i < depth; i++) {
            os << "  ";
        }
        os << "  tail-> NULL\n";
    }
}

//
// Convert a string node representing a number to an integer.
// Handles negative numbers and validates digit characters.
//
int SnobolContext::strbin(const Node *string)
{
    int n, m, sign;
    const Node *p, *q, *s;

    s = string;
    n = 0;
    if (s == nullptr)
        return (0);
    p    = s->head;
    q    = s->tail;
    sign = 1;
    if (char_class(p->ch) == CharClass::MINUS) { // minus
        sign = -1;
        if (p == q)
            return (0);
        p = p->head;
    }
loop:
    m = p->ch - '0';
    if (m > 9 || m < 0)
        writes("bad integer string");
    n = n * 10 + m;
    if (p == q)
        return (n * sign);
    p = p->head;
    goto loop;
}

//
// Convert an integer to a string node.
// Builds the string representation digit by digit, handling negative numbers.
//
Node &SnobolContext::binstr(int binary)
{
    int n, sign;
    Node *m, *q;
    Node &p = alloc();

    n    = binary;
    q    = &alloc();
    sign = 1;
    if (binary < 0) {
        sign = -1;
        n    = -binary;
    }
    p.tail = q;
loop:
    q->ch = n % 10 + '0';
    n     = n / 10;
    if (n == 0) {
        if (sign < 0) {
            m       = &alloc();
            m->head = q;
            q       = m;
            q->ch   = '-';
        }
        p.head = q;
        return p;
    }
    m       = &alloc();
    m->head = q;
    q       = m;
    goto loop;
}

//
// Add two numeric strings and return the result as a string.
//
Node &SnobolContext::add(const Node &string1, const Node &string2)
{
    return binstr(strbin(&string1) + strbin(&string2));
}

//
// Subtract two numeric strings and return the result as a string.
//
Node &SnobolContext::sub(const Node &string1, const Node &string2)
{
    return binstr(strbin(&string1) - strbin(&string2));
}

//
// Multiply two numeric strings and return the result as a string.
//
Node &SnobolContext::mult(const Node &string1, const Node &string2)
{
    return binstr(strbin(&string1) * strbin(&string2));
}

//
// Divide two numeric strings and return the result as a string.
//
Node &SnobolContext::divide(const Node &string1, const Node &string2)
{
    return binstr(strbin(&string1) / strbin(&string2));
}

//
// Concatenate two strings, creating new copies.
// Returns a new string node containing the concatenation.
//
Node *SnobolContext::cat(const Node *string1, const Node *string2)
{
    Node *a, *b;

    if (string1 == nullptr)
        return (copy(string2));
    if (string2 == nullptr)
        return (copy(string1));
    a             = copy(string1);
    b             = copy(string2);
    a->tail->head = b->head;
    a->tail       = b->tail;
    free_node(*b);
    return (a);
}

//
// Concatenate two strings and delete the original strings (destructive concatenation).
// Used when the original strings are no longer needed.
//
Node *SnobolContext::dcat(Node &a, Node &b)
{
    Node *c;

    c = cat(&a, &b);
    delete_string(&a);
    delete_string(&b);
    return (c);
}

//
// Delete a string by freeing all its component nodes.
// Traverses the linked list and returns each node to the free list.
//
void SnobolContext::delete_string(Node *string)
{
    Node *a, *b, *c;

    if (string == nullptr)
        return;
    a = string;
    b = string->tail;
    while (a != b) {
        c = a->head;
        free_node(*a);
        a = c;
    }
    free_node(*a);
}

//
// Output a string and then delete it (system put with cleanup).
//
void SnobolContext::sysput(Node *string)
{
    syspot(string);
    delete_string(string);
}

//
// Dump the entire symbol table for debugging.
//
void SnobolContext::dump()
{
    dump1(namelist);
}

//
// Recursively dump symbol table entries starting from a base node.
// Outputs symbol names, types, and values for debugging purposes.
//
void SnobolContext::dump1(Node *base)
{
    Node *b, *c, *e;
    Node *d;

    while (base) {
        b = base->head;
        c = &binstr(static_cast<int>(b->typ));
        d = &cstr_to_node("  ");
        e = dcat(*c, *d);
        sysput(cat(e, b->head));
        delete_string(e);
        if (b->typ == Token::EXPR_VALUE) {
            c = &cstr_to_node("   ");
            sysput(cat(c, b->tail));
            delete_string(c);
        }
        base = base->tail;
    }
}

//
// Write an error message with line number and handle error recovery.
// If compilation is in progress, skips to end of statement and recompiles.
//
void SnobolContext::writes(const char *s)
{
    Node &n1 = cstr_to_node(s);
    Node &n2 = cstr_to_node("\t");
    Node *n3 = dcat(n2, n1);
    Node &n4 = binstr(lc);
    sysput(dcat(n4, *n3));
    flush();
    if (cfail) {
        dump();
        flush();
        std::exit(1);
    }
    // Error recovery: skip to end of current statement
    while (getc_char() != nullptr)
        ;
    while (compile() != nullptr)
        ;
    flush();
    std::exit(1);
}

//
// Get the next character from the current input line.
// Reads a new line when the current one is exhausted.
// Returns NULL at end of line (after all characters have been consumed).
//
Node *SnobolContext::getc_char()
{
    Node *a;

    while (current_line == nullptr) {
        current_line = syspit();
        if (rfail) {
            cfail++;
            writes("eof on input");
        }
        lc++;
    }
    if (line_flag) {
        current_line = nullptr;
        line_flag    = 0;
        return (nullptr);
    }
    a = current_line->head;
    if (a == current_line->tail) {
        free_node(*current_line);
        line_flag++;
    } else
        current_line->head = a->head;
    return (a);
}

//
// Flush the output buffer.
//
void SnobolContext::flush() const
{
    fout.flush();
}
