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
#include <awklib/err.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif


#ifdef __APPLE__
extern char **environ;
#endif


/// Free elem s from ap (i.e., ap["s"])
void freeelem (Cell *ap, const char *s)
{
  Cell *p = ap->arrval->removesym (s);
  delete p;
}


void funnyvar (Cell *vp, const char *rw)
{
  if (vp->isarr ())
    FATAL (AWK_ERR_ARG, "can't %s %s; it's an array name.", rw, vp->nval);
  if (vp->tval & FCN)
    FATAL (AWK_ERR_ARG, "can't %s %s; it's a function.", rw, vp->nval);
  WARNING ("funny variable %s %p: n=%s s=\"%s\" f=%g t=0x%x",
    rw, vp, vp->nval, vp->sval, vp->fval, vp->tval);
}

/// Make a copy of string s
char *tostring (const char *s)
{
  char* p = new char[(s ? strlen (s) : 0) + 1];
  strcpy (p, s ? s : "");
  return p;
}

/// Collect string up to next delim
char *qstring (const char *is, int delim)
{
  const char *os = is;
  const char *pd;
  int c, n;

  const char *s = is;
  char *buf, *bp;

  if (!(pd = strchr (is, delim)))
    FATAL (AWK_ERR_ARG, "invalid delimiter for qstring");

  buf = new char[pd - is + 3];
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
  return buf;
}

const char *flags2str (int flags)
{
  static const struct ftab {
    const char *name;
    int value;
  } flagtab[] = {
    { "NUM", NUM },
    { "STR", STR },
    { "ARR", ARR },
    { "FCN", FCN },
    { "CONVC", CONVC },
    { NULL, 0 }
  };
  static char buf[100];
  int i;
  char *cp = buf;

  for (i = 0; flagtab[i].name != NULL; i++)
  {
    if ((flags & flagtab[i].value) != 0)
    {
      if (cp > buf)
        *cp++ = '|';
      strcpy (cp, flagtab[i].name);
      cp += strlen (cp);
    }
  }

  return buf;
}
