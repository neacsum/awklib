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
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <sys/wait.h>
#endif

#include "awk.h"
#include "ytab.h"
#include "proto.h"

#include <awklib/err.h>
#include <awklib/awk.h>

using namespace std;

extern  int  pairstack[];
extern  Awkfloat  srand_seed;
extern Interpreter* interp;

static Cell  truecell ("true", Cell::type::BTRUE, NUM, 1.);
Cell  *True  = &truecell;
static Cell  falsecell ("false", Cell::type::BFALSE, NUM, 0.);
Cell  *False  = &falsecell;
static Cell  breakcell  ("break", Cell::type::JBREAK);
Cell  *jbreak  = &breakcell;
static Cell  contcell ("continue", Cell::type::JCONT);
Cell  *jcont  = &contcell;
static Cell  nextcell ("next", Cell::type::JNEXT);
Cell  *jnext  = &nextcell;
static Cell  nextfilecell ("nextfile", Cell::type::JNEXTFILE);
Cell  *jnextfile  = &nextfilecell;
static Cell  exitcell ("exit", Cell::type::JEXIT, NUM);
Cell  *jexit  = &exitcell;
static Cell  retcell ("return", Cell::type::JRET);
Cell  *jret  = &retcell;


static int    format (char **, size_t *, const char *, Node *);
static Cell*  gettemp ();

/*! 
  Buffer memory management
  \par pbuf    - address of pointer to buffer being managed
  \par psiz    - address of buffer size variable
  \par minlen  - minimum length of buffer needed
  \par quantum - buffer size quantum
  \par pbptr   - address of movable pointer into buffer (can be NULL)
*/
void adjbuf (char **pbuf, size_t *psiz, size_t minlen, int quantum, char **pbptr)
{
  if (minlen > *psiz)
  {
    char *tbuf;
    int rminlen = quantum ? minlen % quantum : 0;
    size_t boff = pbptr ? *pbptr - *pbuf : 0;
    /* round up to next multiple of quantum */
    if (rminlen)
      minlen += quantum - rminlen;
    tbuf = new char[minlen];
    memcpy (tbuf, pbuf, *psiz);
    delete *pbuf;
    *pbuf = tbuf;
    *psiz = minlen;
    if (pbptr)
      *pbptr = tbuf + boff;
  }
}


inline int notlegal (const Node* n)
{
  return n->tokid <= FIRSTTOKEN
      || n->tokid >= LASTTOKEN
      || n->proc == nullproc;
}

/// Execute a node of the parse tree
Cell *execute (const Node* u)
{
  Cell *x;
  if (u == NULL)
    return True;

  for (const Node* a = u; ; a = a->nnext)
  {
    if (a->isvalue())
    {
      x = a->to_cell();
      if (x->isfld () && !interp->donefld)
        interp->fldbld ();
      else if (x->isrec () && !interp->donerec)
        interp->recbld ();

      return x;
    }
    if (notlegal (a))  /* probably a Cell* but too risky to print */
      FATAL (AWK_ERR_SYNTAX, "illegal statement");
   
    x = (*a->proc)(a->arg, a->iarg);
    if (x->isfld () && !interp->donefld)
      interp->fldbld ();
    else if (x->isrec () && !interp->donerec)
      interp->recbld ();
    if (a->ntype == NEXPR)
      return x;
    if (x->isjump ())
      return x;
    if (a->nnext == NULL)
      return x;
    tempfree (x);
  }
}

/// Execute an awk program
//Cell *program (Node **a, int n)
Cell* program (const Node::Arguments& a, int n)
{        /* a[0] = BEGIN, a[1] = body, a[2] = END */
  Cell* x;
  int exit_seen = 0;

  if (a[0])
  {    /* BEGIN */
    x = execute (a[0]);
    if (x->isexit ())
      exit_seen = 1;
    else if (x->isjump ())
      FATAL (AWK_ERR_SYNTAX, "illegal break, continue, next or nextfile from BEGIN");
    tempfree (x);
  }
  if (!exit_seen && (a[1] || a[2]))
  {
    while (interp->getrec (interp->fldtab[0].get()))
    {
      x = execute (a[1]);
      if (x->isexit ())
        break;
      tempfree (x);
    }
  }
  if (a[2])
  {    /* END */
    x = execute (a[2]);
    if (x->isbreak () || x->isnext () || x->iscont ())
      FATAL (AWK_ERR_SYNTAX, "illegal break, continue, next or nextfile from END");
    tempfree (x);
  }
  return True;
}


/*!
  Function call
  a[0] - function, a[1] - args
*/
Cell *call (const Node::Arguments& a, int n)
{
  int i, ncall, ndef;
  Node *x;
  Cell *y;
  Frame frm;
  Cell **callpar = NULL;

  memset (&frm, 0, sizeof (frm));
  frm.fcn = execute (a[0]);  /* the function itself */
  if (!frm.fcn->isfcn ())
    FATAL (AWK_ERR_RUNTIME, "calling undefined function %s", frm.fcn->nval.c_str());
  for (ncall = 0, x = a[1].get(); x != NULL; x = x->nnext)  /* args in call */
    ncall++;
  ndef = (int)frm.fcn->fval;      /* args in defn */
  dprintf ("calling %s, %d args (%d in defn)\n", frm.fcn->nval.c_str(), ncall, ndef);
  if (ncall > ndef)
    WARNING ("function %s called with %d args, uses only %d",
      frm.fcn->nval.c_str(), ncall, ndef);

  frm.args = 0;
  if (ndef)
  {
    frm.args = (Cell**)calloc (ndef, sizeof (Cell*));
    callpar = (Cell**)calloc (ndef, sizeof(Cell*));
  }

  for (i = 0, x = a[1].get(); x != NULL; i++, x = x->nnext)
  {  /* get call args */
    y = execute (x);
    dprintf ("args[%d]: %s %f <%s>, t=%s\n",
      i, y->nval.c_str(), y->fval, y->isarr () ? "(array)" : y->sval.c_str(), flags2str (y->flags));
    if (y->isfcn ())
      FATAL (AWK_ERR_RUNTIME, "can't use function %s as argument in %s", y->nval.c_str(), frm.fcn->nval);
    if (i < ndef)  // used arguments are stored in args array the rest are just
    {              // evaluated (for eventual side effects)
      callpar[i] = y;
      if (y->isarr ())
        frm.args[i] = y;  // arrays by ref
      else
      {
        frm.args[i] = new Cell ();
        frm.args[i]->nval = y->nval;
        switch (y->flags & (NUM | STR))
        {
        case STR:
          dprintf ("STR arg\n");
          frm.args[i]->sval = y->sval;
          frm.args[i]->flags = STR;
          break;
        case NUM:
          frm.args[i]->fval = y->fval;
          frm.args[i]->flags = NUM;
          break;
        case (NUM|STR):
          frm.args[i]->sval = y->sval;
          frm.args[i]->fval = y->fval;
          frm.args[i]->flags = NUM | STR;
          break;
        case 0:
          funnyvar (y, "argument");
          frm.args[i]->flags = STR;
          break;
        }
      }
    }
    tempfree (y);
  }

  for (; i < ndef; i++)
  {  /* add null args for ones not provided */
    frm.args[i] = new Cell ();
    frm.args[i]->flags = STR;
  }

  frm.nargs = ndef;
  frm.ret = gettemp();

  //save previous function call frame
  Frame prev = interp->fn;

  //and replace it with current frame
  interp->fn = frm;

  dprintf ("start exec of %s\n", frm.fcn->nval.c_str());
  if (frm.fcn->ctype == Cell::type::EXTFUNC)
  {
    //set args for external function
    awksymb *extargs = (awksymb *)calloc (ndef, sizeof (awksymb));
    for (i = 0; i < ndef; i++)
    {
      if (frm.args[i]->isarr())
      {
        extargs[i].flags = AWKSYMB_ARR;
        extargs[i].name = frm.args[i]->nval.c_str();
      }
      else
      {
        extargs[i].sval = const_cast<char*>(frm.args[i]->getsval ());
        extargs[i].fval = frm.args[i]->getfval ();
        extargs[i].flags = AWKSYMB_NUM | AWKSYMB_STR;
      }
    }
    awksymb extret{ 0,0,0,0.,0 };

    //call external function
    ((awkfunc)frm.fcn->funptr) ((AWKINTERP*)interp, &extret, ndef, extargs);
    y = gettemp ();
    free (extargs);
    if (extret.flags & AWKSYMB_STR)
      frm.ret->setsval (extret.sval);
    if (extret.flags & AWKSYMB_NUM)
      frm.ret->setfval (extret.fval);
  }
  else
    y = execute (frm.fcn->nodeptr);  /* execute body */
  dprintf ("finished exec of %s\n", frm.fcn->nval.c_str());
  interp->fn = prev;    //restore previous function frame
  for (i = 0; i < ndef; i++)
  {
    Cell* t = frm.args[i];
    if (t->isarr())
    {
      //array coming out of the function
      if (callpar[i] && !callpar[i]->isarr())
      {
        //scalar coming in
        callpar[i]->sval.clear ();
        callpar[i]->flags |= ARR;
        callpar[i]->arrval = t->arrval;
        continue;
      }
    }
    else
      delete t;
  }
  xfree (frm.args);
  xfree (callpar);
  if (y->isexit () || y->isnext ())
  {
    delete frm.ret;
    return y;
  }

  if ((frm.ret->flags & (NUM | STR)) == 0)
  {
    //no return statement
    frm.ret->flags = NUM | STR;
  }
  dprintf ("%s returns %g |%s| %s\n", frm.fcn->nval.c_str(), frm.ret->getfval (),
    frm.ret->getsval (), flags2str (frm.ret->flags));
  return frm.ret;
}

/// nth argument of a function
Cell *arg (const Node::Arguments& a, int n)
{
  dprintf ("arg(%d), nargs=%d\n", n, interp->fn.nargs);
  if (n + 1 > interp->fn.nargs)
    FATAL (AWK_ERR_RUNTIME, "argument #%d of function %s was not supplied",
      n + 1, interp->fn.fcn->nval.c_str());
  return interp->fn.args[n];
}

/// break, continue, next, nextfile, return
Cell *jump (const Node::Arguments& a, int n)
{
  Cell *y;

  switch (n)
  {
  case EXIT:
    if (a[0] != NULL)
    {
      y = execute (a[0]);
      interp->err = (int)y->getfval ();
      tempfree (y);
    }
    return jexit;

  case RETURN:
    if (a[0] != NULL)
    {
      Cell *pr = interp->fn.ret;
      y = execute (a[0]);
      if ((y->flags & (NUM | STR)) == 0)
        funnyvar (y, "return");

      pr->flags = 0;
      if (y->flags & NUM)
      {
        pr->fval = y->getfval ();
        pr->flags |= NUM;
      }
      if (y->flags & STR)
      {
        pr->setsval (y->getsval ());
        pr->flags |= STR;
      }
      tempfree (y);
    }
    return jret;

  case NEXT:
    return jnext;

  case NEXTFILE:
    interp->nextfile ();
    return jnextfile;

  case BREAK:
    return jbreak;

  case CONTINUE:
    return jcont;

  default:  /* can't happen */
    FATAL (AWK_ERR_OTHER, "illegal jump type %d", n);
  }
  return 0;  /* not reached */
}

/// Get next line from specific input
Cell *awkgetline (const Node::Arguments& a, int n)
{
  /* a[0] is variable, a[1] is filename */
  Cell *r, *x;
  FILE *fp;
  int mode, c;

  r = a[0] ? execute (a[0]) : interp->fldtab[0].get();

  fflush (interp->files[1].fp);  /* in case someone is waiting for a prompt */
  if (a[2])
  {    /* getline < file */
    x = execute (a[1]);    /* filename */
    mode = n;
    if (mode == '|')    /* input pipe */
      mode = LE;  /* arbitrary flag */
    fp = openfile (mode, x->getsval ());
    tempfree (x);
    if (fp == NULL)
      c = 0;
    else
      c = interp->readrec (r, fp);
  }
  else
  {      /* bare getline; use current input */
    c = interp->getrec (r);
  }
  if (c && is_number (r->sval))
  {
    r->fval = stof (r->sval);
    r->flags |= NUM;
  }
  if (r->isrec())
  {
    interp->donerec = true;
    interp->fldbld ();
  }
  return r;
}

/// get NF
Cell *getnf (const Node::Arguments& a, int)
{
  if (!interp->donefld)
    interp->fldbld ();
  Cell *pnf = execute (a[0]);
  return pnf;
}

Cell *array (const Node::Arguments& a, int)
{
  /* a[0] array, a[1] is list of subscripts */
  Cell *x, *y, *z;
  Node *np;

  x = execute (a[0]);  /* array */
  string subscript;
  for (np = a[1].get(); np; np = np->nnext)
  {
    y = execute (np);  /* subscript */
    subscript += y->getsval ();
    if (np->nnext)
      subscript += SUBSEP;
    tempfree (y);
  }
  if (!x->isarr ())
  {
    dprintf ("making %s into an array\n", x->nval);
    x->makearray ();
  }
  z = x->arrval->setsym (subscript.c_str(), "", 0.0, STR | NUM);
  tempfree (x);
  return z;
}

Cell *awkdelete (const Node::Arguments& a, int)
{
  /* a[0] is array, a[1] is list of subscripts */
  Cell *x, *y;
  Node *np;
  size_t nsub = SUBSEP.size ();

  x = execute (a[0]);  /* Cell* for array */
  if (!x->isarr ())
    return True;
  if (!a[1])
  {
    //replace with a clean array
    delete x->arrval;
    x->arrval = new Array (NSYMTAB);
  }
  else
  {
    string sub;
    for (np = a[1].get(); np; np = np->nnext)
    {
      y = execute (np);  /* subscript */
      sub += y->getsval ();
      if (np->nnext)
        sub += SUBSEP;
      tempfree (y);
    }
    delete x->arrval->removesym (sub);
  }
  tempfree (x);

  return True;
}

Cell *intest (const Node::Arguments& a, int)
{
  /* a[0] is index (list), a[1] is array */
  Cell *x, *ap, *k;
  Node *p;
  size_t bufsz = RECSIZE;
  size_t nsub = SUBSEP.size();

  ap = execute (a[1]);  /* array name */
  if (!ap->isarr ())
  {
    dprintf ("making %s into an array\n", ap->nval);
    ap->makearray ();
  }

  string sub;
  for (p = a[0].get(); p; p = p->nnext)
  {
    x = execute (p);  /* expr */
    sub += x->getsval ();
    if (p->nnext)
      sub += SUBSEP;
  }
  k = ap->arrval->lookup (sub.c_str());
  tempfree (ap);
  if (k == NULL)
    return False;
  else
    return True;
}

/*
  ~  and !~ operators
    a[0] = target text
    a[1] = already compiled regex
    a[2] = regex string

    Only one of a[1] and a[2] can be not NULL
*/
Cell *matchop (const Node::Arguments& a, int n)
{
  Cell *x, *y;
  bool found;

  x = execute (a[0]);  /* a[0] = target text */
  if (a[1])    /* a[1] != 0: already-compiled reg expr */
    found = a[1]->to_cell ()->match(x->getsval ());
  else
  {
    y = execute (a[2]);  /* a[2] = regular expr */
    Cell* re = interp->makedfa (y->getsval (), false);
    found = re->match(x->getsval());
    tempfree (y);
  }
  tempfree (x);
  if ((n == MATCH && found) || (n == NOTMATCH && !found))
    return True;
  else
    return False;
}

/*
  match ()
    a[0] = target text
    a[1] = already compiled regex
    a[2] = regex string

    Only one of a[1] and a[2] can be not NULL
*/
Cell* matchfun (const Node::Arguments& a, int)
{
  Cell* x, * y;
  bool found;
  size_t patbeg, patlen;

  x = execute (a[0]);  /* a[0] = target text */
  const char *s = x->getsval ();
  if (a[1])    /* a[1] != 0: already-compiled reg expr */
    found = a[1]->to_cell ()->pmatch(s, patbeg, patlen);
  else
  {
    y = execute (a[2]);  /* a[2] = regular expr */
    Cell* re = interp->makedfa (y->getsval (), false);
    found = re->pmatch(s, patbeg, patlen);
    tempfree (y);
  }
  tempfree (x);
  if (found)
  {
    RSTART = (Awkfloat)(patbeg + 1);
    RLENGTH = (Awkfloat)patlen;
  }
  else
    RSTART = 0.;

  x = gettemp ();
  x->flags |= NUM;
  x->fval = RSTART;
  return x;
}

///  Boolean operations a[0] || a[1], a[0] && a[1], !a[0]
Cell *boolop (const Node::Arguments& a, int n)
{
  Cell *x, *y;
  bool i;

  x = execute (a[0]);
  i = x->istrue ();
  tempfree (x);
  switch (n)
  {
  case BOR:
    if (i) return(True);
    y = execute (a[1]);
    i = y->istrue ();
    tempfree (y);
    return i? True : False;

  case AND:
    if (!i) return(False);
    y = execute (a[1]);
    i = y->istrue ();
    tempfree (y);
    return i? True : False;

  case NOT:
    return i? False : True;

  default:  /* can't happen */
    FATAL (AWK_ERR_OTHER, "unknown boolean operator %d", n);
  }
  return 0;  /*NOTREACHED*/
}

/// Relational operators  a[0] < a[1], etc.
Cell *relop (const Node::Arguments& a, int n)
{
  int i;
  Cell *x, *y;
  Awkfloat j;

  x = execute (a[0]);
  y = execute (a[1]);
  if ((x->flags & NUM) && (y->flags & NUM))
  {
    j = x->fval - y->fval;
    i = j < 0 ? -1 : (j > 0 ? 1 : 0);
  }
  else
  {
    i = strcmp (x->getsval (), y->getsval ());
  }
  tempfree (x);
  tempfree (y);
  switch (n)
  {
  case LT:  return (i < 0)? True : False;
  case LE:  return (i <= 0)? True : False;
  case NE:  return (i != 0)? True : False;
  case EQ:  return (i == 0)? True : False;
  case GE:  return (i >= 0)? True : False;
  case GT:  return (i > 0)? True : False;

  default:  /* can't happen */
    FATAL (AWK_ERR_OTHER, "unknown relational operator %d", n);
  }
  return 0;  /*NOTREACHED*/
}

/// Free a tempcell
void tempfree (Cell *a)
{
  if (a->ctype == Cell::type::CTEMP)
  {
    dprintf ("freeing %s %s %s\n", a->nval.c_str(), quote (a->sval).c_str (), flags2str (a->flags));
    delete a;
  }
}

///  Get a tempcell
Cell *gettemp ()
{
  Cell *p =  new Cell (nullptr, Cell::type::CTEMP);
  return p;
}

/// $( a[0] )
Cell *indirect (const Node::Arguments& a, int)
{
  Awkfloat val;
  Cell *x;
  int m;

  x = execute (a[0]);
  val = x->getfval ();  /* freebsd: defend against super large field numbers */
  if ((Awkfloat)INT_MAX < val)
    FATAL (AWK_ERR_LIMIT, "trying to access out of range field %s", x->nval);
  m = (int)val;
#if 0
  if (m == 0 && !is_number (s = getsval (x)))  /* suspicion! */
    FATAL (AWK_ERR_ARG, "illegal field $(%s), name \"%s\"", s, x->nval);
#endif
  tempfree (x);
  x = interp->fieldadr (m);
  return x;
}

/// substr(a[0], a[1], a[2])
Cell *substr (const Node::Arguments& a, int)
{
  const char *s;
  char *ss;
  size_t n, m, k;
  Cell *x, *y, *z = 0;

  x = execute (a[0]);
  y = execute (a[1]);
  if (a[2])
    z = execute (a[2]);
  s = x->getsval ();
  k = strlen (s) + 1;
  if (k <= 1)
  {
    tempfree (x);
    tempfree (y);
    if (a[2] != 0)
      tempfree (z);
    x = gettemp ();
    x->setsval ("");
    return x;
  }
  int iy = (int)y->getfval ();
  if (iy <= 0)
    m = 1;
  else if ((size_t)iy > k)
    m = k;
  else
    m = iy;
  tempfree (y);

  if (a[2] != 0)
  {
    int iz = (int)z->getfval ();
    if (iz < 0)
      n = 0;
    else
      n = iz;
    tempfree (z);
  }
  else
    n = k - 1;
  if (n > k - m)
    n = k - m;
  dprintf ("substr: m=%zd, n=%zd, s=%s\n", m, n, s);
  y = gettemp ();
  ss = strdup (s);
  ss[n + m - 1] = '\0';
  y->setsval (&ss[m - 1]);
  free (ss);
  tempfree (x);
  return(y);
}

/// index(a[0], a[1])
Cell *sindex (const Node::Arguments& a, int)
{
  Cell *x, *y, *z;
  const char *s1, *s2, *p1, *p2, *q;
  Awkfloat v = 0.0;

  x = execute (a[0]);
  s1 = x->getsval ();
  y = execute (a[1]);
  s2 = y->getsval ();

  z = gettemp ();
  for (p1 = s1; *p1 != '\0'; p1++)
  {
    for (q = p1, p2 = s2; *p2 != '\0' && *q == *p2; q++, p2++)
      ;
    if (*p2 == '\0')
    {
      v = (Awkfloat)(p1 - s1 + 1);  /* origin 1 */
      break;
    }
  }
  tempfree (x);
  tempfree (y);
  z->setfval (v);
  return z;
}

#define  MAXNUMSIZE  50

/// printf-like conversions
int format (char **pbuf, size_t *pbufsize, const char *s, Node *a)
{
  char *fmt;
  char *p, *t;
  const char *os;
  Cell *x;
  int flag = 0;
  size_t n;
  int fmtwd; /* format width */
  size_t fmtsz = RECSIZE;
  char *buf = *pbuf;
  size_t bufsize = *pbufsize;

  static int first = 1;
  static int have_a_format = 0;

  if (first)
  {
    char buf[100];

    sprintf (buf, "%a", 42.0);
    have_a_format = (strcmp (buf, "0x1.5p+5") == 0);
    first = 0;
  }

  os = s;
  p = buf;
  fmt = new char[fmtsz];
  while (*s)
  {
    adjbuf (&buf, &bufsize, MAXNUMSIZE + 1 + p - buf, RECSIZE, &p);
    if (*s != '%')
    {
      *p++ = *s++;
      continue;
    }
    if (*(s + 1) == '%')
    {
      *p++ = '%';
      s += 2;
      continue;
    }
    /* have to be real careful in case this is a huge number, eg, %100000d */
    fmtwd = atoi (s + 1);
    if (fmtwd < 0)
      fmtwd = -fmtwd;
    adjbuf (&buf, &bufsize, fmtwd + 1 + p - buf, RECSIZE, &p);
    for (t = fmt; (*t++ = *s) != '\0'; s++)
    {
      adjbuf (&fmt, &fmtsz, MAXNUMSIZE + 1 + t - fmt, RECSIZE, &t);
      if (isalpha (*s) && *s != 'l' && *s != 'h' && *s != 'L')
        break;  /* the ansi panoply */
      if (*s == '$')
        FATAL (AWK_ERR_ARG, "'$' not permitted in awk formats");
      if (*s == '*')
      {
        x = execute (a);
        a = a->nnext;
        sprintf (t - 1, "%d", fmtwd = (int)x->getfval ());
        if (fmtwd < 0)
          fmtwd = -fmtwd;
        adjbuf (&buf, &bufsize, fmtwd + 1 + p - buf, RECSIZE, &p);
        t = fmt + strlen (fmt);
        tempfree (x);
      }
    }
    *t = '\0';
    if (fmtwd < 0)
      fmtwd = -fmtwd;
    adjbuf (&buf, &bufsize, fmtwd + 1 + p - buf, RECSIZE, &p);
    switch (*s)
    {
    case 'a': case 'A':
      if (have_a_format)
        flag = *s;
      else
        flag = 'f';
      break;
    case 'f': case 'e': case 'g': case 'E': case 'G':
      flag = 'f';
      break;
    case 'd': case 'i':
      flag = 'd';
      if (*(s - 1) == 'l') break;
      *(t - 1) = 'l';
      *t = 'd';
      *++t = '\0';
      break;
    case 'o': case 'x': case 'X': case 'u':
      flag = *(s - 1) == 'l' ? 'd' : 'u';
      break;
    case 's':
      flag = 's';
      break;
    case 'c':
      flag = 'c';
      break;
    default:
      WARNING ("weird printf conversion %s", fmt);
      flag = '?';
      break;
    }
    if (a == NULL)
      FATAL (AWK_ERR_ARG, "not enough args in printf(%s)", os);
    x = execute (a);
    a = a->nnext;
    n = MAXNUMSIZE;
    if ((size_t)fmtwd > n)
      n = fmtwd;
    adjbuf (&buf, &bufsize, 1 + n + p - buf, RECSIZE, &p);
    switch (flag)
    {
    case '?':
      sprintf (p, "%s", fmt);  /* unknown, so dump it too */
      os = x->getsval ();
      n = strlen (os);
      if ((size_t)fmtwd > n)
        n = fmtwd;
      adjbuf (&buf, &bufsize, 1 + strlen (p) + n + p - buf, RECSIZE, &p);
      p += strlen (p);
      sprintf (p, "%s", os);
      break;
    case 'a':
    case 'A':
    case 'f':  sprintf (p, fmt, x->getfval ()); break;
    case 'd':  sprintf (p, fmt, (long)x->getfval ()); break;
    case 'u':  sprintf (p, fmt, (int)x->getfval ()); break;
    case 's':
      os = x->getsval ();
      n = strlen (os);
      if ((size_t)fmtwd > n)
        n = fmtwd;
      adjbuf (&buf, &bufsize, 1 + n + p - buf, RECSIZE, &p);
      sprintf (p, fmt, os);
      break;
    case 'c':
      if (x->flags & NUM)
      {
        if (x->getfval ())
          sprintf (p, fmt, (int)x->getfval ());
        else
        {
          *p++ = '\0'; /* explicit null byte */
          *p = '\0';   /* next output will start here */
        }
      }
      else
        sprintf (p, fmt, x->getsval ()[0]);
      break;
    default:
      FATAL (AWK_ERR_OTHER, "can't happen: bad conversion %c in format()", flag);
    }
    tempfree (x);
    p += strlen (p);
    s++;
  }
  *p = '\0';
  delete fmt;
  for (; a; a = a->nnext)    /* evaluate any remaining args */
    execute (a);
  *pbuf = buf;
  *pbufsize = bufsize;
  return (int)(p - buf);
}

/// sprintf(a[0])
Cell *awksprintf (const Node::Arguments& a, int)
{
  Cell *x;
  Node *y;
  char *buf;
  size_t bufsz = RECSIZE;

  buf = new char[bufsz];
  y = a[0]->nnext;
  x = execute (a[0]);
  if (format (&buf, &bufsz, x->getsval (), y) == -1)
    FATAL (AWK_ERR_NOMEM, "sprintf string %.30s... too long.  can't happen.", buf);
  tempfree (x);
  x = gettemp ();
  x->sval = tostring(buf);
  x->flags = STR;
  delete buf;
  return x;
}

/// printf
Cell *awkprintf (const Node::Arguments& a, int n)
{
  /* a[0] is list of args, starting with format string */
  /* a[1] is redirection file  */
  /* n is redirection operator */
  FILE *fp;
  Cell *x;
  Node *y;
  int len;
  size_t bufsz = RECSIZE;
  char *buf = new char[bufsz];

  y = a[0]->nnext;
  x = execute (a[0]);
  if ((len = format (&buf, &bufsz, x->getsval (), y)) == -1)
    FATAL (AWK_ERR_NOMEM, "printf string %.30s... too long.  can't happen.", buf);
  tempfree (x);
  if (!n)
  {
    if (interp->outredir)
      interp->outredir (buf, len);
    else
    {
      fwrite (buf, len, 1, interp->files[1].fp);
      if (ferror (interp->files[1].fp))
        FATAL (AWK_ERR_OUTFILE, "write error on %s", interp->files[1].fname);
    }
  }
  else
  {
    fp = redirect (n, *a[1]);
    fwrite (buf, len, 1, fp);
    fflush (fp);
    if (ferror (fp))
      FATAL (AWK_ERR_OUTFILE, "write error on %s", filename (fp));
  }
  delete buf;
  return True;
}

/// Arithmetic operations   a[0] + a[1], etc.  also -a[0]
Cell *arith (const Node::Arguments& a, int n)
{
  Awkfloat i, j = 0;
  double v;
  Cell *x, *y, *z;

  x = execute (a[0]);
  i = x->getfval ();
  tempfree (x);
  if (n != UMINUS && n != UPLUS)
  {
    y = execute (a[1]);
    j = y->getfval ();
    tempfree (y);
  }
  z = gettemp ();
  switch (n)
  {
  case ADD:
    i += j;
    break;
  case MINUS:
    i -= j;
    break;
  case MULT:
    i *= j;
    break;
  case DIVIDE:
    if (j == 0)
      FATAL (AWK_ERR_RUNTIME, "division by zero");
    i /= j;
    break;
  case MOD:
    if (j == 0)
      FATAL (AWK_ERR_RUNTIME, "division by zero in mod");
    modf (i / j, &v);
    i = i - j * v;
    break;
  case UMINUS:
    i = -i;
    break;
  case UPLUS: /* handled by getfval(), above */
    break;
  case POWER:
    i = errcheck (pow (i, j), "pow");
    break;
  default:  /* can't happen */
    FATAL (AWK_ERR_OTHER, "illegal arithmetic operator %d", n);
  }
  z->setfval (i);
  return z;
}

/// Increment/decrement operators a[0]++, etc.
Cell *incrdecr (const Node::Arguments& a, int n)
{
  Cell *x, *z;
  int k;
  Awkfloat xf;

  x = execute (a[0]);
  xf = x->getfval ();
  k = (n == PREINCR || n == POSTINCR) ? 1 : -1;
  if (n == PREINCR || n == PREDECR)
  {
    x->setfval (xf + k);
    return(x);
  }
  z = gettemp ();
  z->flags = NUM;
  z->setfval (xf);
  x->setfval (xf + k);
  tempfree (x);
  return z;
}

/// a[0] = a[1], a[0] += a[1], etc.
Cell *assign (const Node::Arguments& a, int n)
{
  Cell *x, *y;

  y = execute (a[1]);
  if ((y->flags & (NUM | STR)) == 0)
    funnyvar (y, "assign");
  x = execute (a[0]);
  if ((x->flags & (NUM | STR)) == 0)
    funnyvar (x, "assign");
  if (n == ASSIGN)
  {
    /* ordinary assignment
      To save some time we don't call Cell's get... and set... functions but
      we must take care of side-effects if cells are fields.
    */
    if (x->isfld ())
    {
      int n = atoi (x->nval.c_str ());
      if (n > NF)
        interp->setlastfld (n);
      interp->donerec = false;
    }
    if (x->isrec ())
    {
      interp->donefld = false;
      interp->donerec = true;
    }
    if (x != y)  // leave alone self-assignment */
    {
      x->flags &= ~(NUM | STR);
      switch (y->flags & (STR | NUM))
      {
      case (NUM | STR):
        x->sval = y->sval;
        x->fval = y->fval;
        x->flags |= NUM | STR;
        break;
      case STR:
        x->sval = y->sval;
        x->flags |= STR;
        break;
      case NUM:
        x->fval = y->fval;
        x->flags |= NUM;
        break;
      case 0:
        x->sval.clear ();
        x->fval = 0.;
        break;
      }
    }
  }
  else
  {
    /* Other assignment operations (+=, -=, etc.)*/
    Awkfloat xf = x->getfval (),
      yf = y->getfval ();
    double v;
    switch (n)
    {
    case ADDEQ:
      xf += yf;
      break;
    case SUBEQ:
      xf -= yf;
      break;
    case MULTEQ:
      xf *= yf;
      break;
    case DIVEQ:
      if (yf == 0)
        FATAL (AWK_ERR_RUNTIME, "division by zero in /=");
      xf /= yf;
      break;
    case MODEQ:
      if (yf == 0)
        FATAL (AWK_ERR_RUNTIME, "division by zero in %%=");
      modf (xf / yf, &v);
      xf = xf - yf * v;
      break;
    case POWEQ:
      xf = errcheck (pow (xf, yf), "pow");
      break;
    default:
      FATAL (AWK_ERR_OTHER, "illegal assignment operator %d", n);
      break;
    }
    x->setfval (xf);
  }
  tempfree (y);
  return x;
}

/// Concatenation a[0] cat a[1]
Cell *cat (const Node::Arguments& a, int)
{
  Cell *x, *y, *z;

  x = execute (a[0]);
  y = execute (a[1]);

  z = gettemp ();
  z->sval = x->getsval() + string(y->getsval());
  z->flags = STR;
  tempfree (x);
  tempfree (y);
  return z;
}

/// Pattern-action statement a[0] { a[1] }
Cell *pastat (const Node::Arguments& a, int)
{
  Cell *x;

  if (!a[0])
    x = execute (a[1]);
  else
  {
    x = execute (a[0]);
    if (x->istrue ())
    {
      tempfree (x);
      x = execute (a[1]);
    }
  }
  return x;
}

/// Pattern range - action statement a[0], a[1] { a[2] }
Cell *dopa2 (const Node::Arguments& a, int n)
{
  Cell *x;
  int pair;

  pair = n;
  if (pairstack[pair] == 0)
  {
    x = execute (a[0]);
    if (x->istrue ())
      pairstack[pair] = 1;
    tempfree (x);
  }
  if (pairstack[pair] == 1)
  {
    x = execute (a[1]);
    if (x->istrue ())
      pairstack[pair] = 0;
    tempfree (x);
    x = execute (a[2]);
    return x;
  }
  return False;
}

/// Ternary operator ?:   a[0] ? a[1] : a[2]
Cell *condexpr (const Node::Arguments& a, int n)
{
  Cell *x;

  x = execute (a[0]);
  if (x->istrue ())
  {
    tempfree (x);
    x = execute (a[1]);
  }
  else
  {
    tempfree (x);
    x = execute (a[2]);
  }
  return x;
}

/// If statement  if (a[0]) a[1]; else a[2]
Cell *ifstat (const Node::Arguments& a, int)
{
  Cell *x;

  x = execute (a[0]);
  if (x->istrue ())
  {
    tempfree (x);
    x = execute (a[1]);
  }
  else if (a[2])
  {
    tempfree (x);
    x = execute (a[2]);
  }
  return(x);
}

/// While statement  while (a[0]) a[1]
Cell *whilestat (const Node::Arguments& a, int)
{
  Cell *x;

  for (;;)
  {
    x = execute (a[0]);
    if (!x->istrue ())
      return(x);
    tempfree (x);
    x = execute (a[1]);
    if (x->isbreak ())
      return True;

    if (x->isnext () || x->isexit () || x->isret ())
      return x;
    tempfree (x);
  }
}

/// Do statement  do a[0]; while(a[1])
Cell *dostat(const Node::Arguments& a, int)
{
  Cell *x;

  for (;;)
  {
    x = execute(a[0]);
    if (x->isbreak())
      return True;
    if (x->isnext() || x->isexit() || x->isret())
      return x;
    tempfree(x);
    x = execute(a[1]);
    if (!x->istrue())
      return x;
    tempfree(x);
  }
}

/// For statement  for (a[0]; a[1]; a[2]) a[3]
Cell *forstat (const Node::Arguments& a, int)
{
  Cell *x;

  x = execute (a[0]);
  tempfree (x);
  for (;;)
  {
    if (a[1])
    {
      x = execute (a[1]);
      if (!x->istrue ()) return(x);
      else tempfree (x);
    }
    x = execute (a[3]);
    if (x->isbreak ())    /* turn off break */
      return True;
    if (x->isnext () || x->isexit () || x->isret ())
      return x;
    tempfree (x);
    x = execute (a[2]);
    tempfree (x);
  }
}

/// For ... in statement  for (a[0] in a[1]) a[2]
Cell *instat (const Node::Arguments& a, int)
{
  Cell *x, *vp, *arrayp;
  Array *tp;

  vp = execute (a[0]);
  arrayp = execute (a[1]);
  if (!arrayp->isarr ())
    return True;

  tp = arrayp->arrval;
  tempfree (arrayp);
  for (Array::Iterator cp = tp->begin(); cp != tp->end(); cp++)
  {
    vp->setsval (cp->nval.c_str());
    x = execute (a[2]);
    if (x->isbreak ())
    {
      tempfree (vp);
      return True;
    }
    if (x->isnext () || x->isexit () || x->isret ())
    {
      tempfree (vp);
      return x;
    }
    tempfree (x);
  }
  return True;
}

/// Builtin function (sin, cos, etc.)  n is type, a[0] is arg list
Cell *bltin (const Node::Arguments& a, int n)
{
  Cell *x, *y;
  Awkfloat u;
  Awkfloat tmp;
  char *p, *buf;
  Node *nextarg;
  FILE *fp;
  void flush_all (void);
  int status = 0;

  x = execute (a[0]);
  nextarg = a[0]->nnext;
  switch (n)
  {
  case FLENGTH:
    if (x->isarr ())
      u = x->arrval->length();  /* GROT.  should be function*/
    else
      u = (Awkfloat)strlen (x->getsval ());
    break;
  case FLOG:
    u = errcheck (log (x->getfval ()), "log"); break;
  case FINT:
    modf (x->getfval (), &u); break;
  case FEXP:
    u = errcheck (exp (x->getfval ()), "exp"); break;
  case FSQRT:
    u = errcheck (sqrt (x->getfval ()), "sqrt"); break;
  case FSIN:
    u = sin (x->getfval ()); break;
  case FCOS:
    u = cos (x->getfval ()); break;
  case FATAN:
    if (nextarg == 0)
    {
      WARNING ("atan2 requires two arguments; returning 1.0");
      u = 1.0;
    }
    else
    {
      y = execute (a[1]->nnext);
      u = atan2 (x->getfval (), y->getfval ());
      tempfree (y);
      nextarg = nextarg->nnext;
    }
    break;
  case FSYSTEM:
    fflush (stdout);    /* in case something is buffered already */
    status = system (x->getsval ());
    u = status;
#ifndef _MSC_VER
    if (status != -1)
    {
      if (WIFEXITED (status))
      {
        u = WEXITSTATUS (status);
      }
      else if (WIFSIGNALED (status))
      {
        u = WTERMSIG (status) + 256;
#ifdef WCOREDUMP
        if (WCOREDUMP (status))
          u += 256;
#endif
      }
      else  /* something else?!? */
        u = 0;
    }
#endif
    break;
  case FRAND:
    /* normalize to (0..1) range */
    u = (Awkfloat)(interp->randgen ()) / ((Awkfloat)interp->randgen.max ()+1);
    break;
  case FSRAND:
    if (x->isrec ())  /* no argument provided */
      u = (Awkfloat)time ((time_t *)0);
    else
      u = x->getfval ();
    tmp = u;
    interp->randgen.seed ((unsigned int)u);
    u = (unsigned int)interp->rand_seed;
    interp->rand_seed = (unsigned int)tmp;
    break;
  case FTOUPPER:
  case FTOLOWER:
    buf = tostring (x->getsval ());
    if (n == FTOUPPER)
    {
      for (p = buf; *p; p++)
        if (islower (*p))
          *p = toupper (*p);
    }
    else
    {
      for (p = buf; *p; p++)
        if (isupper (*p))
          *p = tolower (*p);
    }
    tempfree (x);
    x = gettemp ();
    x->setsval (buf);
    delete buf;
    return x;
  case FFLUSH:
    if (x->isrec () || strlen (x->getsval ()) == 0)
    {
      flush_all ();  /* fflush() or fflush("") -> all */
      u = 0;
    }
    else if ((fp = openfile (FFLUSH, x->getsval ())) == NULL)
      u = EOF;
    else
      u = fflush (fp);
    break;
  default:  /* can't happen */
    FATAL (AWK_ERR_OTHER, "illegal function type %d", n);
    break;
  }
  tempfree (x);
  x = gettemp ();
  x->setfval (u);
  if (nextarg != 0)
  {
    WARNING ("warning: function has too many arguments");
    for (; nextarg; nextarg = nextarg->nnext)
      execute (nextarg);
  }
  return x;
}

/*!
  print 
    a[0] - print argument(s) (linked list)
    a[1] - file or NULL
    n - redirection operator (FFLUSH, GT, APPEND, PIPE) or NULL
*/
Cell *printstat (const Node::Arguments& a, int n)
{
  Node *x;
  Cell *y;
  FILE *fp;

  if (!n)  /* n is redirection operator, a[1] is file */
    fp = interp->files[1].fp;
  else
    fp = redirect (n, *a[1]);
  for (x = a[0].get(); x != NULL; x = x->nnext)
  {
    y = execute (x);
    interp->putstr (y->getpssval (), fp);
    tempfree (y);
    if (x->nnext == NULL)
      interp->putstr (ORS.c_str(), fp);
    else
      interp->putstr (OFS.c_str(), fp);
  }
  if (n)
    fflush (fp);
  if (ferror (fp))
    FATAL (AWK_ERR_OUTFILE, "write error on %s", filename (fp));

  return True;
}

Cell *nullproc (const Node::Arguments& a, int n)
{
  return 0;
}

/// Set up all i/o redirections
FILE *redirect (int a, Node& b)
{
  FILE *fp;
  Cell *x;
  const char *fname;

  x = execute (&b);
  fname = x->getsval ();
  fp = openfile (a, fname);
  tempfree (x);
  return fp;
}

FILE *openfile (int a, const char *us)
{
  const char *s = us;
  int i, m;
  FILE *fp = 0;

  if (*s == '\0')
    FATAL (AWK_ERR_ARG, "null file name in print or getline");
  for (i = 0; i < interp->nfiles; i++)
  {
    if (interp->files[i].fname && strcmp (s, interp->files[i].fname) == 0)
    {
      if (a == interp->files[i].mode || (a == APPEND && interp->files[i].mode == GT))
        return interp->files[i].fp;
      if (a == FFLUSH)
        return interp->files[i].fp;
    }
  }
  if (a == FFLUSH)  /* didn't find it, so don't create it! */
    return NULL;

  for (i = 0; i < interp->nfiles; i++)
    if (interp->files[i].fp == 0)
      break;
  if (i >= interp->nfiles)
  {
    struct FILE_STRUC *nf;
    int nnf = interp->nfiles + FOPEN_MAX;
    nf = (FILE_STRUC*)realloc (interp->files, nnf * sizeof (FILE_STRUC));
    if (nf == NULL)
      FATAL (AWK_ERR_NOMEM, "cannot grow files for %s and %d files", s, nnf);
    memset (&nf[interp->nfiles], 0, FOPEN_MAX * sizeof (*nf));
    interp->nfiles = nnf;
    interp->files = nf;
  }
  fflush (interp->files[1].fp);  /* force a semblance of order */
  m = a;
  if (a == GT)
  {
    fp = fopen (s, "w");
    if (!fp)
      FATAL (AWK_ERR_OUTFILE, "can't open file %s", s);
  }
  else if (a == APPEND) 
  {
    fp = fopen (s, "a");
    if (!fp)
      FATAL (AWK_ERR_OUTFILE, "can't open file %s", s);
    m = GT;  /* so can mix > and >> */
  }
  else if (a == '|')
  {  /* output pipe */
    fp = popen (s, "w");
    if (!fp)
      FATAL (AWK_ERR_PIPE, "can't output pipe %s", s);
  }
  else if (a == LE)
  {  /* input pipe */
    fp = popen (s, "r");
    if (!fp)
      FATAL (AWK_ERR_PIPE, "can't open input pipe %s", s);
  }
  else if (a == LT)
  {  /* getline <file */
    fp = strcmp (s, "-") == 0 ? stdin : fopen (s, "r");  /* "-" is stdin */
    if (!fp)
      FATAL (AWK_ERR_INFILE, "can't open file %s", s);
  }
  else  /* can't happen */
    FATAL (AWK_ERR_OTHER, "illegal redirection %d", a);

  interp->files[i].fname = tostring (s);
  interp->files[i].fp = fp;
  interp->files[i].mode = m;
  return fp;
}

const char *filename (FILE *fp)
{
  int i;

  for (i = 0; i < interp->nfiles; i++)
  {
    if (fp == interp->files[i].fp)
      return interp->files[i].fname;
  }
  return "???";
}

Cell *closefile (const Node::Arguments& a, int)
{
  Cell *x;
  int i, stat;

  x = execute (a[0]);
  x->getsval ();
  stat = -1;
  for (i = 0; i < interp->nfiles; i++)
  {
    if (interp->files[i].fname && x->sval == interp->files[i].fname)
    {
      if (ferror (interp->files[i].fp))
        WARNING ("i/o error occurred on %s", interp->files[i].fname);
      if (interp->files[i].mode == '|' || interp->files[i].mode == LE)
        stat = pclose (interp->files[i].fp);
      else
        stat = fclose (interp->files[i].fp);
      if (stat == EOF)
        WARNING ("i/o error occurred closing %s", interp->files[i].fname);
      if (i > 2)  /* don't do /dev/std... */
        xfree (interp->files[i].fname);
      interp->files[i].fname = NULL;  /* watch out for ref thru this */
      interp->files[i].fp = NULL;
    }
  }
  tempfree (x);
  x = gettemp ();
  x->setfval ((Awkfloat)stat);
  return x;
}


void flush_all (void)
{
  int i;

  for (i = 0; i < interp->nfiles; i++)
    if (interp->files[i].fp)
      fflush (interp->files[i].fp);
}

#ifdef _MSC_VER
/// Windows replacements for popen and pclose functions

FILE *popen (const char *s, const char *m)
{
  return _popen (s, m);
}

int pclose (FILE *f)
{
  return _pclose (f);
}
#endif