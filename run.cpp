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
#include <awklib/err.h>
#include <awklib/awk.h>

extern  int  pairstack[];
extern  Awkfloat  srand_seed;

static Cell  truecell (Cell::type::OBOOL, Cell::subtype::BTRUE, 0, 0, 1.0, NUM);
Cell  *True  = &truecell;
static Cell  falsecell (Cell::type::OBOOL, Cell::subtype::BFALSE, 0, 0, 0.0, NUM);
Cell  *False  = &falsecell;
static Cell  breakcell  (Cell::type::OJUMP, Cell::subtype::JBREAK, 0, 0, 0.0, NUM);
Cell  *jbreak  = &breakcell;
static Cell  contcell (Cell::type::OJUMP, Cell::subtype::JCONT, 0, 0, 0.0, NUM);
Cell  *jcont  = &contcell;
static Cell  nextcell (Cell::type::OJUMP, Cell::subtype::JNEXT, 0, 0, 0.0, NUM);
Cell  *jnext  = &nextcell;
static Cell  nextfilecell (Cell::type::OJUMP, Cell::subtype::JNEXTFILE, 0, 0, 0.0, NUM);
Cell  *jnextfile  = &nextfilecell;
static Cell  exitcell (Cell::type::OJUMP, Cell::subtype::JEXIT, 0, 0, 0.0, NUM);
Cell  *jexit  = &exitcell;
static Cell  retcell (Cell::type::OJUMP, Cell::subtype::JRET, 0, 0, 0.0, NUM);
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


inline int notlegal (Node* n)
{
  return n->nobj <= FIRSTTOKEN
      || n->nobj >= LASTTOKEN
      || n->proc == nullproc;
}

/// Execute a node of the parse tree
Cell *execute (Node *u)
{
  Cell *x;
  Node *a;

  if (u == NULL)
    return True;

  for (a = u; ; a = a->nnext)
  {
    if (isvalue (a))
    {
      x = (Cell *)(a->narg[0]);
      if (x->isfld () && !interp->donefld)
        interp->fldbld ();
      else if (x->isrec () && !interp->donerec)
        interp->recbld ();

      return x;
    }
    if (notlegal (a))  /* probably a Cell* but too risky to print */
      FATAL (AWK_ERR_SYNTAX, "illegal statement");
   
    x = (*a->proc)(a->narg, a->nobj);
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
Cell *program (Node **a, int n)
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
    while (interp->getrec (&interp->fldtab[0]) > 0)
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
Cell *call (Node **a, int n)
{
  int i, ncall, ndef;
  Node *x;
  Cell *y;
  Frame frm;
  Cell **callpar = NULL;

  memset (&frm, 0, sizeof (frm));
  frm.fcn = execute (a[0]);  /* the function itself */
  if (!frm.fcn->isfcn ())
    FATAL (AWK_ERR_RUNTIME, "calling undefined function %s", frm.fcn->nval);
  for (ncall = 0, x = a[1]; x != NULL; x = x->nnext)  /* args in call */
    ncall++;
  ndef = (int)frm.fcn->fval;      /* args in defn */
  dprintf ("calling %s, %d args (%d in defn)\n", frm.fcn->nval, ncall, ndef);
  if (ncall > ndef)
    WARNING ("function %s called with %d args, uses only %d",
      frm.fcn->nval, ncall, ndef);

  frm.args = 0;
  if (ndef)
  {
    frm.args = (Cell**)calloc (ndef, sizeof (Cell*));
    callpar = (Cell**)calloc (ndef, sizeof(Cell*));
  }

  for (i = 0, x = a[1]; x != NULL; i++, x = x->nnext)
  {  /* get call args */
    y = execute (x);
    dprintf ("args[%d]: %s %f <%s>, t=%s\n",
      i, NN (y->nval), y->fval, y->isarr () ? "(array)" : NN (y->sval), flags2str (y->flags));
    if (y->isfcn ())
      FATAL (AWK_ERR_RUNTIME, "can't use function %s as argument in %s", y->nval, frm.fcn->nval);
    if (i < ndef)  // used arguments are stored in args array the rest are just
    {              // evaluated (for eventual side effects)
      callpar[i] = y;
      if (y->isarr ())
        frm.args[i] = y;  // arrays by ref
      else
      {
        frm.args[i] = gettemp ();
        frm.args[i]->csub = Cell::subtype::CARG;
        frm.args[i]->nval = y->nval?strdup (y->nval) : 0;
        switch (y->flags & (NUM | STR))
        {
        case STR:
          dprintf ("STR arg\n");
          frm.args[i]->sval = tostring (y->sval);
          frm.args[i]->flags = STR;
          break;
        case NUM:
          frm.args[i]->fval = y->fval;
          frm.args[i]->flags = NUM;
          break;
        case (NUM|STR):
          frm.args[i]->sval = tostring (y->sval);
          frm.args[i]->fval = y->fval;
          frm.args[i]->flags = NUM | STR;
          break;
        case 0:
          frm.args[i]->sval = tostring ("");
          frm.args[i]->fval = 0.;
          break;
        }
      }
    }
    tempfree (y);
  }

  for (; i < ndef; i++)
  {  /* add null args for ones not provided */
    frm.args[i] = gettemp ();
    frm.args[i]->csub = Cell::subtype::CARG;
  }

  frm.nargs = ndef;
  frm.ret = gettemp ();

  //save previous function call frame
  Frame prev = interp->fn;

  //and replace it with current frame
  interp->fn = frm;

  dprintf ("start exec of %s\n", frm.fcn->nval);
  if (frm.fcn->flags & EXTFUNC)
  {
    //set args for external function
    awksymb *extargs = (awksymb *)calloc (ndef, sizeof (awksymb));
    for (i = 0; i < ndef; i++)
    {
      if (frm.args[i]->isarr())
      {
        extargs[i].flags = AWKSYMB_ARR;
        extargs[i].name = frm.args[i]->nval;
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
    ((awkfunc)frm.fcn->sval) ((AWKINTERP*)interp, &extret, ndef, extargs);
    y = gettemp ();
    free (extargs);
    if (extret.flags & AWKSYMB_STR)
      frm.ret->setsval (extret.sval);
    if (extret.flags & AWKSYMB_NUM)
      frm.ret->setfval (extret.fval);
  }
  else
    y = execute ((Node*)(frm.fcn->sval));  /* execute body */
  dprintf ("finished exec of %s\n", frm.fcn->nval);
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
        delete callpar[i]->sval;
        callpar[i]->csub = Cell::subtype::CARR;
        callpar[i]->arrval = t->arrval;
        continue;
      }
    }
    else
    {
      t->csub = Cell::subtype::CTEMP;
      tempfree (t);
    }
  }
  xfree (frm.args);
  xfree (callpar);
  tempfree (frm.fcn);
  if (y->isexit () || y->isnext ())
    return y;

  if ((frm.ret->flags & (NUM | STR)) == 0)
  {
    //no return statement
    frm.ret->flags = NUM | STR;
  }
  dprintf ("%s returns %g |%s| %s\n", frm.fcn->nval, frm.ret->getfval (),
    frm.ret->getsval (), flags2str (frm.ret->flags));
  return frm.ret;
}

/// nth argument of a function
Cell *arg (Node **a, int n)
{
  n = ptoi (a[0]);  /* argument number, counting from 0 */
  dprintf ("arg(%d), nargs=%d\n", n, interp->fn.nargs);
  if (n + 1 > interp->fn.nargs)
    FATAL (AWK_ERR_RUNTIME, "argument #%d of function %s was not supplied",
      n + 1, interp->fn.fcn->nval);
  return interp->fn.args[n];
}

/// break, continue, next, nextfile, return
Cell *jump (Node **a, int n)
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
Cell *awkgetline (Node **a, int)
{
  /* a[0] is variable, a[1] is operator, a[2] is filename */
  Cell *r, *x;
  FILE *fp;
  int mode, c;

  r = a[0] ? execute (a[0]) : &interp->fldtab[0];

  fflush (interp->files[1].fp);  /* in case someone is waiting for a prompt */
  if (a[2] != NULL)
  {    /* getline < file */
    x = execute (a[2]);    /* filename */
    mode = ptoi (a[1]);
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
    r->fval = atof (r->sval);
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
Cell *getnf (Node **a, int n)
{
  if (!interp->donefld)
    interp->fldbld ();
  Cell *pnf = execute (a[0]);
  return pnf;
}

Cell *array (Node **a, int n)
{
  /* a[0] is symtab, a[1] is list of subscripts */
  Cell *x, *y, *z;
  const char *s;
  Node *np;
  char *buf;
  size_t bufsz = RECSIZE;
  size_t nsub = strlen (SUBSEP);

  buf = new char[bufsz];

  x = execute (a[0]);  /* Cell* for symbol table */
  buf[0] = 0;
  for (np = a[1]; np; np = np->nnext)
  {
    y = execute (np);  /* subscript */
    s = y->getsval ();
    adjbuf (&buf, &bufsz, strlen (buf) + strlen (s) + nsub + 1, RECSIZE, 0);
    strcat (buf, s);
    if (np->nnext)
      strcat (buf, SUBSEP);
    tempfree (y);
  }
  if (!x->isarr ())
  {
    dprintf ("making %s into an array\n", NN (x->nval));
    delete x->sval;
    x->flags &= ~(STR | NUM);
    x->csub = Cell::subtype::CARR;
    x->arrval = new Array (NSYMTAB);
  }
  z = x->arrval->setsym (buf, "", 0.0, STR | NUM);
  z->csub = Cell::subtype::CVAR;
  tempfree (x);
  delete buf;

  return z;
}

Cell *awkdelete (Node **a, int n)
{
  /* a[0] is array, a[1] is list of subscripts */
  Cell *x, *y;
  Node *np;
  const char *s;
  size_t nsub = strlen (SUBSEP);

  x = execute (a[0]);  /* Cell* for array */
  if (!x->isarr ())
    return True;
  if (a[1] == 0)
  {
    //replace with a clean array
    delete x->arrval;
    x->arrval = new Array (NSYMTAB);
  }
  else
  {
    size_t bufsz = RECSIZE;
    char* buf = new char[bufsz];
    buf[0] = 0;
    for (np = a[1]; np; np = np->nnext)
    {
      y = execute (np);  /* subscript */
      s = y->getsval ();
      adjbuf (&buf, &bufsz, strlen (buf) + strlen (s) + nsub + 1, RECSIZE, 0);
      strcat (buf, s);
      if (np->nnext)
        strcat (buf, SUBSEP);
      tempfree (y);
    }
    freeelem (x, buf);
    delete buf;
  }
  tempfree (x);

  return True;
}

Cell *intest (Node **a, int n)
{
  /* a[0] is index (list), a[1] is array */
  Cell *x, *ap, *k;
  Node *p;
  char *buf;
  const char *s;
  size_t bufsz = RECSIZE;
  size_t nsub = strlen (SUBSEP);

  ap = execute (a[1]);  /* array name */
  if (!ap->isarr ())
  {
    dprintf ("making %s into an array\n", ap->nval);
    delete ap->sval;
    ap->flags &= ~(STR | NUM);
    ap->csub = Cell::subtype::CARR;
    ap->arrval = new Array (NSYMTAB);
  }
  buf = new char[bufsz];
  buf[0] = 0;
  for (p = a[0]; p; p = p->nnext)
  {
    x = execute (p);  /* expr */
    s = x->getsval ();
    adjbuf (&buf, &bufsz, strlen (buf) + strlen (s) + nsub + 1, RECSIZE, 0);
    strcat (buf, s);
    tempfree (x);
    if (p->nnext)
      strcat (buf, SUBSEP);
  }
  k = ap->arrval->lookup (buf);
  tempfree (ap);
  delete buf;
  if (k == NULL)
    return False;
  else
    return True;
}

/// ~ and match()
Cell *matchop (Node **a, int n)
{
  Cell *x, *y;
  const char *s, *t;
  int i;
  fa *pfa;
  int (*mf)(fa *, const char *) = match, mode = 0;

  if (n == MATCHFCN)
  {
    mf = pmatch;
    mode = 1;
  }
  x = execute (a[1]);  /* a[1] = target text */
  s = x->getsval ();
  if (a[0] == 0)    /* a[1] == 0: already-compiled reg expr */
    i = (*mf)((fa *)a[2], s);
  else
  {
    y = execute (a[2]);  /* a[2] = regular expr */
    t = y->getsval ();
    pfa = makedfa (t, mode);
    i = (*mf)(pfa, s);
    tempfree (y);
  }
  tempfree (x);
  if (n == MATCHFCN)
  {
    size_t start = patbeg - s + 1;
    if (patlen == (size_t)(-1))
      start = 0;
    RSTART = (Awkfloat)start;
    RLENGTH = (Awkfloat)patlen;
    x = gettemp ();
    x->flags = NUM;
    x->fval = (Awkfloat)start;
    return x;
  }
  else if ((n == MATCH && i == 1) || (n == NOTMATCH && i == 0))
    return True;
  else
    return False;
}

///  Boolean operations a[0] || a[1], a[0] && a[1], !a[0]
Cell *boolop (Node **a, int n)
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
Cell *relop (Node **a, int n)
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
  if (a->csub != Cell::subtype::CTEMP)
    return;
  dprintf ("freeing %s %s %s\n", NN (a->nval), quote (a->sval), flags2str(a->flags));
  delete a;
}

#define TEMPSCHUNK  100

///  Get a tempcell
Cell *gettemp ()
{
  return  new Cell (Cell::type::OCELL, Cell::subtype::CTEMP, 0, 0, 0., 0);
}

/// $( a[0] )
Cell *indirect (Node **a, int n)
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
Cell *substr (Node **a, int nnn)
{
  const char *s;
  char *ss;
  size_t n, m, k;
  Cell *x, *y, *z = 0;

  x = execute (a[0]);
  y = execute (a[1]);
  if (a[2] != 0)
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
Cell *sindex (Node **a, int nnn)
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
Cell *awksprintf (Node **a, int n)
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
Cell *awkprintf (Node **a, int n)
{
  /* a[0] is list of args, starting with format string */
  /* a[1] is redirection operator, a[2] is redirection file */
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
  if (a[1] == NULL)
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
    fp = redirect (ptoi (a[1]), a[2]);
    fwrite (buf, len, 1, fp);
    fflush (fp);
    if (ferror (fp))
      FATAL (AWK_ERR_OUTFILE, "write error on %s", filename (fp));
  }
  delete buf;
  return True;
}

/// Arithmetic operations   a[0] + a[1], etc.  also -a[0]
Cell *arith (Node **a, int n)
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
Cell *incrdecr (Node **a, int n)
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
Cell *assign (Node **a, int n)
{
  /* this is subtle; don't muck with it. */
  Cell *x, *y;
  Awkfloat xf, yf;
  double v;

  y = execute (a[1]);
  x = execute (a[0]);
  if (n == ASSIGN)
  {
    /* ordinary assignment */
    if (x->isfld())
    {
      int n = atoi (x->nval);
      if (n > NF)
        interp->newfld (n);
    }
    if (x != y)  // leave alone self-assignment */
    {
      x->flags &= ~(NUM | STR);
      switch (y->flags & (STR|NUM))
      {
      case (NUM|STR):
        x->setsval (y->getsval ());
        x->fval = y->getfval ();
        x->flags |= NUM | STR;
        break;
      case STR:
        x->setsval (y->getsval ());
        x->flags |= STR;
        break;
      case NUM:
        x->setfval (y->getfval ());
        x->flags |= NUM;
        break;
      case 0:
        delete x->sval;
        x->sval = 0;
        x->fval = 0.;
        break;
      }
    }
    tempfree (y);
    return x;
  }
  xf = x->getfval ();
  yf = y->getfval ();
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
  tempfree (y);
  x->setfval (xf);
  return x;
}

/// Concatenation a[0] cat a[1]
Cell *cat (Node **a, int q)
{
  Cell *x, *y, *z;
  size_t n1, n2;
  char *s;

  x = execute (a[0]);
  y = execute (a[1]);

  //get private copies of svals (in case x == y and getting one sval
  //invalidates the other)
  char *sx = tostring(x->getsval ());
  char *sy = tostring(y->getsval ());
  n1 = strlen (sx);
  n2 = strlen (sy);
  s = new char[n1 + n2 + 1];
  strcpy (s, sx);
  strcpy (s + n1, sy);
  tempfree (x);
  tempfree (y);
  z = gettemp ();
  z->sval = s;
  z->flags = STR;
  delete sx;
  delete sy;
  return z;
}

/// Pattern-action statement a[0] { a[1] }
Cell *pastat (Node **a, int n)
{
  Cell *x;

  if (a[0] == 0)
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
Cell *dopa2 (Node **a, int n)
{
  Cell *x;
  int pair;

  pair = ptoi (a[3]);
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

/// split(a[0], a[1], a[2]); a[3] is type
Cell *split (Node **a, int nnn)
{
  Cell *x = 0, *y, *ap;
  char *s, *origs;
  int sep;
  char *t, temp, num[50];
  const char *fs = 0;
  int n, tempstat, arg3type;

  y = execute (a[0]);  /* source string */
  origs = s = strdup (y->getsval ());
  arg3type = ptoi (a[3]);
  if (a[2] == 0)    /* fs string */
    fs = FS;
  else if (arg3type == STRING)
  {  /* split(str,arr,"string") */
    x = execute (a[2]);
    fs = x->getsval ();
  }
  else if (arg3type == REGEXPR)
    fs = "(regexpr)";  /* split(str,arr,/regexpr/) */
  else
    FATAL (AWK_ERR_ARG, "illegal type of split");
  sep = *fs;
  ap = execute (a[1]);  /* array name */
  dprintf ("split: s=|%s|, a=%s, sep=|%s|\n", s, NN (ap->nval), fs);
  if (ap->isarr ())
    delete ap->arrval;
  else
    delete ap->sval;
  ap->flags &= ~(STR | NUM);
  ap->csub = Cell::subtype::CARR;
  ap->arrval = new Array (NSYMTAB);

  n = 0;
  if (arg3type == REGEXPR && strlen ((char*)((fa*)a[2])->restr) == 0)
  {
    /* split(s, a, //); have to arrange that it looks like empty sep */
    arg3type = 0;
    fs = "";
    sep = 0;
  }
  if (*s != '\0' && (strlen (fs) > 1 || arg3type == REGEXPR))
  {  /* reg expr */
    fa *pfa;
    if (arg3type == REGEXPR)
    {  /* it's ready already */
      pfa = (fa *)a[2];
    }
    else
    {
      pfa = makedfa (fs, 1);
    }
    if (nematch (pfa, s))
    {
      tempstat = pfa->initstat;
      pfa->initstat = 2;
      do
      {
        n++;
        sprintf (num, "%d", n);
        temp = *patbeg;
        *patbeg = '\0';
        if (is_number (s))
          ap->arrval->setsym (num, s, atof (s), STR | NUM);
        else
          ap->arrval->setsym (num, s, 0.0, STR);
        *patbeg = temp;
        s = patbeg + patlen;
        if (*(patbeg + patlen - 1) == 0 || *s == 0)
        {
          n++;
          sprintf (num, "%d", n);
          ap->arrval->setsym (num, "", 0.0, STR);
          pfa->initstat = tempstat;
          goto spdone;
        }
      } while (nematch (pfa, s));
      pfa->initstat = tempstat;   /* bwk: has to be here to reset */
              /* cf gsub and refldbld */
    }
    n++;
    sprintf (num, "%d", n);
    if (is_number (s))
      ap->arrval->setsym (num, s, atof (s), STR | NUM);
    else
      ap->arrval->setsym (num, s, 0.0, STR);
  spdone:
    pfa = NULL;
  }
  else if (sep == ' ')
  {
    for (n = 0; ; )
    {
      while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
      if (*s == 0)
        break;
      n++;
      t = s;
      do
        s++;
      while (*s != ' ' && *s != '\t' && *s != '\n' && *s != '\0');
      temp = *s;
      *s = '\0';
      sprintf (num, "%d", n);
      if (is_number (t))
        ap->arrval->setsym (num, t, atof (t), STR | NUM);
      else
        ap->arrval->setsym (num, t, 0.0, STR);
      *s = temp;
      if (*s != 0)
        s++;
    }
  }
  else if (sep == 0)
  {  /* new: split(s, a, "") => 1 char/elem */
    for (n = 0; *s != 0; s++)
    {
      char buf[2];
      n++;
      sprintf (num, "%d", n);
      buf[0] = *s;
      buf[1] = 0;
      if (isdigit (buf[0]))
        ap->arrval->setsym (num, buf, atof (buf), STR | NUM);
      else
        ap->arrval->setsym (num, buf, 0.0, STR);
    }
  }
  else if (*s != 0)
  {
    for (;;)
    {
      n++;
      t = s;
      while (*s != sep && *s != '\n' && *s != '\0')
        s++;
      temp = *s;
      *s = '\0';
      sprintf (num, "%d", n);
      if (is_number (t))
        ap->arrval->setsym (num, t, atof (t), STR | NUM);
      else
        ap->arrval->setsym (num, t, 0.0, STR);
      *s = temp;
      if (*s++ == 0)
        break;
    }
  }
  tempfree (ap);
  tempfree (y);
  free (origs);
  if (a[2] != 0 && arg3type == STRING)
  {
    tempfree (x);
  }
  x = gettemp ();
  x->flags = NUM;
  x->fval = n;
  return x;
}

/// Ternaty operator ?:   a[0] ? a[1] : a[2]
Cell *condexpr (Node **a, int n)
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
Cell *ifstat (Node **a, int n)
{
  Cell *x;

  x = execute (a[0]);
  if (x->istrue ())
  {
    tempfree (x);
    x = execute (a[1]);
  }
  else if (a[2] != 0)
  {
    tempfree (x);
    x = execute (a[2]);
  }
  return(x);
}

/// While statement  while (a[0]) a[1]
Cell *whilestat (Node **a, int n)
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
Cell *dostat(Node **a, int n)
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
Cell *forstat (Node **a, int n)
{
  Cell *x;

  x = execute (a[0]);
  tempfree (x);
  for (;;)
  {
    if (a[1] != 0)
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
Cell *instat (Node **a, int n)
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
    vp->setsval (cp->nval);
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

/// Builtin function (sin, cos, etc.)  a[0] is type, a[1] is arg list
Cell *bltin (Node **a, int n)
{
  Cell *x, *y;
  Awkfloat u;
  int t;
  Awkfloat tmp;
  char *p, *buf;
  Node *nextarg;
  FILE *fp;
  void flush_all (void);
  int status = 0;

  t = ptoi (a[0]);
  x = execute (a[1]);
  nextarg = a[1]->nnext;
  switch (t)
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
    /* in principle, rand() returns something in 0..RAND_MAX */
    u = (Awkfloat)(rand () % RAND_MAX) / RAND_MAX;
    break;
  case FSRAND:
    if (x->isrec ())  /* no argument provided */
      u = (Awkfloat)time ((time_t *)0);
    else
      u = x->getfval ();
    tmp = u;
    srand ((unsigned int)u);
    u = interp->srand_seed;
    interp->srand_seed = tmp;
    break;
  case FTOUPPER:
  case FTOLOWER:
    buf = tostring (x->getsval ());
    if (t == FTOUPPER)
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
    FATAL (AWK_ERR_OTHER, "illegal function type %d", t);
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
    a[1] - redirection operator (FFLUSH, GT, APPEND, PIPE) or NULL
    a[2] - file or NULL
*/
Cell *printstat (Node **a, int n)
{
  Node *x;
  Cell *y;
  FILE *fp;

  if (a[1] == 0)  /* a[1] is redirection operator, a[2] is file */
    fp = interp->files[1].fp;
  else
    fp = redirect (ptoi (a[1]), a[2]);
  for (x = a[0]; x != NULL; x = x->nnext)
  {
    y = execute (x);
    interp->putstr (y->getpssval (), fp);
    tempfree (y);
    if (x->nnext == NULL)
      interp->putstr (ORS, fp);
    else
      interp->putstr (OFS, fp);
  }
  if (a[1] != 0)
    fflush (fp);
  if (ferror (fp))
    FATAL (AWK_ERR_OUTFILE, "write error on %s", filename (fp));

  return True;
}

Cell *nullproc (Node **a, int n)
{
  return 0;
}

/// Set up all i/o redirections
FILE *redirect (int a, Node *b)
{
  FILE *fp;
  Cell *x;
  const char *fname;

  x = execute (b);
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

Cell *closefile (Node **a, int n)
{
  Cell *x;
  int i, stat;

  x = execute (a[0]);
  x->getsval ();
  stat = -1;
  for (i = 0; i < interp->nfiles; i++)
  {
    if (interp->files[i].fname && strcmp (x->sval, interp->files[i].fname) == 0)
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

void backsub(char **pb_ptr, const char **sptr_ptr);

/// substitute command
Cell *sub (Node **a, int nnn)
{
  char *pb, *q;
  Cell *x, *y, *result;
  const char *t, *sptr;
  fa *pfa;
  size_t bufsz = RECSIZE;
  char* buf = new char[bufsz];

  x = execute (a[3]);  /* target string */
  t = x->getsval ();
  if (a[0] == 0)    /* 0 => a[1] is already-compiled regexpr */
    pfa = (fa *)a[1];  /* regular expression */
  else
  {
    y = execute (a[1]);
    pfa = makedfa (y->getsval (), 1);
    tempfree (y);
  }
  y = execute (a[2]);  /* replacement string */
  result = False;
  if (pmatch (pfa, t))
  {
    sptr = t;
    adjbuf (&buf, &bufsz, 1 + patbeg - sptr, RECSIZE, 0);
    pb = buf;
    while (sptr < patbeg)
      *pb++ = *sptr++;
    sptr = y->getsval ();
    while (*sptr != 0)
    {
      adjbuf (&buf, &bufsz, 5 + pb - buf, RECSIZE, &pb);
      if (*sptr == '\\')
      {
        backsub (&pb, &sptr);
      }
      else if (*sptr == '&')
      {
        sptr++;
        adjbuf (&buf, &bufsz, 1 + patlen + pb - buf, RECSIZE, &pb);
        for (q = patbeg; q < patbeg + patlen; )
          *pb++ = *q++;
      }
      else
        *pb++ = *sptr++;
    }
    *pb = '\0';
    if (pb > buf + bufsz)
      FATAL (AWK_ERR_OTHER, "sub result1 %.30s too big; can't happen", buf);
    sptr = patbeg + patlen;
    if ((patlen == 0 && *patbeg) || (patlen && *(sptr - 1)))
    {
      adjbuf (&buf, &bufsz, 1 + strlen (sptr) + pb - buf, 0, &pb);
      while ((*pb++ = *sptr++) != 0)
        ;
    }
    if (pb > buf + bufsz)
      FATAL (AWK_ERR_OTHER, "sub result2 %.30s too big; can't happen", buf);
    x->setsval (buf);  /* BUG: should be able to avoid copy */
    result = True;;
  }
  tempfree (x);
  tempfree (y);
  delete buf;
  return result;
}

/// global substitute
Cell *gsub (Node **a, int nnn)
{
  Cell *x, *y;
  char *pb, *q;
  const char *t, *rptr, *sptr;
  fa *pfa;
  int mflag, tempstat, num;
  size_t bufsz = RECSIZE;
  char* buf = new char[bufsz];

  mflag = 0;  /* if mflag == 0, can replace empty string */
  num = 0;
  x = execute (a[3]);  /* target string */
  t = x->getsval ();
  if (a[0] == 0)    /* 0 => a[1] is already-compiled regexpr */
    pfa = (fa *)a[1];  /* regular expression */
  else
  {
    y = execute (a[1]);
    pfa = makedfa (y->getsval (), 1);
    tempfree (y);
  }
  y = execute (a[2]);  /* replacement string */
  if (pmatch (pfa, t))
  {
    tempstat = pfa->initstat;
    pfa->initstat = 2;
    pb = buf;
    rptr = y->getsval ();
    do
    {
      if (patlen == 0 && *patbeg != 0)
      {  /* matched empty string */
        if (mflag == 0)
        {  /* can replace empty */
          num++;
          sptr = rptr;
          while (*sptr != 0)
          {
            adjbuf (&buf, &bufsz, 5 + pb - buf, RECSIZE, &pb);
            if (*sptr == '\\')
            {
              backsub (&pb, &sptr);
            }
            else if (*sptr == '&')
            {
              sptr++;
              adjbuf (&buf, &bufsz, 1 + patlen + pb - buf, RECSIZE, &pb);
              for (q = patbeg; q < patbeg + patlen; )
                *pb++ = *q++;
            }
            else
              *pb++ = *sptr++;
          }
        }
        if (*t == 0)  /* at end */
          goto done;
        adjbuf (&buf, &bufsz, 2 + pb - buf, RECSIZE, &pb);
        *pb++ = *t++;
        if (pb > buf + bufsz)  /* BUG: not sure of this test */
          FATAL (AWK_ERR_OTHER, "gsub result0 %.30s too big; can't happen", buf);
        mflag = 0;
      }
      else
      {  /* matched nonempty string */
        num++;
        sptr = t;
        adjbuf (&buf, &bufsz, 1 + (patbeg - sptr) + pb - buf, RECSIZE, &pb);
        while (sptr < patbeg)
          *pb++ = *sptr++;
        sptr = rptr;
        while (*sptr != 0)
        {
          adjbuf (&buf, &bufsz, 5 + pb - buf, RECSIZE, &pb);
          if (*sptr == '\\')
          {
            backsub (&pb, &sptr);
          }
          else if (*sptr == '&')
          {
            sptr++;
            adjbuf (&buf, &bufsz, 1 + patlen + pb - buf, RECSIZE, &pb);
            for (q = patbeg; q < patbeg + patlen; )
              *pb++ = *q++;
          }
          else
            *pb++ = *sptr++;
        }
        t = patbeg + patlen;
        if (patlen == 0 || *t == 0 || *(t - 1) == 0)
          goto done;
        if (pb > buf + bufsz)
          FATAL (AWK_ERR_OTHER, "gsub result1 %.30s too big; can't happen", buf);
        mflag = 1;
      }
    } while (pmatch (pfa, t));
    sptr = t;
    adjbuf (&buf, &bufsz, 1 + strlen (sptr) + pb - buf, 0, &pb);
    while ((*pb++ = *sptr++) != 0)
      ;
  done:  if (pb < buf + bufsz)
    *pb = '\0';
         else if (*(pb - 1) != '\0')
    FATAL (AWK_ERR_OTHER, "gsub result2 %.30s truncated; can't happen", buf);
         x->setsval (buf);  /* BUG: should be able to avoid copy + free */
         pfa->initstat = tempstat;
  }
  tempfree (x);
  tempfree (y);
  x = gettemp ();
  x->flags = NUM;
  x->fval = num;
  delete buf;
  return x;
}

/*!  handle \\& variations */
void backsub (char **pb_ptr, const char **sptr_ptr)
{
  /* sptr[0] == '\\' */
  char *pb = *pb_ptr;
  const char* sptr = *sptr_ptr;

  if (sptr[1] == '\\')
  {
    if (sptr[2] == '\\' && sptr[3] == '&')
    { /* \\\& -> \& */
      *pb++ = '\\';
      *pb++ = '&';
      sptr += 4;
    }
    else if (sptr[2] == '&')
    {  /* \\& -> \ + matched */
      *pb++ = '\\';
      sptr += 2;
    }
    else
    {      /* \\x -> \\x */
      *pb++ = *sptr++;
      *pb++ = *sptr++;
    }
  }
  else if (sptr[1] == '&')
  {  /* literal & */
    sptr++;
    *pb++ = *sptr++;
  }
  else        /* literal \ */
    *pb++ = *sptr++;

  *pb_ptr = pb;
  *sptr_ptr = sptr;
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