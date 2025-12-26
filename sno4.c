#include "sno.h"

/*
 * sno4
 */
node_t *eval_operand(node_t *ptr)
{
    node_t *a, *p;

    p = ptr;
    a = p->p1;
    if (p->typ == 0) {
        switch (a->typ) {
        case 0:
            a->typ = 1;
            /* fall through */
        case 1:
            goto l1;
        case 3:
            flush();
            return (syspit());
        case 5:
            a = a->p2->p1;
            goto l1;
        case 6:
            return (binstr(nfree()));
        default:
            writes("attempt to take an illegal value");
            goto l1;
        }
    l1:
        a = copy(a->p2);
    }
    return (a);
}

node_t *eval(node_t *e, int t)
{
    node_t *list, *a3, *a4, *a3base;
    node_t *a1, *stack;
    int op;
    node_t *a2;

    if (rfail == 1)
        return (NULL);
    stack = NULL;
    list  = e;
    goto l1;
advanc:
    list = list->p1;
l1:
    op = list->typ;
    switch (op) {
    default:
    case 0:
        if (t == 1) {
            a1 = eval_operand(stack);
            goto e1;
        }
        if (stack->typ == 1)
            writes("attempt to store in a value");
        a1 = stack->p1;
    e1:
        stack = pop(stack);
        if (stack)
            writes("phase error");
        return (a1);
    case 12:
        a1        = eval_operand(stack);
        stack->p1 = look(a1);
        delete_string(a1);
        stack->typ = 0;
        goto advanc;
    case 13:
        if (stack->typ)
            writes("illegal function");
        a1 = stack->p1;
        if (a1->typ != 5)
            writes("illegal function");
        a1 = a1->p2;
        {
            node_t *op_ptr = a1->p1;
            a3base = a3 = alloc();
            a3->p2      = op_ptr->p2;
            op_ptr->p2  = NULL;
            a1          = a1->p2;
            a2          = list->p2;
        f1:
            if (a1 != NULL && a2 != NULL)
                goto f2;
            if (a1 != a2)
                writes("parameters do not match");
            op_ptr = op_ptr->p1;
            goto f3;
        f2:
            a3->p1 = a4 = alloc();
            a3          = a4;
            a3->p2      = eval_operand(a1);
            assign(a1->p1, eval(a2->p2, 1)); /* recursive */
            a1 = a1->p2;
            a2 = a2->p1;
            goto f1;
        f3:
            op_ptr = execute(op_ptr); /* recursive */
            if (op_ptr)
                goto f3;
            a1 = stack->p1->p2;
            {
                node_t *op_ptr2 = a1->p1;
                a3              = a3base;
                stack->p1       = op_ptr2->p2;
                stack->typ      = 1;
                op_ptr2->p2     = a3->p2;
            f4:
                a4 = a3->p1;
                free_node(a3);
                a3 = a4;
                a1 = a1->p2;
                if (a1 == NULL)
                    goto advanc;
                assign(a1->p1, a3->p2);
                goto f4;
            }
        }
    case 11:
    case 10:
    case 9:
    case 8:
    case 7:
        a1    = eval_operand(stack);
        stack = pop(stack);
        a2    = eval_operand(stack);
        a3    = doop(op, a2, a1);
        delete_string(a1);
        delete_string(a2);
        stack->p1  = a3;
        stack->typ = 1;
        goto advanc;
    case 15:
        a1 = copy(list->p2);
        {
            int a2_int = 1;
            stack      = push(stack);
            stack->p1  = a1;
            stack->typ = a2_int;
            goto advanc;
        }
    case 14:
        a1 = list->p2;
        {
            int a2_int = 0;
            stack      = push(stack);
            stack->p1  = a1;
            stack->typ = a2_int;
            goto advanc;
        }
        goto advanc;
    }
    return NULL;
}

node_t *doop(int op, node_t *arg1, node_t *arg2)
{
    switch (op) {
    case 11:
        return (divide(arg1, arg2));
    case 10:
        return (mult(arg1, arg2));
    case 8:
        return (add(arg1, arg2));
    case 9:
        return (sub(arg1, arg2));
    case 7:
        return (cat(arg1, arg2));
    }
    return (NULL);
}

node_t *execute(node_t *e)
{
    node_t *r, *b, *c;
    node_t *m, *ca, *d, *a;

    r  = e->p2;
    lc = e->ch;
    switch (e->typ) {
    case 0: /*  r g */
        a = r->p1;
        delete_string(eval(r->p2, 1));
        goto xsuc;
    case 1: /*  r m g */
        m = r->p1;
        a = m->p1;
        b = eval(r->p2, 1);
        c = search(m, b);
        delete_string(b);
        if (c == NULL)
            goto xfail;
        free_node(c);
        goto xsuc;
    case 2: /*  r a g */
        ca = r->p1;
        a  = ca->p1;
        b  = eval(r->p2, 0);
        assign(b, eval(ca->p2, 1));
        goto xsuc;
    case 3: /*  r m a g */
        m  = r->p1;
        ca = m->p1;
        a  = ca->p1;
        b  = eval(r->p2, 0);
        d  = search(m, b->p2);
        if (d == NULL)
            goto xfail;
        c = eval(ca->p2, 1);
        if (d->p1 == NULL) {
            free_node(d);
            assign(b, cat(c, b->p2));
            delete_string(c);
            goto xsuc;
        }
        if (d->p2 == b->p2->p2) {
            assign(b, c);
            free_node(d);
            goto xsuc;
        }
        (r = alloc())->p1 = d->p2->p1;
        r->p2             = b->p2->p2;
        assign(b, cat(c, r));
        free_node(d);
        free_node(r);
        delete_string(c);
        goto xsuc;
    }
xsuc:
    if (rfail)
        goto xfail;
    b = a->p1;
    goto xboth;
xfail:
    rfail = 0;
    b     = a->p2;
xboth:
    if (b == NULL) {
        return (e->p1);
    }
    b = eval(b, 0);
    if (b == lookret)
        return (NULL);
    if (b == lookfret) {
        rfail = 1;
        return (NULL);
    }
    if (b->typ != 2)
        writes("attempt to transfer to non-label");
    return (b->p2);
}

void assign(node_t *adr, node_t *val)
{
    node_t *a, *addr, *value;

    addr  = adr;
    value = val;
    if (rfail == 1) {
        delete_string(value);
        return;
    }
    switch (addr->typ) {
    default:
        writes("attempt to make an illegal assignment");
        return;
    case 0:
        addr->typ = 1;
        /* fall through */
    case 1:
        delete_string(addr->p2);
        addr->p2 = value;
        return;
    case 4:
        sysput(value);
        return;
    case 5:
        a = addr->p2->p1;
        delete_string(a->p2);
        a->p2 = value;
        return;
    }
}
