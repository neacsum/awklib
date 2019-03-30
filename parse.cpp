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
#include "awk.h"
#include "ytab.h"
#include <awkerr.h>

Node* alloc_list = 0;

static Node*  nodealloc (int);
static Node*  node1 (int, Node *);
static Node*  node2 (int, Node *, Node *);
static Node*  node3 (int, Node *, Node *, Node *);
static Node*  node4 (int, Node *, Node *, Node *, Node *);

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
  x->lalloc = alloc_list;
  alloc_list = x;
  return x;
}

char *tokname (int tok);
/// Free all memory allocated for nodes and cells
void freenodes (void)
{
  Node *temp = alloc_list;
  while (temp)
  {
    alloc_list = temp->lalloc;
    dprintf ("%Deleting node 0x%p %s", temp, tokname (temp->nobj));
    dprintf (" type %s\n", temp->ntype == NVALUE ? "value" : temp->ntype == NSTAT ? "statement" : "expression");
    free (temp);
    temp = alloc_list;
  }
}

/// Convert node to a statement node (presumably from an expression node)
Node *exptostat (Node *a)
{
  a->ntype = NSTAT;
  return a;
}

/// Create a node with one descendant 
Node *node1 (int a, Node *b)
{
  Node *x;

  x = nodealloc (1);
  x->nobj = a;
  x->narg[0] = b;
  return x;
}

/// Crate a node with 2 descendants
Node *node2 (int a, Node *b, Node *c)
{
  Node *x;

  x = nodealloc (2);
  x->nobj = a;
  x->narg[0] = b;
  x->narg[1] = c;
  return x;
}

/// Create a node with 3 descendants
Node *node3 (int a, Node *b, Node *c, Node *d)
{
  Node *x;

  x = nodealloc (3);
  x->nobj = a;
  x->narg[0] = b;
  x->narg[1] = c;
  x->narg[2] = d;
  return x;
}

/// Create a node with 4 descendants
Node *node4 (int a, Node *b, Node *c, Node *d, Node *e)
{
  Node *x;

  x = nodealloc (4);
  x->nobj = a;
  x->narg[0] = b;
  x->narg[1] = c;
  x->narg[2] = d;
  x->narg[3] = e;
  return x;
}

/// Create a statement node with 1 descendant
Node *stat1 (int a, Node *b)
{
  Node *x;

  x = node1 (a, b);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 2 descendants
Node *stat2 (int a, Node *b, Node *c)
{
  Node *x;

  x = node2 (a, b, c);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 3 descendants
Node *stat3 (int a, Node *b, Node *c, Node *d)
{
  Node *x;

  x = node3 (a, b, c, d);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 4 descendants
Node *stat4 (int a, Node *b, Node *c, Node *d, Node *e)
{
  Node *x;

  x = node4 (a, b, c, d, e);
  x->ntype = NSTAT;
  return x;
}

/// Create an expression node with 1 descendant
Node *op1 (int a, Node *b)
{
  Node *x;

  x = node1 (a, b);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 2 descendants
Node *op2 (int a, Node *b, Node *c)
{
  Node *x;

  x = node2 (a, b, c);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 3 descendants
Node *op3 (int a, Node *b, Node *c, Node *d)
{
  Node *x;

  x = node3 (a, b, c, d);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 4 descendants
Node *op4 (int a, Node *b, Node *c, Node *d, Node *e)
{
  Node *x;

  x = node4 (a, b, c, d, e);
  x->ntype = NEXPR;
  return x;
}

Node *celltonode (Cell *a, int b)
{
  Node *x;

  a->ctype = OCELL;
  a->csub = b;
  x = node1 (0, (Node *)a);
  x->ntype = NVALUE;
  return x;
}

Node *rectonode (void)  /* make $0 into a Node */
{
  extern Cell *literal0;
  return op1 (INDIRECT, celltonode (literal0, CUNK));
}

Node *makearr (Node *p)
{
  Cell *cp;

  if (isvalue (p))
  {
    cp = (Cell *)(p->narg[0]);
    if (isfcn (cp))
      SYNTAX ("%s is a function, not an array", cp->nval);
    else if (!isarr (cp))
    {
      xfree (cp->sval);
      cp->sval = (char *)makearray (NSYMTAB);
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

  x = node4 (PASTAT2, a, b, c, itonp (paircnt));
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

  if (isarr (v))
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
  return (int)(long)p;  /* swearing that p fits, of course */
}

Node *itonp (int i)  /* and vice versa */
{
  return (Node *)(long)i;
}
