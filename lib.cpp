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
#include <awklib/err.h>
#include "proto.h"

static int  refldbld (const char *, const char *);



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
