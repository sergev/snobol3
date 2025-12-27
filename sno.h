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
// Global variables
//
extern int freesize;
extern node_t *lookf;
extern node_t *looks;
extern node_t *lookend;
extern node_t *lookstart;
extern node_t *lookdef;
extern node_t *lookret;
extern node_t *lookfret;
extern int cfail;
extern int rfail;
extern node_t *freelist;
extern node_t *namelist;
extern int lc;
extern node_t *schar;

//
// Memory management
//
extern node_t *freespace;
extern node_t *freespace_current;
extern node_t *freespace_end;
extern int fin;
extern int fout;

//
// Function prototypes from sno1.c
//
void mes(const char *s);
node_t *init(const char *s, int t);
node_t *syspit(void);
void syspot(node_t *string);
node_t *cstr_to_node(const char *s);
char_class_t char_class(int c);
node_t *alloc(void);
void free_node(node_t *pointer);
int nfree(void);
node_t *look(node_t *string);
node_t *copy(node_t *string);
int equal(node_t *string1, node_t *string2);
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
void dump(void);
void dump1(node_t *base);
void writes(const char *s);
node_t *getc_char(void);
void flush(void);

//
// Function prototypes from sno2.c
//
node_t *compon(void);
node_t *nscomp(void);
node_t *push(node_t *stack);
node_t *pop(node_t *stack);
node_t *expr(node_t *start, int eof, node_t *e);
node_t *match(node_t *start, node_t *m);
node_t *compile(void);

//
// Function prototypes from sno3.c
//
int bextend(node_t *str, node_t *last);
int ubextend(node_t *str, node_t *last);
node_t *search(node_t *arg, node_t *r);

//
// Function prototypes from sno4.c
//
node_t *eval_operand(node_t *ptr);
node_t *eval(node_t *e, int t);
node_t *doop(int op, node_t *arg1, node_t *arg2);
node_t *execute(node_t *e);
void assign(node_t *adr, node_t *val);

#endif // SNO_H
