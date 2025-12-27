//
// Snobol III
//
#include "sno.h"

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

    a = fin.get();
    if (a == '\n')
        return (nullptr);
    if (fin.eof() || a == std::char_traits<char>::eof()) {
        rfail = 1;
        return (nullptr);
    }
    b = c = &alloc();
    while (a != '\n') {
        d     = &alloc();
        c->p1 = d;
        c     = d;
        c->ch = a;
        a     = fin.get();
        if (fin.eof() || a == std::char_traits<char>::eof()) {
            rfail = 1;
            break;
        }
    }
    b->p2 = c;
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
        b = s->p2;
        while (a != b) {
            a = a->p1;
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
        e     = &alloc();
        e->ch = c;
        f->p1 = e;
        f     = e;
    }
    d.p2 = e; // Store end marker in head node
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
    size_t alloc_size;

    if (freelist == nullptr) {
        if (freespace_current == nullptr || freespace_current >= freespace_end) {
            alloc_size = 200 * sizeof(Node);
            if (freespace == nullptr) {
                freespace = (Node *)malloc(alloc_size);
                if (freespace == nullptr) {
                    flush();
                    fout << "Out of free space\n";
                    std::exit(1);
                }
                freespace_current = freespace;
                freespace_end     = freespace + 200;
                freesize          = 200;
            } else {
                // Allocate new block and append
                Node *new_block = (Node *)malloc(alloc_size);
                if (new_block == nullptr) {
                    flush();
                    fout << "Out of free space\n";
                    std::exit(1);
                }
                freespace_current = new_block;
                freespace_end     = new_block + 200;
                freesize          = 200;
            }
        }
        f = freespace_current++;
        freesize--;
        return *f;
    }
    // Reuse node from free list
    f        = freelist;
    freelist = freelist->p1;
    return *f;
}

//
// Free a node by adding it to the free list for reuse.
//
void SnobolContext::free_node(Node &pointer)
{
    pointer.p1 = freelist;
    freelist   = &pointer;
}

//
// Count the number of free nodes available (both in current block and free list).
//
int SnobolContext::nfree() const
{
    int i;
    Node *a;

    i = freesize;
    a = freelist;
    while (a) {
        a = a->p1;
        i++;
    }
    return (i);
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
        j = i->p1;
        if (j->p1->equal(&string) == 0)
            return *j;
        i = (k = i)->p2;
    }
    // Symbol not found, create new entry
    i     = &alloc();
    i->p2 = nullptr;
    if (k)
        k->p2 = i;
    else
        namelist = i;
    j      = &alloc();
    i->p1  = j;
    j->p1  = copy(&string);
    j->p2  = nullptr;
    j->typ = Token::EXPR_VAR_REF;
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
    k     = const_cast<Node *>(string->p2);
    while (j_src != k) {
        m     = &alloc();
        m->ch = (j_src = j_src->p1)->ch;
        l->p1 = m;
        l     = m;
    }
    i->p2 = l;
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
    j = this->p2; // End marker for string1
    k = string2;
    l = string2->p2; // End marker for string2
    for (;;) {
        m = (i = i->p1)->ch; // Next char from string1
        n = (k = k->p1)->ch; // Next char from string2
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
    p    = s->p1;
    q    = s->p2;
    sign = 1;
    if (char_class(p->ch) == CharClass::MINUS) { // minus
        sign = -1;
        if (p == q)
            return (0);
        p = p->p1;
    }
loop:
    m = p->ch - '0';
    if (m > 9 || m < 0)
        writes("bad integer string");
    n = n * 10 + m;
    if (p == q)
        return (n * sign);
    p = p->p1;
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
    p.p2 = q;
loop:
    q->ch = n % 10 + '0';
    n     = n / 10;
    if (n == 0) {
        if (sign < 0) {
            m     = &alloc();
            m->p1 = q;
            q     = m;
            q->ch = '-';
        }
        p.p1 = q;
        return p;
    }
    m     = &alloc();
    m->p1 = q;
    q     = m;
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
    a         = copy(string1);
    b         = copy(string2);
    a->p2->p1 = b->p1;
    a->p2     = b->p2;
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
    b = string->p2;
    while (a != b) {
        c = a->p1;
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
        b = base->p1;
        c = &binstr(static_cast<int>(b->typ));
        d = &cstr_to_node("  ");
        e = dcat(*c, *d);
        sysput(cat(e, b->p1));
        delete_string(e);
        if (b->typ == Token::EXPR_VALUE) {
            c = &cstr_to_node("   ");
            sysput(cat(c, b->p2));
            delete_string(c);
        }
        base = base->p2;
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
    a = current_line->p1;
    if (a == current_line->p2) {
        free_node(*current_line);
        line_flag++;
    } else
        current_line->p1 = a->p1;
    return (a);
}

//
// Flush the output buffer.
//
void SnobolContext::flush() const
{
    fout.flush();
}
