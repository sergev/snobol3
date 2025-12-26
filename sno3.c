#include "sno.h"

/*
 * sno3
 */
int bextend(node_t *str, node_t *last)
{
    node_t *a, *s;
    int b;
    node_t *c;
    int d;
    int class_val;

    s = str;
    c = s->p1;
    if (c == NULL)
        goto bad;
    b = 0;
    d = 0;
    a = s->p2;
    if (a == NULL) {
        a = c;
        goto eb2;
    }
eb1:
    if (a == last)
        goto bad;
    a = a->p1;
eb2:
    d++;
    class_val = class(a->ch);
    if (class_val == 1) { /* rp */
        if (b == 0)
            goto bad;
        b--;
        goto eb3;
    }
    if (class_val == 2) { /* lp */
        b++;
        goto eb1;
    }
eb3:
    if (b == 0) {
        s->p2 = a;
        return (d);
    }
    goto eb1;
bad:
    return (0);
}

int ubextend(node_t *str, node_t *last)
{
    node_t *a, *b, *s;

    s = str;
    a = s->p1;
    if (a == NULL)
        goto bad;
    b = s->p2;
    if (b == NULL)
        goto good;
    if (b == last)
        goto bad;
    a = b->p1;
good:
    s->p2 = a;
    return (1);
bad:
    return (0);
}

node_t *search(node_t *arg, node_t *r)
{
    node_t *list, *back, *str, *etc, *next, *last, *base, *e;
    node_t *a, *b, *var;
    int c, d;

    a    = arg->p2;
    list = base = alloc();
    last        = NULL;
    next        = NULL;
    goto badv1;
badvanc:
    a = a->p1;
    if (a->typ == 0) {
        list->p1 = NULL;
        if (rfail == 1) {
            a = NULL;
            goto fail;
        }
        list = base;
        if (r == NULL)
            next = last = NULL;
        else {
            next = r->p1;
            last = r->p2;
        }
        goto adv1;
    }
    b        = alloc();
    list->p1 = b;
    list     = b;
badv1:
    list->p2 = back = alloc();
    back->p1        = last;
    b               = a->p2;
    c               = a->typ;
    list->typ       = c;
    if (c < 2) {
        back->p2 = eval(b, 1);
        goto badvanc;
    }
    last     = list;
    str      = alloc();
    etc      = alloc();
    back->p2 = var = alloc();
    var->typ       = b->typ;
    var->p1        = str;
    var->p2        = etc;
    e              = b->p1;
    if (e == NULL)
        etc->p1 = NULL;
    else
        etc->p1 = eval(e, 0);
    e = b->p2;
    if (e == NULL)
        etc->p2 = NULL;
    else {
        e       = eval(e, 1);
        etc->p2 = (node_t *)(long)(intptr_t)strbin(e);
        delete_string(e);
    }
    goto badvanc;

retard:
    a = back->p1;
    if (a == NULL) {
        rfail = 1;
        goto fail;
    }
    list = a;
    back = list->p2;
    var  = back->p2;
    str  = var->p1;
    etc  = var->p2;
    if (etc->p2)
        goto retard;
    if (var->typ == 1) {
        if (bextend(str, last) == 0)
            goto retard;
        goto adv0;
    }
    if (ubextend(str, last) == 0)
        goto retard;
adv0:
    a = str->p2;
adv01:
    if (a == last)
        next = NULL;
    else
        next = a->p1;
advanc:
    a = list->p1;
    if (a == NULL) {
        a = alloc();
        if (r == NULL) {
            a->p1 = a->p2 = NULL;
            goto fail;
        }
        b     = r->p1;
        a->p1 = b;
        if (next == NULL) {
            a->p2 = r->p2;
            goto fail;
        }
        while (1) {
            e = b->p1;
            if (e == next) {
                a->p2 = b;
                goto fail;
            }
            b = e;
        }
    }
    list = a;
adv1:
    back = list->p2;
    var  = back->p2;
    d    = list->typ;
    if (d < 2) {
        if (var == NULL)
            goto advanc;
        if (next == NULL)
            goto retard;
        a = next;
        b = var->p1;
        e = var->p2;
        while (1) {
            if (a->ch != b->ch)
                goto retard;
            if (b == e)
                goto adv01;
            if (a == last)
                goto retard;
            a = a->p1;
            b = b->p1;
        }
    }
    str     = var->p1;
    etc     = var->p2;
    str->p1 = next;
    str->p2 = NULL;
    c       = (int)(intptr_t)etc->p2;
    if (var->typ == 1) {
        d = bextend(str, last);
        if (d == 0)
            goto retard;
        if (c == 0)
            goto adv0;
        while (1) {
            c = -d;
            if (c == 0)
                goto adv0;
            if (c < 0)
                goto retard;
            d = bextend(str, last);
            if (d == 0)
                goto retard;
        }
    }
    if (c == 0) {
        if (d == 3 && next != NULL) {
            str->p2 = last;
            goto adv0;
        }
        goto advanc;
    }
    while (c--)
        if (ubextend(str, last) == 0)
            goto retard;
    goto adv0;

fail:
    list = base;
    goto f1;
fadv:
    free_node(back);
    b = list->p1;
    free_node(list);
    if (b == NULL)
        return (a);
    list = b;
f1:
    back = list->p2;
    var  = back->p2;
    if (list->typ < 2) {
        delete_string(var);
        goto fadv;
    }
    str = var->p1;
    etc = var->p2;
    if (a != NULL && etc->p1 != NULL) {
        if (str->p2 == NULL) {
            free_node(str);
            str = NULL;
        }
        assign(etc->p1, copy(str));
    }
    if (str)
        free_node(str);
    free_node(etc);
    free_node(var);
    goto fadv;
}
