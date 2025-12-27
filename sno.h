#ifndef SNO_H
#define SNO_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

//
// Character class enumeration for lexical analysis
//
typedef enum {
    CHAR_CLASS_OTHER        = 0,  // Default/unclassified character
    CHAR_CLASS_RPAREN       = 1,  // Right parenthesis ')'
    CHAR_CLASS_LPAREN       = 2,  // Left parenthesis '('
    CHAR_CLASS_WHITESPACE   = 3,  // Whitespace (space, tab)
    CHAR_CLASS_PLUS         = 4,  // Plus operator '+'
    CHAR_CLASS_MINUS        = 5,  // Minus operator '-'
    CHAR_CLASS_ASTERISK     = 6,  // Asterisk operator '*'
    CHAR_CLASS_SLASH        = 7,  // Division operator '/'
    CHAR_CLASS_DOLLAR       = 8,  // Dollar sign '$'
    CHAR_CLASS_STRING_DELIM = 9,  // String delimiter '"' or '\''
    CHAR_CLASS_EQUALS       = 10, // Equals sign '='
    CHAR_CLASS_COMMA        = 11  // Comma ','
} char_class_t;

//
// Token type enumeration for node types
// Uses TOKEN_* prefix for lexical/parsing operations
// Uses EXPR_* prefix for runtime/evaluation operations
// Uses STMT_* prefix for statement types
//
typedef enum {
    // Value 0: End marker / Variable reference / Simple statement
    TOKEN_END    = 0,
    EXPR_VAR_REF = 0,
    STMT_SIMPLE  = 0,

    // Value 1: Unanchored search / Value / Pattern matching statement
    TOKEN_UNANCHORED = 1,
    EXPR_VALUE       = 1,
    STMT_MATCH       = 1,

    // Value 2: Pattern alternation / Label / Assignment statement
    TOKEN_ALTERNATION = 2,
    EXPR_LABEL        = 2,
    STMT_ASSIGN       = 2,

    // Value 3: Equals / System input function
    TOKEN_EQUALS = 3,
    EXPR_SYSPIT  = 3,

    // Value 4: Comma / System output
    TOKEN_COMMA = 4,
    EXPR_SYSPOT = 4,

    // Value 5: Right parenthesis / Function
    TOKEN_RPAREN  = 5,
    EXPR_FUNCTION = 5,

    // Value 6: Special value / Free space
    EXPR_SPECIAL = 6,

    // Value 7: Whitespace / Concatenation
    TOKEN_WHITESPACE = 7,

    // Value 8: Plus operator
    TOKEN_PLUS = 8,

    // Value 9: Minus operator
    TOKEN_MINUS = 9,

    // Value 10: Multiplication operator
    TOKEN_MULT = 10,

    // Value 11: Division operator
    TOKEN_DIV = 11,

    // Value 12: Dollar sign (pattern immediate value)
    TOKEN_DOLLAR = 12,

    // Value 13: Function call
    EXPR_CALL = 13,

    // Value 14: Variable reference
    TOKEN_VARIABLE = 14,

    // Value 15: String literal
    TOKEN_STRING = 15,

    // Value 16: Left parenthesis
    TOKEN_LPAREN = 16
} token_type_t;

//
// Node structure for Snobol III interpreter
//
struct Node {
    Node *p1;
    Node *p2;
    token_type_t typ;
    char ch;

    int equal(const Node *other) const;
    int bextend(const Node *last);
    int ubextend(const Node *last);
};

//
// Snobol interpreter context class
// Holds all global state previously stored in global variables
//
class SnobolContext {
public:
    // Constructor - references must be initialized in initializer list
    SnobolContext(std::istream &input, std::ostream &output) : fin(input), fout(output) {}

    // I/O streams (references - cannot be reassigned)
    std::istream &fin;
    std::ostream &fout;

    // Memory management
    int freesize{};
    Node *freespace{};
    Node *freespace_current{};
    Node *freespace_end{};
    Node *freelist{};

    // Symbol table
    Node *namelist{};
    Node *lookf{};
    Node *looks{};
    Node *lookend{};
    Node *lookstart{};
    Node *lookdef{};
    Node *lookret{};
    Node *lookfret{};

    // Execution state
    int cfail{};
    int rfail{};
    int lc{};
    Node *schar{};
    Node *current_line{}; // Current input line being processed
    int line_flag{};        // Flag for end of line
    int compon_next{};      // Flag for compon() to reuse current character

    // Methods from sno1.c
    void mes(const char *s);
    Node *init(const char *s, token_type_t t);
    Node *syspit();
    void syspot(Node *string);
    Node *cstr_to_node(const char *s);
    Node *alloc();
    void free_node(Node *pointer);
    int nfree();
    Node *look(Node *string);
    Node *copy(Node *string);
    int strbin(Node *string);
    Node *binstr(int binary);
    Node *add(Node *string1, Node *string2);
    Node *sub(Node *string1, Node *string2);
    Node *mult(Node *string1, Node *string2);
    Node *divide(Node *string1, Node *string2);
    Node *cat(Node *string1, Node *string2);
    Node *dcat(Node *a, Node *b);
    void delete_string(Node *string);
    void sysput(Node *string);
    void dump();
    void writes(const char *s);
    Node *getc_char();
    void flush();

    // Methods from sno2.c
    Node *compon();
    Node *nscomp();
    Node *push(Node *stack);
    Node *pop(Node *stack);
    Node *expr(Node *start, token_type_t eof, Node *e);
    Node *match(Node *start, Node *m);
    Node *compile();

    // Methods from sno3.c
    Node *search(Node *arg, Node *r);

    // Methods from sno4.c
    Node *eval_operand(Node *ptr);
    Node *eval(Node *e, int t);
    Node *doop(int op, Node *arg1, Node *arg2);
    Node *execute(Node *e);
    void assign(Node *adr, Node *val);

    // Standalone functions (no context parameter)
    static char_class_t char_class(int c);

private:
    // Private helper methods
    void dump1(Node *base);
};

#endif // SNO_H
