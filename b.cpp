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

#include <assert.h>
#include "awk.h"
#include "ytab.h"
#include "proto.h"

#include <awklib/err.h>

extern Interpreter* interp;
using namespace std;

int hexdigit (char c)
{
  if (isdigit (c))
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else
    return 0;
}
/*
  There is no flavor of std::regex that exactly matches GAWK (or One True AWK)
  syntax. The std::regex::constants::awk does not accept hex escaping (\xhh)
  and std::regex::constants::ECMAScript does not accept octal escaping (\ddd).

  Here we convert a string containing \xhh to \ddd. The number of characters
  the same.
*/
void requote (string& str)
{
  auto s = str.begin ();
  while (s != str.end())
  {
    if (*s == '\\' && *(s + 1) == 'x'
      && isxdigit (*(s + 2)) && isxdigit (*(s + 3)))
    {
      int n = 0;
      *s++ = '\\';
      n = (hexdigit (*(s + 1)) << 4) + hexdigit (*(s + 2));
      *s++ = (n >> 6) + '0';
      *s++ = ((n >> 3) & 0x07) + '0';
      *s++ = (n & 0x07) + '0';
    }
    else
      s++;
  }
}


/// Compile a string into a regex and return the node containing that regex
Node* nodedfa (const char *s, int anchor)
{
  Cell* a = interp->makedfa (s, anchor);
  Node *x = new Node (0, nullproc, 0, (Node*)a);
  x->ntype = NVALUE;

  return x;
}

bool match (Cell *f, const char *p0)  /* shortest match ? */
{
  assert (f->isregex ());
  regex_constants::match_flag_type flags = regex_constants::match_default;
  if ((f->flags & ANCHORED) != 0)
    flags |= (regex_constants::match_not_bol | regex_constants::match_not_eol);
  
  bool result = regex_search (p0, *f->re, flags);
  return result;
}

/// Longest match, for sub
bool pmatch (Cell *f, const char *p0, size_t& start, size_t& len)
{
  assert (f->isregex ());
  regex_constants::match_flag_type flags = regex_constants::match_default;
  if ((f->flags & ANCHORED) != 0)
    flags |= (regex_constants::match_not_bol | regex_constants::match_not_eol);

  cmatch m;
  if (regex_search (p0, m, *f->re, flags))
  {
    start = m.position ();
    len = m.length ();
    return true;
  }
  return false;
}

/// split(a[0], a[1], a[2]);
Cell* split (const Node::Arguments& a, int)
{
  Cell* x = 0;

  Cell* y = execute (a[0]);  /* source string */
  char *orig_s = tostring (y->getsval ());
  char* s = orig_s;
  bool temp_re = false;

  Cell* ap = execute (a[1]);  /* array name */
  if (ap->isarr ())
    delete ap->arrval;
  else
    delete ap->sval;
  ap->flags &= ~(STR | NUM);
  ap->csub = Cell::subtype::CARR;
  ap->arrval = new Array (NSYMTAB);

  const char* fs = 0;
  regex* re = 0;
  if (!a[2])    /* fs string */
  {
    fs = FS;
  }
  else if (a[2]->ntype == NVALUE && a[2]->to_cell ()->isregex ())
  { // precompiled regexp
    if (strlen (a[2]->to_cell ()->nval))
      re = a[2]->to_cell ()->re;
    else
    {
      /* split(s, a, //); have to arrange that it looks like empty sep */
      fs = "";
    }
  }
  else
  {  /* split(str,arr,"string") */
    x = execute (a[2]);
    fs = x->getsval ();
    if (strlen (fs) > 1)
    {
      re = new regex (fs, regex_constants::awk);
      temp_re = true;
    }
  }

  int n = 0;
  char num[10], temp;
  if (re)
  {
    cmatch m;

    while (regex_search (s, m, *re))
    {
      n++;
      sprintf (num, "%d", n);
      char* patbeg = s + m.position ();
      temp = *patbeg;
      *patbeg = '\0';
      if (is_number (s))
        ap->arrval->setsym (num, s, atof (s), STR | NUM);
      else
        ap->arrval->setsym (num, s, 0.0, STR);
      *patbeg = temp;
      s = patbeg + m.length ();
    }
  }
  else
  {
    char sep = *fs;
    if (sep == ' ')
    {
      for (n = 0; ; )
      {
        while (*s == ' ' || *s == '\t' || *s == '\n')
          s++;
        if (*s == 0)
          break;
        n++;
        const char* t = s;
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
        const char *t = s;
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
  }
  delete orig_s;

  tempfree (ap);
  tempfree (y);

  if (temp_re)
    delete re;

  x = new Cell (Cell::type::OCELL, Cell::subtype::CTEMP, 0, 0, 0., 0);
  x->flags = NUM;
  x->fval = n;
  return x;
}

extern Cell *True, *False;

/*
  Change replacement string for sub and gsub replacing the AWK substitution
  operator (&) with std::regex one ($&).
*/
void format_repl (string& s)
{
  string out;
  for (auto pc = s.begin (); pc != s.end (); pc++)
  {
    if (*pc == '\\')
    {
      if (++pc == s.end ())
        break;
    }
    else if (*pc == '&')
      out.push_back ('$');
    out.push_back (*pc);
  }
  s = out;
}


/// substitute command
/// a[0] = regexp string
/// a[1] = compiled regexp
/// a[2] = replacement string
/// a[3] = target string
Cell* sub (const Node::Arguments& a, int)
{
  Cell* result = False;
  Cell* y;
  Cell *x = execute (a[3]);  /* target string */
  regex* re;
  bool temp_re = false;
  if (!a[0])  // a[1] is already-compiled regexp
    re = a[1]->to_cell ()->re;
  else
  {
    y = execute (a[0]);
    re = new regex (y->sval, regex_constants::awk);
    temp_re = true;
    tempfree (y);
  }
  y = execute (a[2]);  /* replacement string */
  string repl (y->sval);
  string target (x->getsval ());
  format_repl (repl);
  smatch m;
  if (regex_search (target, m, *re))
  {
    target = regex_replace (target, *re, repl, regex_constants::format_first_only);
    x->setsval (target.c_str ());
    result = True;
  }
  tempfree (x);
  tempfree (y);
  return result;
}

/// global substitute
/// a[0] = regexp string
/// a[1] = compiled regexp
/// a[2] = replacement string
/// a[3] = target string
Cell* gsub (const Node::Arguments& a, int)
{
  int num = 0;

  Cell* y;
  Cell* x = execute (a[3]);  /* target string */
  regex* re;
  bool temp_re = false;
  if (!a[0])  // a[1] is already-compiled regexp
    re = a[1]->to_cell ()->re;
  else
  {
    y = execute (a[0]);
    re = new regex (y->sval, regex_constants::awk);
    temp_re = true;
    tempfree (y);
  }
  y = execute (a[2]);  /* replacement string */
  string repl (y->sval);
  string target (x->getsval ());
  format_repl (repl);
  smatch m;
  string::const_iterator i = target.begin ();
  string result;
  while (regex_search (i, target.cend (), m, *re))
  {
    result += m.prefix ();
    result += m.format (repl);
    i += m.position () + m.length();
    num++;
  }
  result.append (i, target.cend ());
  x->setsval (result.c_str ());
  tempfree (x);
  tempfree (y);

  x = new Cell (Cell::type::OCELL, Cell::subtype::CTEMP, 0, 0, 0., 0);
  x->flags = NUM;
  x->fval = num;
  return x;
}
