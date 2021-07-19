/****************************************************************
Copyright (C) Lucent Technologies 1997
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name Lucent Technologies or any of
its entities not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "awk.h"
#include "ytab.h"
#include <awklib/err.h>


/// Allocate a node with n descendants
Node *nodealloc (int n)
{
  Node *x;
  x = (Node *)malloc (sizeof (Node) + (n - 1) * sizeof (Node *));
  if (x == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space in nodealloc");
  x->nnext = NULL;
  x->lineno = lineno;
  x->args = n;
  return x;
}

#ifndef NDEBUG
const char *tokname (int tok);
#endif

/// Free a node and its descendants
void freenode (Node *p)
{
  Node *temp;
  while (p)
  {
    if ((ptrdiff_t)p < LASTTOKEN)
      break;
#ifndef NDEBUG
    dprintf ("%Deleting node 0x%p %s with %d arguments (line %d)", 
      p, tokname (p->nobj), p->args, p->lineno);
    dprintf (" type %s\n", p->ntype == NVALUE ? "value" : p->ntype == NSTAT ? "statement" : "expression");
#endif
    if (p->ntype != NVALUE)
    {
      for (int i = 0; i < p->args; i++)
        freenode (p->narg[i]);
    }
    temp = p;
    p = p->nnext;
    if (temp != nullnode)
      free (temp);
  }
}

/// Convert node to a statement node (presumably from an expression node)
Node *exptostat (Node *a)
{
  a->ntype = NSTAT;
  return a;
}

/// Create a node with one descendant 
Node *node1 (int tokid, pfun fn, Node *arg1)
{
  Node *x;

  x = nodealloc (1);
  x->nobj = tokid;
  x->proc = fn;
  x->narg[0] = arg1;
  return x;
}

/// Crate a node with 2 descendants
Node *node2 (int tokid, pfun fn, Node *arg1, Node *arg2)
{
  Node *x;

  x = nodealloc (2);
  x->nobj = tokid;
  x->proc = fn;
  x->narg[0] = arg1;
  x->narg[1] = arg2;
  return x;
}

/// Create a node with 3 descendants
Node *node3 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3)
{
  Node *x;

  x = nodealloc (3);
  x->nobj = tokid;
  x->proc = fn;
  x->narg[0] = arg1;
  x->narg[1] = arg2;
  x->narg[2] = arg3;
  return x;
}

/// Create a node with 4 descendants
Node *node4 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, Node *arg4)
{
  Node *x;

  x = nodealloc (4);
  x->nobj = tokid;
  x->proc = fn;
  x->narg[0] = arg1;
  x->narg[1] = arg2;
  x->narg[2] = arg3;
  x->narg[3] = arg4;
  return x;
}

/// Create a statement node with 1 descendant
Node *stat1 (int tokid, pfun fn, Node *arg1)
{
  Node *x;

  x = node1 (tokid, fn, arg1);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 2 descendants
Node *stat2 (int tokid, pfun fn, Node *arg1, Node *arg2)
{
  Node *x;

  x = node2 (tokid, fn, arg1, arg2);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 3 descendants
Node *stat3 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3)
{
  Node *x;

  x = node3 (tokid, fn, arg1, arg2, arg3);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 4 descendants
Node *stat4 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, Node *arg4)
{
  Node *x;

  x = node4 (tokid, fn, arg1, arg2, arg3, arg4);
  x->ntype = NSTAT;
  return x;
}

/// Create an expression node with 1 descendant
Node *op1 (int tokid, pfun fn, Node *arg1)
{
  Node *x;

  x = node1 (tokid, fn, arg1);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 2 descendants
Node *op2 (int tokid, pfun fn, Node *arg1, Node *arg2)
{
  Node *x;

  x = node2 (tokid, fn, arg1, arg2);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 3 descendants
Node *op3 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3)
{
  Node *x;

  x = node3 (tokid, fn, arg1, arg2, arg3);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 4 descendants
Node *op4 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, Node *arg4)
{
  Node *x;

  x = node4 (tokid, fn, arg1, arg2, arg3, arg4);
  x->ntype = NEXPR;
  return x;
}

Node *celltonode (Cell *a, Cell::subtype csub)
{
  Node *x;

  a->ctype = Cell::type::OCELL;
  a->csub = csub;
  x = node1 (0, nullproc, (Node *)a);
  x->ntype = NVALUE;
  return x;
}

Node *rectonode (void)  /* make $0 into a Node */
{
  extern Cell *literal0;
  return op1 (INDIRECT, indirect, celltonode (literal0, Cell::subtype::CCON));
}

Node *makearr (Node *p)
{
  Cell *cp;

  if (isvalue (p))
  {
    cp = (Cell *)(p->narg[0]);
    if (cp->isfcn ())
      SYNTAX ("%s is a function, not an array", cp->nval);
    else if (!cp->isarr ())
    {
      delete cp->sval;
      cp->arrval = new Array (NSYMTAB);
      cp->tval = ARR;
    }
  }
  return p;
}

#define PA2NUM  50  /* max number of pat,pat patterns allowed */
int  paircnt;    /* number of them in use */
int  pairstack[PA2NUM];  /* state of each pat,pat */

Node *pa2stat (Node *a, Node *b, Node *c)  /* pat, pat {...} */
{
  Node *x;

  x = node4 (PASTAT2, dopa2, a, b, c, itonp (paircnt));
  if (paircnt++ >= PA2NUM)
    SYNTAX ("limited to %d pat,pat statements", PA2NUM);
  x->ntype = NSTAT;
  return x;
}

Node *linkum (Node *a, Node *b)
{
  Node *c;

  if (errorflag)  /* don't link things that are wrong */
    return a;
  if (a == NULL)
    return b;
  else if (b == NULL)
    return a;
  for (c = a; c->nnext != NULL; c = c->nnext)
    ;
  c->nnext = b;
  return a;
}

void defn (Cell *v, Node *vl, Node *st)  /* turn on FCN bit in definition, */
{          /*   body of function, arglist */
  Node *p;
  int n;

  if (v->isarr ())
  {
    SYNTAX ("`%s' is an array name and a function name", v->nval);
    return;
  }
  if (isarg (v->nval) != -1)
  {
    SYNTAX ("`%s' is both function name and argument name", v->nval);
    return;
  }

  v->tval = FCN;
  v->sval = (char *)st;
  n = 0;  /* count arguments */
  for (p = vl; p; p = p->nnext)
    n++;
  v->fval = n;
  dprintf ("defining func %s (%d args)\n", v->nval, n);
}

int isarg (const char *s)    /* is s in argument list for current function? */
{      /* return -1 if not, otherwise arg # */
  extern Node *arglist;
  Node *p = arglist;
  int n;

  for (n = 0; p != 0; p = p->nnext, n++)
    if (strcmp (((Cell *)(p->narg[0]))->nval, s) == 0)
      return n;
  return -1;
}

int ptoi (void *p)  /* convert pointer to integer */
{
  return (int)(ptrdiff_t)p;  /* swearing that p fits, of course */
}

Node *itonp (int i)  /* and vice versa */
{
  return (Node *)(ptrdiff_t)i;
}


