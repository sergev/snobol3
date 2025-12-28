#ifndef SNO_H
#define SNO_H

#include <array>
#include <iostream>
#include <memory>
#include <vector>

//
// Character class enumeration for lexical analysis
//
enum class CharClass {
    OTHER        = 0,  // Default/unclassified character
    RPAREN       = 1,  // Right parenthesis ')'
    LPAREN       = 2,  // Left parenthesis '('
    WHITESPACE   = 3,  // Whitespace (space, tab)
    PLUS         = 4,  // Plus operator '+'
    MINUS        = 5,  // Minus operator '-'
    ASTERISK     = 6,  // Asterisk operator '*'
    SLASH        = 7,  // Division operator '/'
    DOLLAR       = 8,  // Dollar sign '$'
    STRING_DELIM = 9,  // String delimiter '"' or '\''
    EQUALS       = 10, // Equals sign '='
    COMMA        = 11  // Comma ','
};

//
// Token type enumeration for node types
//
enum class Token {

    // Lexical/parsing operations
    TOKEN_END         = 0,  // End marker
    TOKEN_UNANCHORED  = 1,  // Unanchored search
    TOKEN_ALTERNATION = 2,  // Pattern alternation
    TOKEN_EQUALS      = 3,  // Equals
    TOKEN_COMMA       = 4,  // Comma
    TOKEN_RPAREN      = 5,  // Right parenthesis
    TOKEN_MARKER      = 6,  // Marker for left paren
    TOKEN_WHITESPACE  = 7,  // Whitespace / Concatenation
    TOKEN_PLUS        = 8,  // Plus operator
    TOKEN_MINUS       = 9,  // Minus operator
    TOKEN_MULT        = 10, // Multiplication operator
    TOKEN_DIV         = 11, // Division operator
    TOKEN_DOLLAR      = 12, // Dollar sign (pattern immediate value)
    TOKEN_CALL        = 13, // Function call
    TOKEN_VARIABLE    = 14, // Variable reference
    TOKEN_STRING      = 15, // String literal
    TOKEN_LPAREN      = 16, // Left parenthesis

    // Runtime/evaluation operations
    EXPR_VAR_REF  = 0,  // Variable reference TODO: use unique value
    EXPR_VALUE    = 51, // Value
    EXPR_LABEL    = 52, // Label
    EXPR_SYSPIT   = 53, // System input function
    EXPR_SYSPOT   = 54, // System output
    EXPR_FUNCTION = 55, // Function

    // Statement types
    STMT_SIMPLE  = 100, // Expression evaluation statement
    STMT_MATCH   = 101, // Pattern matching statement
    STMT_ASSIGN  = 102, // Assignment statement
    STMT_REPLACE = 103, // Pattern replacement
};

//
// Node structure for Snobol III interpreter
//
struct Node {
    Node *p1;
    Node *p2;
    Token typ;
    char ch;

    int equal(const Node *other) const;
    int bextend(const Node *last);
    int ubextend(const Node *last);
    void debug_print(std::ostream &os, int depth = 0, int max_depth = 10) const;
};

//
// Snobol interpreter context class
// Holds all global state previously stored in global variables
//
class SnobolContext {
public:
    // Constructor - references must be initialized in initializer list
    SnobolContext(std::ostream &output);

    // I/O streams
    std::istream *fin;
    std::ostream &fout;

    // Memory management
    static const unsigned BLOCK_SIZE = 200;
    using NodeBlock                  = std::array<Node, BLOCK_SIZE>;
    std::vector<std::unique_ptr<NodeBlock>> mem_pool;
    Node *freelist{};
    int freesize{};

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
    Node *program{};
    int cfail{};
    int rfail{};
    int lc{};
    Node *schar{};
    Node *current_line{}; // Current input line being processed
    int line_flag{};      // Flag for end of line
    int compon_next{};    // Flag for compon() to reuse current character

    // Methods from sno1.c
    void compile_program(std::istream &input);
    void execute_program(std::istream &input);
    void mes(const char *s);
    Node &init(const char *s, Token t);
    Node *syspit();
    void syspot(Node *string) const;
    Node &cstr_to_node(const char *s);
    Node &alloc();
    void free_node(Node &pointer);
    Node &look(const Node &string);
    Node *copy(const Node *string);
    int strbin(const Node *string);
    Node &binstr(int binary);
    Node &add(const Node &string1, const Node &string2);
    Node &sub(const Node &string1, const Node &string2);
    Node &mult(const Node &string1, const Node &string2);
    Node &divide(const Node &string1, const Node &string2);
    Node *cat(const Node *string1, const Node *string2);
    Node *dcat(Node &a, Node &b); // Deletes a and b, so non-const
    void delete_string(Node *string);
    void sysput(Node *string);
    void dump();
    void writes(const char *s);
    Node *getc_char();
    void flush() const;

    // Methods from sno2.c
    Node &compon();
    Node &nscomp();
    Node &push(Node *stack); // Can be null, must stay as pointer
    Node *pop(Node *stack);  // Can be null, must stay as pointer
    Node &expr(Node *start, Token eof, Node &e);
    Node &match(Node *start, Node &m);
    Node *compile();

    // Methods from sno3.c
    Node *search(const Node &arg, Node *r);

    // Methods from sno4.c
    Node *eval_operand(const Node &ptr);
    Node *eval(const Node &e, int t);
    Node *doop(Token op, const Node &arg1, const Node &arg2);
    Node *execute(const Node &e);
    void assign(Node &adr, Node &val); // val is deleted, so non-const

    // Standalone functions (no context parameter)
    static CharClass char_class(int c);

private:
    // Private helper methods
    void dump1(Node *base); // base can be null, must stay as pointer
};

#endif // SNO_H
