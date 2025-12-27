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
typedef struct node {
    struct node *p1;
    struct node *p2;
    token_type_t typ;
    char ch;
} node_t;

//
// Snobol interpreter context class
// Holds all global state previously stored in global variables
//
class SnobolContext {
public:
    // Constructor - references must be initialized in initializer list
    SnobolContext(std::istream &input, std::ostream &output);

    // I/O streams (references - cannot be reassigned)
    std::istream &fin;
    std::ostream &fout;

    // Memory management
    int freesize;
    node_t *freespace;
    node_t *freespace_current;
    node_t *freespace_end;
    node_t *freelist;

    // Symbol table
    node_t *namelist;
    node_t *lookf;
    node_t *looks;
    node_t *lookend;
    node_t *lookstart;
    node_t *lookdef;
    node_t *lookret;
    node_t *lookfret;

    // Execution state
    int cfail;
    int rfail;
    int lc;
    node_t *schar;
    node_t *current_line; // Current input line being processed
    int line_flag;        // Flag for end of line
    int compon_next;      // Flag for compon() to reuse current character

    // Methods from sno1.c
    void mes(const char *s);
    node_t *init(const char *s, int t);
    node_t *syspit();
    void syspot(node_t *string);
    node_t *cstr_to_node(const char *s);
    node_t *alloc();
    void free_node(node_t *pointer);
    int nfree();
    node_t *look(node_t *string);
    node_t *copy(node_t *string);
    int strbin(node_t *string);
    node_t *binstr(int binary);
    node_t *add(node_t *string1, node_t *string2);
    node_t *sub(node_t *string1, node_t *string2);
    node_t *mult(node_t *string1, node_t *string2);
    node_t *divide(node_t *string1, node_t *string2);
    node_t *cat(node_t *string1, node_t *string2);
    node_t *dcat(node_t *a, node_t *b);
    void delete_string(node_t *string);
    void sysput(node_t *string);
    void dump();
    void writes(const char *s);
    node_t *getc_char();
    void flush();

    // Methods from sno2.c
    node_t *compon();
    node_t *nscomp();
    node_t *push(node_t *stack);
    node_t *pop(node_t *stack);
    node_t *expr(node_t *start, int eof, node_t *e);
    node_t *match(node_t *start, node_t *m);
    node_t *compile();

    // Methods from sno3.c
    node_t *search(node_t *arg, node_t *r);

    // Methods from sno4.c
    node_t *eval_operand(node_t *ptr);
    node_t *eval(node_t *e, int t);
    node_t *doop(int op, node_t *arg1, node_t *arg2);
    node_t *execute(node_t *e);
    void assign(node_t *adr, node_t *val);

private:
    // Private helper methods
    void dump1(node_t *base);
};

// Standalone functions (no context parameter)
char_class_t char_class(int c);
int equal(node_t *string1, node_t *string2);
int bextend(node_t *str, node_t *last);
int ubextend(node_t *str, node_t *last);

#endif // SNO_H
