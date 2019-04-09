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
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "awk.h"
#include "ytab.h"
#include <awkerr.h>

#define  FULLTAB  2 /* rehash when table gets this x full */
#define  GROWTAB  4 /* grow table by this factor */

Array  *symtab;     /* main symbol table */

char  **FS;         /* initial field sep */
char  **RS;         /* initial record sep */
char  **OFS;        /* output field sep */
char  **ORS;        /* output record sep */
char  **OFMT;       /* output format for numbers */
char  **CONVFMT;    /* format for conversions in getsval */
Awkfloat *NF;       /* number of fields in current record */
Awkfloat *NR;       /* number of current record */
Awkfloat *FNR;      /* number of current record in current file */
const char  **FILENAME;   /* current filename argument */
Awkfloat *ARGC;     /* number of arguments from command line */
char  **SUBSEP;     /* subscript separator for a[i,j,k]; default \034 */
Awkfloat *RSTART;   /* start of re matched with ~; origin 1 (!) */
Awkfloat *RLENGTH;  /* length of same */

Cell  *fsloc;       /* FS */
Cell  *nrloc;       /* NR */
Cell  *nfloc;       /* NF */
Cell  *fnrloc;      /* FNR */
Array  *ARGVtab;    /* symbol table containing ARGV[...] */
Cell  *rstartloc;   /* RSTART */
Cell  *rlengthloc;  /* RLENGTH */

Node  *nullnode;    /* zero&null, converted into a node for comparisons */
Cell  *literal0;

extern Cell **fldtab;

static int  hash (const char *, int);
static void	rehash (Array *);

static void setfree(Cell *vp)
{
  if (&vp->sval == FS || &vp->sval == RS
   || &vp->sval == OFS || &vp->sval == ORS
   || &vp->sval == OFMT || &vp->sval == CONVFMT
   || &vp->sval == FILENAME || &vp->sval == SUBSEP)
    vp->tval |= DONTFREE;
  else
    vp->tval &= ~DONTFREE;
}

/// Initialize symbol table with builtin vars
void syminit (void)
{
  literal0 = setsymtab ("0", "0", 0.0, NUM | STR | DONTFREE, symtab);
  /* this is used for if(x)... tests: */
  nullnode = celltonode (setsymtab ("$zero&null", "", 0.0, NUM | STR | DONTFREE, symtab)
    , CCON);

  fsloc = setsymtab ("FS", " ", 0.0, STR | DONTFREE, symtab);
  FS = &fsloc->sval;
  RS = &setsymtab ("RS", "\n", 0.0, STR | DONTFREE, symtab)->sval;
  OFS = &setsymtab ("OFS", " ", 0.0, STR | DONTFREE, symtab)->sval;
  ORS = &setsymtab ("ORS", "\n", 0.0, STR | DONTFREE, symtab)->sval;
  OFMT = &setsymtab ("OFMT", "%.6g", 0.0, STR | DONTFREE, symtab)->sval;
  CONVFMT = &setsymtab ("CONVFMT", "%.6g", 0.0, STR | DONTFREE, symtab)->sval;
  FILENAME = const_cast <const char **>(&setsymtab ("FILENAME", "", 0.0, STR | DONTFREE, symtab)->sval);
  nfloc = setsymtab ("NF", "", 0.0, NUM, symtab);
  NF = &nfloc->fval;
  nrloc = setsymtab ("NR", "", 0.0, NUM, symtab);
  NR = &nrloc->fval;
  fnrloc = setsymtab ("FNR", "", 0.0, NUM, symtab);
  FNR = &fnrloc->fval;
  SUBSEP = &setsymtab ("SUBSEP", "\034", 0.0, STR | DONTFREE, symtab)->sval;
  rstartloc = setsymtab ("RSTART", "", 0.0, NUM, symtab);
  RSTART = &rstartloc->fval;
  rlengthloc = setsymtab ("RLENGTH", "", 0.0, NUM, symtab);
  RLENGTH = &rlengthloc->fval;
  
  setsymtab ("SYMTAB", "", 0.0, ARR, symtab)->sval = (char*)symtab;
}

/// Set up ARGV and ARGC
void arginit (int ac, char **av)
{
  Cell *cp;
  int i;
  char temp[50];

  ARGC = &setsymtab ("ARGC", "", (Awkfloat)ac, NUM, symtab)->fval;
  cp = setsymtab ("ARGV", "", 0.0, ARR, symtab);
  ARGVtab = makearray (NSYMTAB);  /* could be (int) ARGC as well */
  cp->sval = (char *)ARGVtab;
  for (i = 0; i < ac; i++)
  {
    sprintf (temp, "%d", i);
    if (is_number (*av))
      setsymtab (temp, *av, atof (*av), STR | NUM, ARGVtab);
    else
      setsymtab (temp, *av, 0.0, STR, ARGVtab);
    av++;
  }
}

/// Set up ENVIRON variable
void envinit ()
{
  Cell *cp;
  char *p;
  Array  *envtab;     /* symbol table containing ENVIRON[...] */
  char **envp;
  envp = environ;
  cp = setsymtab ("ENVIRON", "", 0.0, ARR, symtab);
  envtab = makearray (NSYMTAB);
  cp->sval = (char *)envtab;
  for (; *envp; envp++)
  {
    if ((p = strchr (*envp, '=')) == NULL)
      continue;
    if (p == *envp) /* no left hand side name in env string */
      continue;
    *p++ = 0;  /* split into two strings at = */
    if (is_number (p))
      setsymtab (*envp, p, atof (p), STR | NUM, envtab);
    else
      setsymtab (*envp, p, 0.0, STR, envtab);
    p[-1] = '=';  /* restore in case env is passed down to a shell */
  }
}

/// Make a new symbol table
Array *makearray (int n)
{
  Array *ap;
  Cell **tp;

  ap = (Array *)malloc (sizeof (Array));
  tp = (Cell **)calloc (n, sizeof (Cell *));
  if (ap == NULL || tp == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space in makearray");
  ap->nelem = 0;
  ap->size = n;
  ap->tab = tp;
  return ap;
}

void print_cell (Cell *c, int indent);

/// Free a symbol table entry
void freecell (Cell* cp)
{
#ifndef NDEBUG
  dprintf ("Deleting...");
  print_cell (cp, 1);
#endif
  if (isarr (cp))
  {
    dprintf ("Deleting array\n");
    freearray ((Array *)cp->sval);
    cp->sval = 0;
  }
  else if (freeable (cp))
  {
    dprintf ("freed %s\n", cp->sval);
    xfree (cp->sval);
  }
  xfree (cp->nval);
}

/// Free a hash table
void freearray (Array *ap)
{
  Cell *cp, *temp;
  int i;

  if (ap == NULL)
    return;
  for (i = 0; i < ap->size; i++)
  {
    for (cp = ap->tab[i]; cp != NULL; cp = temp)
    {
      dprintf ("freeing %s...", cp->nval);
      xfree (cp->nval);
      if (freeable (cp) || interp->status == AWKS_END) //at end we free everything
      {
        if (isarr (cp))
        {
          if ((Array*)cp->sval != ap) //prevent recursion (SYMTAB is a symbol in symtab)
          {
            dprintf ("...cell is an array\n");
            freearray ((Array*)cp->sval);
          }
          else
            dprintf (" self\n");
        }
        else if (cp->tval & (FCN|EXTFUN))
        {
          dprintf ("... skipping function\n");
        }
        else
        {
          dprintf (" (value=%s)\n", cp->sval);
          xfree (cp->sval);
        }
      }
      temp = cp->cnext;  /* avoids freeing then using */
      free (cp);
      ap->nelem--;
    }
    ap->tab[i] = 0;
  }
  if (ap->nelem != 0)
    FATAL (AWK_ERR_OTHER, "inconsistent element count freeing array");
  free (ap->tab);
  free (ap);
}

/// Free elem s from ap (i.e., ap["s"])
void freeelem (Cell *ap, const char *s)
{
  Array *tp;
  Cell *p, *prev = NULL;
  int h;

  tp = (Array *)ap->sval;
  h = hash (s, tp->size);
  for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
  {
    if (strcmp (s, p->nval) == 0)
    {
      if (prev == NULL)  /* 1st one */
        tp->tab[h] = p->cnext;
      else      /* middle somewhere */
        prev->cnext = p->cnext;
      if (freeable (p))
        xfree (p->sval);
      free (p->nval);
      free (p);
      tp->nelem--;
      return;
    }
  }
}

Cell *setsymtab (const char *n, const char *s, Awkfloat f, unsigned t, Array *tp)
{
  int h;
  Cell *p;

  if (n != NULL && (p = lookup (n, tp)) != NULL)
  {
    dprintf ("setsymtab found %p: n=%s s=\"%s\" f=%g t=%o\n",
      (void*)p, NN (p->nval), NN (p->sval), p->fval, p->tval);
    return p;
  }
  p = (Cell *)malloc (sizeof (Cell));
  if (p == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space for symbol table at %s", n);
  memset (p, 0, sizeof (Cell));
  p->nval = tostring (n);
  if (s)
    p->sval = strdup(s);
  p->fval = f;
  p->tval = t;
  p->ctype = OCELL;
  tp->nelem++;
  if (tp->nelem > FULLTAB * tp->size)
    rehash (tp);
  h = hash (n, tp->size);
  p->cnext = tp->tab[h];
  tp->tab[h] = p;
  dprintf ("setsymtab set %p: n=%s s=\"%s\" f=%g t=%o\n",
    (void*)p, p->nval, p->sval, p->fval, p->tval);
  return p;
}

/// Form hash value for string s
int hash (const char *s, int n)
{
  unsigned hashval;

  for (hashval = 0; *s != '\0'; s++)
    hashval = (*s + 31 * hashval);
  return hashval % n;
}

///  Rehash items in small table into big one
void rehash (Array *tp)
{
  int i, nh, nsz;
  Cell *cp, *op, **np;

  nsz = GROWTAB * tp->size;
  np = (Cell **)calloc (nsz, sizeof (Cell *));
  if (np == NULL)    /* can't do it, but can keep running. */
    return;    /* someone else will run out later. */
  for (i = 0; i < tp->size; i++)
  {
    for (cp = tp->tab[i]; cp; cp = op)
    {
      op = cp->cnext;
      nh = hash (cp->nval, nsz);
      cp->cnext = np[nh];
      np[nh] = cp;
    }
  }
  free (tp->tab);
  tp->tab = np;
  tp->size = nsz;
}

/// Look for s in tp
Cell *lookup (const char *s, Array *tp)
{
  Cell *p;
  int h;

  h = hash (s, tp->size);
  for (p = tp->tab[h]; p != NULL; p = p->cnext)
    if (strcmp (s, p->nval) == 0)
      return p;   /* found it */
  return NULL;    /* not found */
}

///  Set float val of a Cell
Awkfloat setfval (Cell *vp, Awkfloat f)
{
  int fldno;

  f += 0.0;    /* normalize negative zero to positive zero */
  if ((vp->tval & (NUM | STR)) == 0)
    funnyvar (vp, "assign to");
  if (isfld (vp))
  {
    donerec = 0;  /* mark $0 invalid */
    fldno = atoi (vp->nval);
    if (fldno > *NF)
      newfld (fldno);
    dprintf ("setting field %d to %g\n", fldno, f);
  }
  else if (&vp->fval == NF)
  {
    donerec = 0;  /* mark $0 invalid */
    setlastfld ((int)f);
    dprintf ("setting NF to %g\n", f);
  }
  else if (isrec (vp))
  {
    donefld = 0;  /* mark $1... invalid */
    donerec = 1;
  }
  if (freeable (vp))
    xfree (vp->sval); /* free any previous string */
  vp->tval &= ~(STR | CONVC); /* mark string invalid */
  vp->fmt = NULL;
  vp->tval |= NUM;  /* mark number ok */
  if (f == -0)  /* who would have thought this possible? */
    f = 0;
  dprintf ("setfval %p: %s = %g, t=%o\n", (void*)vp, NN (vp->nval), f, vp->tval);
  return vp->fval = f;
}

void funnyvar (Cell *vp, const char *rw)
{
  if (isarr (vp))
    FATAL (AWK_ERR_ARG, "can't %s %s; it's an array name.", rw, vp->nval);
  if (vp->tval & FCN)
    FATAL (AWK_ERR_ARG, "can't %s %s; it's a function.", rw, vp->nval);
  WARNING ("funny variable %p: n=%s s=\"%s\" f=%g t=%o",
    vp, vp->nval, vp->sval, vp->fval, vp->tval);
}

///  Set string val of a Cell
const char *setsval (Cell *vp, const char *s)
{
  char *t;
  int fldno;
  Awkfloat f;

  dprintf ("starting setsval %p: %s = \"%s\", t=%o, r,f=%d,%d\n",
    (void*)vp, NN (vp->nval), s, vp->tval, donerec, donefld);
  if ((vp->tval & (NUM | STR)) == 0)
    funnyvar (vp, "assign to");
  if (isfld (vp))
  {
    donerec = 0;  /* mark $0 invalid */
    fldno = atoi (vp->nval);
    if (fldno > *NF)
      newfld (fldno);
    dprintf ("setting field %d to %s (%p)\n", fldno, s, (void *)s);
  }
  else if (isrec (vp))
  {
    donefld = 0;  /* mark $1... invalid */
    donerec = 1;
  }
  else if (&vp->sval == OFS)
  {
    if (donerec == 0)
      recbld ();
  }
  t = s ? tostring (s) : tostring ("");  /* in case it's self-assign */
  if (freeable (vp))
    xfree (vp->sval);
  vp->tval &= ~(NUM | CONVC);
  vp->tval |= STR;
  vp->fmt = NULL;
  setfree (vp);
  dprintf ("setsval %p: %s = \"%s (%p) \", t=%o r,f=%d,%d\n",
    (void*)vp, NN (vp->nval), t, (void *)t, vp->tval, donerec, donefld);
  vp->sval = t;
  if (&vp->fval == NF)
  {
    donerec = 0;  /* mark $0 invalid */
    f = getfval (vp);
    setlastfld ((int)f);
    dprintf ("setting NF to %g\n", f);
  }

  return vp->sval;
}

///  Get float val of a Cell
Awkfloat getfval (Cell *vp)
{
  if ((vp->tval & (NUM | STR)) == 0)
    funnyvar (vp, "read value of");
  if (isfld (vp) && donefld == 0)
    fldbld ();
  else if (isrec (vp) && donerec == 0)
    recbld ();
  if (!isnum (vp))  /* not a number */
  {
    vp->fval = atof (vp->sval);  /* best guess */
    if (is_number (vp->sval) && (vp->csub != CCON))
      vp->tval |= NUM;  /* make NUM only sparingly */
  }
  dprintf ("getfval %p: %s = %g, t=%o\n",
    (void*)vp, NN (vp->nval), vp->fval, vp->tval);
  return vp->fval;
}

/// Get string val of a Cell
static const char *get_str_val (Cell *vp, char **fmt)
{
  char s[100];  /* BUG: unchecked */
  double dtemp;

  if ((vp->tval & (NUM | STR)) == 0)
    funnyvar (vp, "read value of");
  if (isfld (vp) && donefld == 0)
    fldbld ();
  else if (isrec (vp) && donerec == 0)
    recbld ();

  /*
   * ADR: This is complicated and more fragile than is desirable.
   * Retrieving a string value for a number associates the string
   * value with the scalar.  Previously, the string value was
   * sticky, meaning if converted via OFMT that became the value
   * (even though POSIX wants it to be via CONVFMT). Or if CONVFMT
   * changed after a string value was retrieved, the original value
   * was maintained and used.  Also not per POSIX.
   *
   * We work around this design by adding two additional flags,
   * CONVC and CONVO, indicating how the string value was
   * obtained (via CONVFMT or OFMT) and _also_ maintaining a copy
   * of the pointer to the xFMT format string used for the
   * conversion.  This pointer is only read, **never** dereferenced.
   * The next time we do a conversion, if it's coming from the same
   * xFMT as last time, and the pointer value is different, we
   * know that the xFMT format string changed, and we need to
   * redo the conversion. If it's the same, we don't have to.
   *
   * There are also several cases where we don't do a conversion,
   * such as for a field (see the checks below).
   */

   /* Don't duplicate the code for actually updating the value */
#define update_str_val(vp) \
  { \
    if (freeable(vp)) \
      xfree(vp->sval); \
    if (modf(vp->fval, &dtemp) == 0)  /* it's integral */ \
      sprintf(s, "%.30g", vp->fval); \
    else \
      sprintf(s, *fmt, vp->fval); \
    vp->sval = tostring(s); \
    vp->tval &= ~DONTFREE; \
    vp->tval |= STR; \
  }

  if (isstr (vp) == 0)
  {
    update_str_val (vp);
    if (fmt == OFMT)
      vp->tval &= ~CONVC;
    else
      vp->tval |= CONVC;
    vp->fmt = *fmt;
  }
  else
  {
    if ((vp->tval & DONTFREE) != 0 || !isnum (vp) || isfld (vp))
      goto done;

    if (fmt == OFMT && ((vp->tval & CONVC)
       || ( vp->fmt != *fmt)))
    {
      update_str_val (vp);
      vp->tval &= ~CONVC;
      vp->fmt = *fmt;
    }
    else if (fmt == OFMT && ((vp->tval & CONVC) == 0 || vp->fmt != *fmt))
    {
      update_str_val (vp);
      vp->tval |= CONVC;
      vp->fmt = *fmt;
    }
  }
done:
  dprintf ("getsval %p: %s = \"%s (%p)\", t=%o\n",
    (void*)vp, NN (vp->nval), vp->sval, (void *)vp->sval, vp->tval);
  if (!vp->sval)
  {
    dprintf ("NULL sval for %s\n", vp->nval);
    return "";
  }
  return vp->sval;
}

/// Get string val of a Cell
const char *getsval (Cell *vp)
{
  return get_str_val (vp, CONVFMT);
}

/// Get string val of a Cell for print */
const char *getpssval (Cell *vp)
{
  return get_str_val (vp, OFMT);
}

/// Make a copy of string s
char *tostring (const char *s)
{
  char *p;

  p = (char *)malloc (strlen (s) + 1);
  if (p == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space in tostring on %s", s);
  strcpy (p, s);
  return p;
}

/// Collect string up to next delim
char *qstring (const char *is, int delim)
{
  const char *os = is;
  const char *pd;
  int c, n;

  uschar *s = (uschar *)is;
  uschar *buf, *bp;

  if (!(pd = strchr (is, delim)))
    FATAL (AWK_ERR_ARG, "invalid delimiter for qstring");

  n = pd - is;
  if ((buf = (uschar *)malloc (n + 3)) == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space in qstring(%s)", s);
  for (bp = buf; (c = *s) != delim; s++)
  {
    if (c == '\n')
      SYNTAX ("newline in string %.20s...", os);
    else if (c != '\\')
      *bp++ = c;
    else   /* \something */
    {
      c = *++s;
      if (c == 0) /* \ at end */
      { 
        *bp++ = '\\';
        break;  /* for loop */
      }
      switch (c) 
      {
      case '\\':  *bp++ = '\\'; break;
      case 'n':  *bp++ = '\n'; break;
      case 't':  *bp++ = '\t'; break;
      case 'b':  *bp++ = '\b'; break;
      case 'f':  *bp++ = '\f'; break;
      case 'r':  *bp++ = '\r'; break;
      default:
        if (!isdigit (c))
        {
          *bp++ = c;
          break;
        }
        n = c - '0';
        if (isdigit (s[1]))
        {
          n = 8 * n + *++s - '0';
          if (isdigit (s[1]))
            n = 8 * n + *++s - '0';
        }
        *bp++ = n;
        break;
      }
    }
  }
  *bp++ = 0;
  return (char *)buf;
}

const char *flags2str (int flags)
{
  static const struct ftab {
    const char *name;
    int value;
  } flagtab[] = {
    { "NUM", NUM },
    { "STR", STR },
    { "DONTFREE", DONTFREE },
    { "ARR", ARR },
    { "FCN", FCN },
    { "FLD", FLD },
    { "REC", REC },
    { "CONVC", CONVC },
    { NULL, 0 }
  };
  static char buf[100];
  int i;
  char *cp = buf;

  for (i = 0; flagtab[i].name != NULL; i++) {
    if ((flags & flagtab[i].value) != 0) {
      if (cp > buf)
        *cp++ = '|';
      strcpy (cp, flagtab[i].name);
      cp += strlen (cp);
}
  }

  return buf;
}
