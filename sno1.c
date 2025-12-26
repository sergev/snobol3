/*
 *   Snobol III
 */
#include "sno.h"

int freesize = 0;
node_t *freespace = NULL;
node_t *freespace_current = NULL;
node_t *freespace_end = NULL;

int fin = 0;
int fout = 1;

/* Global variables */
int cfail = 0;
int rfail = 0;
node_t *freelist = NULL;
node_t *namelist = NULL;
int lc = 0;
node_t *schar = NULL;
node_t *lookf = NULL;
node_t *looks = NULL;
node_t *lookend = NULL;
node_t *lookstart = NULL;
node_t *lookdef = NULL;
node_t *lookret = NULL;
node_t *lookfret = NULL;

void mes(const char *s)
{
    sysput(cstr_to_node(s));
}

node_t *init(const char *s, int t)
{
    node_t *a, *b;

    a = cstr_to_node(s);
    b = look(a);
    delete(a);
    b->typ = t;
    return (b);
}

int main(int argc, char *argv[])
{
    node_t *a, *b, *c;

    if (argc > 1) {
        fin = open(argv[1], 0);
        if (fin < 0) {
            mes("cannot open input");
            exit(1);
        }
    }
    fout      = dup(1);
    lookf     = init("f", 0);
    looks     = init("s", 0);
    lookend   = init("end", 0);
    lookstart = init("start", 0);
    lookdef   = init("define", 0);
    lookret   = init("return", 0);
    lookfret  = init("freturn", 0);
    init("syspit", 3);
    init("syspot", 4);
    a = c = compile();
    while (lookend->typ != 2) {
        a->p1 = b = compile();
        a         = b;
    }
    cfail = 1;
    a->p1 = 0;
    if (lookstart->typ == 2)
        c = lookstart->p2;
    while ((c = execute(c)) != NULL)
        ;
    flush();
    return 0;
}

node_t *syspit(void)
{
    node_t *b, *c, *d;
    int a;

    if ((a = getchar()) == '\n')
        return (NULL);
    b = c = alloc();
    while (a != '\n') {
        c->p1 = d = alloc();
        c         = d;
    l:
        c->ch = a;
        if (a == '\0') {
            if (fin) {
                close(fin);
                fin = 0;
                a   = getchar();
                goto l;
            }
            rfail = 1;
            break;
        }
        a = getchar();
    }
    b->p2 = c;
    if (rfail) {
        delete(b);
        b = NULL;
    }
    return (b);
}

void syspot(node_t *string)
{
    node_t *a, *b, *s;

    s = string;
    if (s != NULL) {
        a = s;
        b = s->p2;
        while (a != b) {
            a = a->p1;
            putchar(a->ch);
        }
    }
    putchar('\n');
}

node_t *cstr_to_node(const char *s)
{
    int c;
    node_t *e, *f, *d;

    d = f = alloc();
    while ((c = *s++) != '\0') {
        (e = alloc())->ch = c;
        f->p1             = e;
        f                 = e;
    }
    d->p2 = e;
    return (d);
}

int class(int c)
{
    switch (c) {
    case ')':
        return (1);
    case '(':
        return (2);
    case '\t':
    case ' ':
        return (3);
    case '+':
        return (4);
    case '-':
        return (5);
    case '*':
        return (6);
    case '/':
        return (7);
    case '$':
        return (8);
    case '"':
    case '\'':
        return (9);
    case '=':
        return (10);
    case ',':
        return (11);
    }
    return (0);
}

node_t *alloc(void)
{
    node_t *f;
    size_t alloc_size;

    if (freelist == NULL) {
        if (freespace_current == NULL || freespace_current >= freespace_end) {
            alloc_size = 200 * sizeof(node_t);
            if (freespace == NULL) {
                freespace = (node_t *)malloc(alloc_size);
                if (freespace == NULL) {
                    flush();
                    write(fout, "Out of free space\n", 18);
                    exit(1);
                }
                freespace_current = freespace;
                freespace_end = freespace + 200;
                freesize = 200;
            } else {
                /* Allocate new block and append */
                node_t *new_block = (node_t *)malloc(alloc_size);
                if (new_block == NULL) {
                    flush();
                    write(fout, "Out of free space\n", 18);
                    exit(1);
                }
                freespace_current = new_block;
                freespace_end = new_block + 200;
                freesize = 200;
            }
        }
        f = freespace_current++;
        freesize--;
        return (f);
    }
    f        = freelist;
    freelist = freelist->p1;
    return (f);
}

void free_node(node_t *pointer)
{
    pointer->p1 = freelist;
    freelist    = pointer;
}

int nfree(void)
{
    int i;
    node_t *a;

    i = freesize;
    a = freelist;
    while (a) {
        a = a->p1;
        i++;
    }
    return (i);
}

node_t *look(node_t *string)
{
    node_t *i, *j, *k;

    k = NULL;
    i = namelist;
    while (i) {
        j = i->p1;
        if (equal(j->p1, string) == 0)
            return (j);
        i = (k = i)->p2;
    }
    i     = alloc();
    i->p2 = NULL;
    if (k)
        k->p2 = i;
    else
        namelist = i;
    j      = alloc();
    i->p1  = j;
    j->p1  = copy(string);
    j->p2  = NULL;
    j->typ = 0;
    return (j);
}

node_t *copy(node_t *string)
{
    node_t *j, *l, *m;
    node_t *i, *k;

    if (string == NULL)
        return (NULL);
    i = l = alloc();
    j     = string;
    k     = string->p2;
    while (j != k) {
        m     = alloc();
        m->ch = (j = j->p1)->ch;
        l->p1 = m;
        l     = m;
    }
    i->p2 = l;
    return (i);
}

int equal(node_t *string1, node_t *string2)
{
    node_t *i, *j, *k;
    node_t *l;
    int n, m;

    if (string1 == NULL) {
        if (string2 == NULL)
            return (0);
        return (-1);
    }
    if (string2 == NULL)
        return (1);
    i = string1;
    j = string1->p2;
    k = string2;
    l = string2->p2;
    for (;;) {
        m = (i = i->p1)->ch;
        n = (k = k->p1)->ch;
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

int strbin(node_t *string)
{
    int n, m, sign;
    node_t *p, *q, *s;

    s = string;
    n = 0;
    if (s == NULL)
        return (0);
    p    = s->p1;
    q    = s->p2;
    sign = 1;
    if (class(p->ch) == 5) { /* minus */
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

node_t *binstr(int binary)
{
    int n, sign;
    node_t *m, *p, *q;

    n    = binary;
    p    = alloc();
    q    = alloc();
    sign = 1;
    if (binary < 0) {
        sign = -1;
        n    = -binary;
    }
    p->p2 = q;
loop:
    q->ch = n % 10 + '0';
    n     = n / 10;
    if (n == 0) {
        if (sign < 0) {
            m     = alloc();
            m->p1 = q;
            q     = m;
            q->ch = '-';
        }
        p->p1 = q;
        return (p);
    }
    m     = alloc();
    m->p1 = q;
    q     = m;
    goto loop;
}

node_t *add(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) + strbin(string2)));
}

node_t *sub(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) - strbin(string2)));
}

node_t *mult(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) * strbin(string2)));
}

node_t *divide(node_t *string1, node_t *string2)
{
    return (binstr(strbin(string1) / strbin(string2)));
}

node_t *cat(node_t *string1, node_t *string2)
{
    node_t *a, *b;

    if (string1 == NULL)
        return (copy(string2));
    if (string2 == NULL)
        return (copy(string1));
    a         = copy(string1);
    b         = copy(string2);
    a->p2->p1 = b->p1;
    a->p2     = b->p2;
    free_node(b);
    return (a);
}

node_t *dcat(node_t *a, node_t *b)
{
    node_t *c;

    c = cat(a, b);
    delete(a);
    delete(b);
    return (c);
}

void delete(node_t *string)
{
    node_t *a, *b, *c;

    if (string == NULL)
        return;
    a = string;
    b = string->p2;
    while (a != b) {
        c = a->p1;
        free_node(a);
        a = c;
    }
    free_node(a);
}

void sysput(node_t *string)
{
    syspot(string);
    delete(string);
}

void dump(void)
{
    dump1(namelist);
}

void dump1(node_t *base)
{
    node_t *b, *c, *e;
    node_t *d;

    while (base) {
        b = base->p1;
        c = binstr(b->typ);
        d = cstr_to_node("  ");
        e = dcat(c, d);
        sysput(cat(e, b->p1));
        delete(e);
        if (b->typ == 1) {
            c = cstr_to_node("   ");
            sysput(cat(c, b->p2));
            delete(c);
        }
        base = base->p2;
    }
}

void writes(const char *s)
{
    sysput(dcat(binstr(lc), dcat(cstr_to_node("\t"), cstr_to_node(s))));
    flush();
    if (cfail) {
        dump();
        flush();
        exit(1);
    }
    while (getc_char() != NULL)
        ;
    while (compile() != NULL)
        ;
    flush();
    exit(1);
}

node_t *getc_char(void)
{
    node_t *a;
    static node_t *line;
    static int linflg;

    while (line == NULL) {
        line = syspit();
        if (rfail) {
            cfail++;
            writes("eof on input");
        }
        lc++;
    }
    if (linflg) {
        line   = NULL;
        linflg = 0;
        return (NULL);
    }
    a = line->p1;
    if (a == line->p2) {
        free_node(line);
        linflg++;
    } else
        line->p1 = a->p1;
    return (a);
}

void flush(void)
{
    fflush(stdout);
}
