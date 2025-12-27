#ifndef SNO_H
#define SNO_H

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
// Snobol interpreter context structure
// Holds all global state previously stored in global variables
//
typedef struct snobol_context {
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

    // I/O
    int fin;
    int fout;
} snobol_context_t;

//
// Function prototypes from sno1.c
//
snobol_context_t *snobol_context_create(void);
void mes(snobol_context_t *ctx, const char *s);
node_t *init(snobol_context_t *ctx, const char *s, int t);
node_t *syspit(snobol_context_t *ctx);
void syspot(snobol_context_t *ctx, node_t *string);
node_t *cstr_to_node(snobol_context_t *ctx, const char *s);
char_class_t char_class(int c);
node_t *alloc(snobol_context_t *ctx);
void free_node(snobol_context_t *ctx, node_t *pointer);
int nfree(snobol_context_t *ctx);
node_t *look(snobol_context_t *ctx, node_t *string);
node_t *copy(snobol_context_t *ctx, node_t *string);
int equal(node_t *string1, node_t *string2);
int strbin(snobol_context_t *ctx, node_t *string);
node_t *binstr(snobol_context_t *ctx, int binary);
node_t *add(snobol_context_t *ctx, node_t *string1, node_t *string2);
node_t *sub(snobol_context_t *ctx, node_t *string1, node_t *string2);
node_t *mult(snobol_context_t *ctx, node_t *string1, node_t *string2);
node_t *divide(snobol_context_t *ctx, node_t *string1, node_t *string2);
node_t *cat(snobol_context_t *ctx, node_t *string1, node_t *string2);
node_t *dcat(snobol_context_t *ctx, node_t *a, node_t *b);
void delete_string(snobol_context_t *ctx, node_t *string);
void sysput(snobol_context_t *ctx, node_t *string);
void dump(snobol_context_t *ctx);
void dump1(snobol_context_t *ctx, node_t *base);
void writes(snobol_context_t *ctx, const char *s);
node_t *getc_char(snobol_context_t *ctx);
void flush(void);

//
// Function prototypes from sno2.c
//
node_t *compon(snobol_context_t *ctx);
node_t *nscomp(snobol_context_t *ctx);
node_t *push(snobol_context_t *ctx, node_t *stack);
node_t *pop(snobol_context_t *ctx, node_t *stack);
node_t *expr(snobol_context_t *ctx, node_t *start, int eof, node_t *e);
node_t *match(snobol_context_t *ctx, node_t *start, node_t *m);
node_t *compile(snobol_context_t *ctx);

//
// Function prototypes from sno3.c
//
int bextend(node_t *str, node_t *last);
int ubextend(node_t *str, node_t *last);
node_t *search(snobol_context_t *ctx, node_t *arg, node_t *r);

//
// Function prototypes from sno4.c
//
node_t *eval_operand(snobol_context_t *ctx, node_t *ptr);
node_t *eval(snobol_context_t *ctx, node_t *e, int t);
node_t *doop(snobol_context_t *ctx, int op, node_t *arg1, node_t *arg2);
node_t *execute(snobol_context_t *ctx, node_t *e);
void assign(snobol_context_t *ctx, node_t *adr, node_t *val);

#endif // SNO_H
