#ifndef SNO_H
#define SNO_H

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Node structure for Snobol III interpreter */
typedef struct node {
    struct node *p1;
    struct node *p2;
    char typ;
    char ch;
} node_t;

/* Global variables */
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

/* Memory management */
extern node_t *freespace;
extern node_t *freespace_current;
extern node_t *freespace_end;
extern int fin;
extern int fout;

/* Function prototypes from sno1.c */
void mes(const char *s);
node_t *init(const char *s, int t);
node_t *syspit(void);
void syspot(node_t *string);
node_t *cstr_to_node(const char *s);
int class(int c);
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

/* Function prototypes from sno2.c */
node_t *compon(void);
node_t *nscomp(void);
node_t *push(node_t *stack);
node_t *pop(node_t *stack);
node_t *expr(node_t *start, int eof, node_t *e);
node_t *match(node_t *start, node_t *m);
node_t *compile(void);

/* Function prototypes from sno3.c */
int bextend(node_t *str, node_t *last);
int ubextend(node_t *str, node_t *last);
node_t *search(node_t *arg, node_t *r);

/* Function prototypes from sno4.c */
node_t *eval_operand(node_t *ptr);
node_t *eval(node_t *e, int t);
node_t *doop(int op, node_t *arg1, node_t *arg2);
node_t *execute(node_t *e);
void assign(node_t *adr, node_t *val);

#endif /* SNO_H */
