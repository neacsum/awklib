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
#include <awklib/err.h>

#include "awk.h"
#include "ytab.h"
#include "proto.h"

using namespace std;

extern Interpreter* interp;
extern Cell* literal0;
extern Cell* literal_null;

#ifndef NDEBUG
const char *tokname (int tok);
int node_count = 0;
#endif


Node::Node ()
  : nnext {0}
  , lineno {0}
  , ntype {NVALUE}
  , tokid {0}
  , iarg {0}
{
#ifndef NDEBUG
  dprintf ("Allocated nodes = %d\n", (id = ++node_count));
#endif
}

/// Create a node with one descendant 
Node::Node (int id, pfun fn, int i, Node* arg1)
  : Node ()
{
  tokid = id;
  proc = fn;
  arg.push_back (unique_ptr<Node> (arg1));
  iarg = i;
}

/// Create a node with two descendants 
Node::Node (int id, pfun fn, int i, Node* arg1, Node* arg2)
  : Node ()
{
  tokid = id;
  proc = fn;
  arg.push_back (unique_ptr<Node> (arg1));
  arg.push_back (unique_ptr<Node> (arg2));
  iarg = i;
}

/// Create a node with three descendants 
Node::Node (int id, pfun fn, int i, Node* arg1, Node* arg2, Node* arg3)
  : Node ()
{
  tokid = id;
  proc = fn;
  arg.push_back (unique_ptr<Node> (arg1));
  arg.push_back (unique_ptr<Node> (arg2));
  arg.push_back (unique_ptr<Node> (arg3));
  iarg = i;
}

Node::~Node ()
{
#ifndef NDEBUG
    dprintf ("%Deleting node 0x%p %s with %d arguments (line %d)",
      this, tokname (tokid), arg.size (), lineno);
    dprintf (" type %s\n", ntype == NVALUE ? "value" : ntype == NSTAT ? "statement" : "expression");
#endif
    if (ntype == NVALUE)
    {
      Cell* x = (Cell*)arg[0].release();
      // Do not delete cell. It belongs to symtab...
      if (x->isregex ()) //...unless it's a regex
        delete x; //regular expressions are not part of symtab (for now)
    }
    else
    {
      for (int i = 0; i < arg.size (); i++)
        arg[i].reset ();
    }
    delete nnext;
#ifndef NDEBUG
    dprintf ("Remaining nodes = %d\n", --node_count);
#endif
}

/// Create a node with four descendants 
Node::Node (int id, pfun fn, int i, Node* arg1, Node* arg2, Node* arg3, Node* arg4)
  : Node ()
{
  tokid = id;
  proc = fn;
  arg.push_back (unique_ptr<Node> (arg1));
  arg.push_back (unique_ptr<Node> (arg2));
  arg.push_back (unique_ptr<Node> (arg3));
  arg.push_back (unique_ptr<Node> (arg4));
  iarg = i;
}

/// Convert node to a statement node (presumably from an expression node)
Node *exptostat (Node *a)
{
  a->ntype = NSTAT;
  return a;
}

/// Create a statement node with 1 descendant
Node *stat1 (int tokid, pfun fn, Node *arg1, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 2 descendants
Node *stat2 (int tokid, pfun fn, Node *arg1, Node *arg2, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1, arg2);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 3 descendants
Node *stat3 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1, arg2, arg3);
  x->ntype = NSTAT;
  return x;
}

/// Create a statement node with 4 descendants
Node *stat4 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, Node *arg4, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1, arg2, arg3, arg4);
  x->ntype = NSTAT;
  return x;
}

/// Create an expression node with 1 descendant
Node *op1 (int tokid, pfun fn, Node *arg1, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 2 descendants
Node *op2 (int tokid, pfun fn, Node *arg1, Node *arg2, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1, arg2);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 3 descendants
Node *op3 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1, arg2, arg3);
  x->ntype = NEXPR;
  return x;
}

/// Create an expression node with 4 descendants
Node *op4 (int tokid, pfun fn, Node *arg1, Node *arg2, Node *arg3, Node *arg4, int iarg)
{
  Node *x;

  x = new Node (tokid, fn, iarg, arg1, arg2, arg3, arg4);
  x->ntype = NEXPR;
  return x;
}

Node *celltonode (Cell *a, Cell::type t, int flags)
{
  Node *x;

  a->ctype = t;
  a->flags |= flags;
  x = new Node (0, nullproc, 0, (Node *)a);
  x->ntype = NVALUE;
  return x;
}

Node *rectonode ()  /* make $0 into a Node */
{
  return op1 (INDIRECT, indirect, celltonode (literal0));
}

Node* nullnode ()  /* zero&null, converted into a node for comparisons */
{
  return celltonode (literal_null);
}

/*TODO: Check if could use NF as an array. That would be bad!! */

Node* makearr (Node* p)
{
  if (p->isvalue ())
  {
    Cell* cp = p->to_cell ();
    if (cp->isfcn ())
      SYNTAX ("%s is a function, not an array", cp->nval);
    else if (!cp->isarr ())
    {
      cp->makearray();
      dprintf ("%s is now an array\n", cp->nval);
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

  x = new Node (PASTAT2, dopa2, paircnt, a, b, c);
  if (paircnt++ >= PA2NUM)
    SYNTAX ("limited to %d pat,pat statements", PA2NUM);
  x->ntype = NSTAT;
  return x;
}

Node *linkum (Node *a, Node *b)
{
  Node *c;

  if (interp->err)  /* don't link things that are wrong */
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
    SYNTAX ("`%s' is an array name and a function name", v->nval.c_str());
    return;
  }
  if (isarg (v->nval.c_str()) != -1)
  {
    SYNTAX ("`%s' is both function name and argument name", v->nval.c_str());
    return;
  }

  v->ctype = Cell::type::CFUNC;
  v->funptr = st;
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
    if (strcmp (((Cell *)(p->arg[0].get()))->nval.c_str(), s) == 0)
      return n;
  return -1;
}
