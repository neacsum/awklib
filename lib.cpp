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
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include "awk.h"
#include "ytab.h"
#include <awkerr.h>

FILE*   infile = NULL;

int  errorflag = 0;

#define  MAXFLD  2        // Initial number of fields

static int  refldbld (const char *, const char *);

/// Create fields up to $nf inclusive
void makefields (int nf)
{
  if (nf < interp->nfields)
    return;   //all good

  size_t sz = (nf + 1) * sizeof (Cell);
  interp->fldtab = (Cell*)realloc (interp->fldtab, sz);
  if (!interp->fldtab)
    FATAL (AWK_ERR_NOMEM, "out of space growing fields to %d", nf);

  for (int i = interp->nfields+1; i <= nf; i++)
  {
    char temp[50];

    interp->fldtab[i] = { OCELL, CFLD, NULL, NULL, 0.0, NUM | STR };
    sprintf (temp, "%d", i);
    interp->fldtab[i].nval = tostring (temp);
  }
  interp->nfields = nf;
}

/// Release all allocated fields
void freefields ()
{
  int i;
  for (i = 0; i < interp->nfields; i++)
    freecell (&interp->fldtab[i]);

  xfree (interp->fldtab);
}

/// Initialize $0 and fields
void recinit ()
{
  //first allocation - deal with $0
  interp->fldtab = (Cell*)malloc (sizeof(Cell));
  if (!interp->fldtab)
    FATAL (AWK_ERR_NOMEM, "out of space for $0");
  interp->fldtab[0] = { OCELL, CREC, NULL, NULL, 0.0, NUM | STR };
  interp->fldtab[0].nval = tostring ("0");
  interp->nfields = 0;
  makefields (MAXFLD);
}

/// Find first filename argument
void initgetrec (void)
{
  int i;
  const char *p;

  infile = NULL;
  for (i = 1; i < interp->argc; i++)
  {
    p = getargv (i); /* find 1st real filename */
    if (p == NULL || *p == '\0')
    {  /* deleted or zapped */
      interp->argno++;
      continue;
    }
    if (isclvar (p))
      setclvar (p);  /* a command line assignment before filename */
    else
      return; 
    interp->argno++;
  }
  infile = stdin;    /* no filenames, so use stdin */
}

/// Quick and dirty character quoting can quote a few short character strings
const char *quote (const char* in)
{
  static char buf[256];
  char *out = buf;

  if (!in)
  {
    strcpy (out, "(null)");
    return buf;
  }

  while (*in)
  {
    switch (*in)
    {
    case '\n':
      *out++ = '\\';
      *out++ = 'n';
      break;
    case '\r':
      *out++ = '\\';
      *out++ = 'r';
      break;
    case '\t':
      *out++ = '\\';
      *out++ = 't';
      break;
    default:
      *out++ = *in;
    }
    in++;
    if ((size_t)(out - buf) > sizeof (buf) - 5)
    {
      strcpy (out, "...");
      return buf;
    }
  }
  *out++ = 0;
  return buf;
}

/// Get next input record
int getrec (Cell *cell)
{
  const char* file = 0;

  dprintf ("RS=<%s>, FS=<%s>, ARGC=%d, FILENAME=%s\n",
    quote(RS), quote(FS), interp->argc, FILENAME);
  if (isrec (cell))
  {
    interp->donefld = 0;
    interp->donerec = 1;
  }
  while (interp->argno < interp->argc || infile == stdin)
  {
    dprintf ("argno=%d, file=|%s|\n", interp->argno, NN(file));
    if (infile == NULL)
    {
      /* have to open a new file */
      file = getargv (interp->argno);
      if (file == NULL || *file == '\0')
      {
        /* deleted or zapped */
        interp->argno++;
        continue;
      }
      if (isclvar (file))
      {
        /* a var=value arg */
        setclvar (file);
        interp->argno++;
        continue;
      }
      xfree (FILENAME);
      FILENAME = strdup(file);
      dprintf ("opening file %s\n", file);
      if (*file == '-' && *(file + 1) == '\0')
        infile = stdin;
      else if ((infile = fopen (file, "r")) == NULL)
        FATAL (AWK_ERR_INFILE, "can't open file %s", file);
      FNR = 0;
    }
    if (readrec (cell, infile))
    {
      NR = NR + 1;
      FNR = FNR + 1;
      return 1;
    }
    /* EOF arrived on this file; set up next */
    nextfile ();
  }
  return 0;  /* true end of file */
}

void nextfile (void)
{
  if (infile != NULL && infile != stdin)
    fclose (infile);
  infile = NULL;
  interp->argno++;
}

/// Get input char from input file or from input redirection function
int awkgetc (FILE *inf)
{
  int c = (inf == interp->files[0].fp && interp->inredir) ? interp->inredir() : getc (inf);
  return c;
}

// write string to output file or send it to output redirection function
int awkputs (const char *str, FILE *fp)
{
  if (fp == interp->files[1].fp && interp->outredir)
    return interp->outredir (str, strlen (str));
  else
    return fputs (str, fp);
}

/// Read one record in cell's string value
int readrec (Cell *cell, FILE *inf)
{
  int sep, c;
  char *rr, *buf;
  size_t bufsize = RECSIZE;

  xfree (cell->sval);
  buf = (char *)malloc (bufsize);
  if (!buf)
    FATAL (AWK_ERR_NOMEM, "Not enough memory for readrec");

  if ((sep = *RS) == 0)
  {
    sep = '\n';
    while ((c = awkgetc (inf)) == '\n' && c != EOF)  /* skip leading \n's */
      ;
    if (c != EOF)
      ungetc (c, inf);
  }
  for (rr = buf; ; )
  {
    for (; (c = awkgetc (inf)) != sep && c != EOF; )
    {
      if ((size_t)(rr - buf + 1) > bufsize)
        if (!adjbuf (&buf, &bufsize, 1 + rr - buf, RECSIZE, &rr))
          FATAL (AWK_ERR_NOMEM, "input record `%.30s...' too long", buf);
      *rr++ = c;
    }
    if (*RS == sep || c == EOF)
      break;

    /*
      **RS != sep and sep is '\n' and c == '\n'
      This is the case where RS = 0 and records are separated by two
      consecutive \n
    */
    if ((c = awkgetc (inf)) == '\n' || c == EOF) /* 2 in a row */
      break;
    if (!adjbuf (&buf, &bufsize, 2 + rr - buf, RECSIZE, &rr))
      FATAL (AWK_ERR_NOMEM, "input record `%.30s...' too long", buf);
    *rr++ = '\n';
    *rr++ = c;
  }
#if 0
  //Not needed; buffer has been adjusted inside the loop
  if (!adjbuf (&buf, &bufsize, 1 + rr - buf, recsize, &rr))
    FATAL ("input record `%.30s...' too long", buf);
#endif
  *rr = 0;
  dprintf ("readrec saw <%s>, returns %d\n", buf, c == EOF && rr == buf ? 0 : 1);
  cell->sval = buf;
  cell->tval = STR;
  if (is_number (cell->sval))
  {
    cell->fval = atof (cell->sval);
    cell->tval |= NUM | CONVC;
  }
  return c == EOF && rr == buf ? 0 : 1;
}

/// Get ARGV[n]
const char *getargv (int n)
{
  Cell *x;
  const char *s;
  char temp[50];

  sprintf (temp, "%d", n);
  if (lookup (temp, interp->argvtab) == NULL)
    return NULL;
  x = setsymtab (temp, "", 0.0, STR, interp->argvtab);
  s = getsval (x);
  dprintf ("getargv(%d) returns |%s|\n", n, s);
  return s;
}

/*!
  Command line variable.
  Set var=value from s

  Assumes input string has correct format.
*/
void setclvar (const char *s)
{
  const char *p;
  Cell *q;

  for (p = s; *p != '='; p++)
    ;
  p = qstring (p+1, '\0');
  s = qstring (s, '=');
  q = setsymtab (s, p, 0.0, STR, symtab);
  setsval (q, p);
  if (is_number (q->sval))
  {
    q->fval = atof (q->sval);
    q->tval |= NUM;
  }
  dprintf ("command line set %s to |%s|\n", s, p);
}

/// Create fields from current record
void fldbld (void)
{
  char *fields, *fb, *fe, sep;
  Cell *p;
  int i, j;

  if (interp->donefld)
    return;

  fields = strdup (getsval (&interp->fldtab[0]));
  i = 0;  /* number of fields accumulated here */
  fb = fields;        //beginning of field
  if (strlen (FS) > 1)
  {
    /* it's a regular expression */
    i = refldbld (fields, FS);
  }
  else if ((sep = *FS) == ' ')
  {
    /* default whitespace */
    for (i = 0; ; )
    {
      while (*fb && (*fb == ' ' || *fb == '\t' || *fb == '\n'))
        fb++;
      if (!*fb)
        break;
      fe = fb;        //end of field
      while (*fe && *fe != ' ' && *fe != '\t' && *fe != '\n')
        fe++;
      if (*fe)
        *fe++ = 0;
      if (++i > interp->nfields)
        growfldtab (i);
      xfree (interp->fldtab[i].sval);
      interp->fldtab[i].sval = strdup(fb);
      fb = fe;
    }
  }
  else if ((sep = *FS) == 0)
  {
    /* new: FS="" => 1 char/field */
    for (i = 0; *fb != 0; fb++)
    {
      char buf[2];
      if (++i > interp->nfields)
        growfldtab (i);
      buf[0] = *fb;
      buf[1] = 0;
      xfree (interp->fldtab[i].sval);
      interp->fldtab[i].sval = strdup (buf);
    }
  }
  else if (*fb != 0)
  {
    /* if 0, it's a null field */

 /* subtlecase : if length(FS) == 1 && length(RS > 0)
  * \n is NOT a field separator (cf awk book 61,84).
  * this variable is tested in the inner while loop.
  */
    int rtest = '\n';  /* normal case */
    int end_seen = 0;
    if (strlen (RS) > 0)
      rtest = '\0';
    fe = fb;
    for (i = 0; !end_seen; )
    {
      fb = fe;
      while (*fe && *fe != sep && *fe != rtest)
        fe++;
      if (*fe)
        *fe++ = 0;
      else
        end_seen = 1;

      if (++i > interp->nfields)
        growfldtab (i);
      xfree (interp->fldtab[i].sval);
      interp->fldtab[i].sval = strdup(fb);
    }
  }
  cleanfld (i + 1, interp->lastfld);  /* clean out junk from previous record */
  interp->lastfld = i;
  interp->donefld = 1;
  
  for (j = 1, p = &interp->fldtab[1]; j <= interp->lastfld; j++, p++)
  {
    p->tval = STR;
    if (is_number (p->sval))
    {
      p->fval = atof (p->sval);
      p->tval |= (NUM | CONVC);
    }
  }
  NF = interp->lastfld;
  interp->donerec = 1; /* restore */
  free (fields);
#ifndef NDEBUG
  if (dbg)
  {
    for (j = 0; j <= interp->lastfld; j++)
      dprintf ("field %d (%s): |%s|\n", j, interp->fldtab[j].nval, interp->fldtab[j].sval);
  }
#endif
}

/// Clean out fields n1 .. n2 inclusive
void cleanfld (int n1, int n2)
{
  /* nvals remain intact */
  int i;

  for (i = n1; i <= n2; i++)
  {
    xfree (interp->fldtab[i].sval);
    interp->fldtab[i].tval = STR | NUM;
    interp->fldtab[i].fval = 0.;
    interp->fldtab[i].fmt = NULL;
  }
}

/// Add field n after end of existing lastfld
void newfld (int n)
{
  if (n > interp->nfields)
    growfldtab (n);
  NF = interp->lastfld = n;
}

/// Set lastfld cleaning fldtab cells if necessary
void setlastfld (int n)
{
  if (n > interp->nfields)
    growfldtab (n);

  if (interp->lastfld < n)
    cleanfld (interp->lastfld + 1, n);
  else
    cleanfld (n + 1, interp->lastfld);

  interp->lastfld = n;
}

/// Get nth field
Cell *fieldadr (int n)
{
  if (n < 0)
    FATAL (AWK_ERR_ARG, "trying to access out of range field %d", n);
  if (n && !interp->donefld)
    fldbld ();
  else if (!n && !interp->donerec)
    recbld ();
  if (n > interp->nfields)  /* fields after NF are empty */
    growfldtab (n);  /* but does not increase NF */
  return &interp->fldtab[n];
}

/// Make new fields up to at least $n
void growfldtab (size_t n)
{
  size_t nf = 2 * interp->nfields;

  if (n > nf)
    nf = n;
  makefields ((int)nf);
}

/// Build fields from reg expr in FS
int refldbld (const char *rec, const char *fs)
{
  char *fr, *fields;
  int i, tempstat;
  fa *pfa;

  fields = strdup (rec);
  fr = fields;
  *fr = '\0';
  if (*rec == '\0')
    return 0;
  pfa = makedfa (fs, 1);
  dprintf ("into refldbld, rec = <%s>, pat = <%s>\n", rec, fs);
  tempstat = pfa->initstat;
  for (i = 1; ; i++)
  {
    if (i > interp->nfields)
      growfldtab (i);
    xfree (interp->fldtab[i].sval);
    interp->fldtab[i].tval = STR;
    interp->fldtab[i].sval = strdup(fr);
    dprintf ("refldbld: i=%d\n", i);
    if (nematch (pfa, rec))
    {
      pfa->initstat = 2;  /* horrible coupling to b.c */
      dprintf ("match %s (%d chars)\n", patbeg, patlen);
      strncpy (fr, rec, patbeg - rec);
      fr += patbeg - rec + 1;
      *(fr - 1) = '\0';
      rec = patbeg + patlen;
    }
    else
    {
      dprintf ("no match %s\n", rec);
      strcpy (fr, rec);
      pfa->initstat = tempstat;
      break;
    }
  }
  free (fields);
  return i;
}

/// Create $0 from $1..$NF if necessary
void recbld ()
{
  int i;
  char *r;
  const char *p;

  if (interp->donerec)
    return;

  char *buf = (char *)malloc (RECSIZE);
  size_t sz = RECSIZE;

  r = buf;
  for (i = 1; i <= NF; i++)
  {
    p = getsval (&interp->fldtab[i]);
    if (!adjbuf (&buf, &sz, 2 + strlen (p) + strlen (OFS) + r - buf,
         RECSIZE, &r))
      FATAL (AWK_ERR_NOMEM, "created $0 `%.30s...' too long", buf);
    while ((*r = *p++) != 0)
      r++;
    if (i < NF)
    {
      for (p = OFS; (*r = *p++) != 0; )
        r++;
    }
  }
  *r = '\0';
  xfree (interp->fldtab[0].sval);
  interp->fldtab[0].sval = buf;
  dprintf ("in recbld $0=|%s|\n", buf);
  interp->donerec = 1;
  interp->fldtab[0].tval = STR;
  if (is_number (interp->fldtab[0].sval))
  {
    interp->fldtab[0].tval |= NUM;
    interp->fldtab[0].fval = atof (interp->fldtab[0].sval);
  }
}

double errcheck (double x, const char *s)
{
  if (errno == EDOM)
  {
    errno = 0;
    WARNING ("%s argument out of domain", s);
    x = 1;
  }
  else if (errno == ERANGE)
  {
    errno = 0;
    WARNING ("%s result out of range", s);
    x = 1;
  }
  return x;
}

/*!
  Checks if s is a command line variable assignment 
  of the form var=something
*/
int isclvar (const char *s)
{
  const char *os = s;

  if (!isalpha (*s) && *s != '_')
    return 0;
  for (; *s; s++)
    if (!(isalnum (*s) || *s == '_'))
      break;
  return *s == '=' && s > os && *(s + 1) != '=';
}

/* strtod is supposed to be a proper test of what's a valid number */
/* appears to be broken in gcc on linux: thinks 0x123 is a valid FP number */
/* wrong: violates 4.10.1.4 of ansi C standard */

#include <math.h>
int is_number (const char *s)
{
  double r;
  char *ep;

  if (!s)
    return 0;

  errno = 0;
  r = strtod (s, &ep);
  if (ep == s || r == HUGE_VAL || errno == ERANGE)
    return 0;
  while (*ep == ' ' || *ep == '\t' || *ep == '\n')
    ep++;
  if (*ep == '\0')
    return 1;
  else
    return 0;
}
