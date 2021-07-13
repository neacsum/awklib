/*!
  \file libmain.cpp
  \brief API implementation for embedded AWK library

  (c) Mircea Neacsu 2019
  See README file for full copyright information.
*/

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


static const char  *version = 
  "version " 
#ifdef _DEBUG
  " DEBUG "
#endif
  __DATE__;

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "awk.h"
#include "ytab.h"
#include <awklib/awk.h>
#include <awklib/err.h>

#define  MAX_PFILE  20  /* max number of -f's */

extern  FILE* yyin;     /* lex input file */
extern char *curfname;  ///<current function name

Interpreter *interp;      ///< current interpreter status block

char errmsg[1024];      ///< last error message
extern  int errorflag;  ///< non-zero if any syntax errors; set by yyerror
extern Node *winner;    // parser stores root of program tree here

#ifndef NDEBUG
int dbg  = 0;
void print_tree (Node *n, int indent);
const char *tokname (int tok);
#if YYDEBUG
extern int yydebug;
#endif
#endif

/*!
  Initialize interpreter and optionally sets user defined variables
  vars - array of strings "var=value"

  \return  - an interpreter status block if successful or NULL otherwise
*/
extern "C" 
AWKINTERP* awk_init (const char **vars)
{
  signal (SIGFPE, fpecatch);
  try {
    interp = (Interpreter *)calloc (1, sizeof (Interpreter));
    if (!interp)
      FATAL (AWK_ERR_NOMEM, "Out of memory");
    interp->srand_seed = 1;
    interp->argno = 1;
    srand ((unsigned int)interp->srand_seed);
    interp->symtab = symtab = makearray (NSYMTAB);
    interp->predefs = (Cell**)calloc (NPREDEF, sizeof (Cell*));
    syminit ();

    //add all user set variables
    while (vars && *vars)
    {
      if (isclvar (*vars))
        setclvar (*vars++);
      else
        FATAL (AWK_ERR_INVVAR, "invalid variable argument: %s", vars);
    }

    //add a fake argv[0]
    awk_addarg ((AWKINTERP*)interp, "AWKLIB");
  }
  catch (int)
  {
    freearray (symtab);
    xfree (interp);
    return 0;
  }
  interp->status = AWKS_INIT;
  return (AWKINTERP*)interp;
}

/*!
  Set a string as the AWK program text.
*/
int awk_setprog (AWKINTERP *pinter, const char *prog)
{
  interp = (Interpreter*)pinter;
  try {
    if (interp->status != AWKS_INIT)
      FATAL (AWK_ERR_BADSTAT, "Bad interpreter status (%d)", interp->status);
    if (interp->nprog)
      FATAL (AWK_ERR_PROGFILE, "Program file already set");

    xfree (interp->lexprog);
    interp->lexptr = interp->lexprog = strdup (prog);
    return 1;
  }
  catch (int) {
    return 0;
  }
  return 1;
}

int awk_addprogfile (AWKINTERP *pinter, const char *progfile)
{
  interp = (Interpreter*)pinter;
  try {
    if (interp->status != AWKS_INIT)
      FATAL (AWK_ERR_BADSTAT, "Bad interpreter status (%d)", interp->status);
    if (interp->lexprog)
      FATAL (AWK_ERR_PROGFILE, "Program string already set");
    if (interp->nprog)
      interp->progs = (char**)realloc (interp->progs, (interp->nprog + 1) * sizeof (char*));
    else
      interp->progs = (char **)malloc (sizeof (char*));
    interp->progs[interp->nprog++] = strdup (progfile);
  }
  catch (int) {
    return 0;
  }
  return 1;
}

/*!
  Add another argument (input file or assignment)
*/
int awk_addarg (AWKINTERP *pinter, const char *arg)
{
  interp = (Interpreter*)pinter;
  if (interp->status >= AWKS_RUN)
    return 0;
  if (interp->argc)
    interp->argv = (char**)realloc (interp->argv, (interp->argc + 1) * sizeof (char*));
  else
    interp->argv = (char **)malloc (sizeof (char*));
  interp->argv[interp->argc++] = strdup (arg);
  return 1;
}

/// Compile an AWK program
int awk_compile (AWKINTERP *pinter)
{
  interp = (Interpreter*)pinter;
  try {
    if (interp->status != AWKS_INIT)
      FATAL (AWK_ERR_BADSTAT, "Bad interpreter status (%d)", interp->status);
    if (!interp->lexprog && !interp->nprog)
      FATAL (AWK_ERR_NOPROG, "No program file");

    setlocale (LC_CTYPE, "");
    setlocale (LC_NUMERIC, "C"); /* for parsing cmdline & prog */

    // (Re)init all parsing variables
    yyinit ();
    yyin = NULL;
    interp->status = AWKS_COMPILING;
    yyparse ();
    setlocale (LC_NUMERIC, ""); /* back to whatever it is locally */
    if (errorflag == 0)
    {
      interp->status = AWKS_COMPILED;
#ifndef NDEBUG
      if (dbg > 2)
        print_tree (winner, 1);
#endif
      interp->prog_root = winner;

      //things we can/must initialize now
      envinit ();
      stdinit ();
    }

  }
  catch (int) {
    freenode (interp->prog_root);
    interp->status = AWKS_DONE;
    return 0;
  }
  return 1;
}

/// Execute an AWK program
int awk_exec (AWKINTERP *pinter)
{
  interp = (Interpreter*)pinter;
  if (interp->status != AWKS_COMPILED)
  {
    strcpy (errmsg, "awk_exec: no compiled program");
    errorflag = AWK_ERR_NOPROG;
    return 0;
  }

  symtab = interp->symtab;
  try {
    recinit ();
    arginit ();
    initgetrec ();
    run (interp->prog_root);
  }
  catch (int& x) {
    interp->status = AWKS_DONE;
    errorflag = x;
  }
  return errorflag;
}

int awk_run (AWKINTERP* pinter, const char *progfile)
{
  interp = (Interpreter*)pinter;
  symtab = interp->symtab;
  try {
    if (interp->status != AWKS_INIT)
      FATAL (AWK_ERR_BADSTAT, "Bad interpreter status (%d)", interp->status);
    interp->lexptr = interp->lexprog = strdup (progfile);
    setlocale (LC_CTYPE, "");
    setlocale (LC_NUMERIC, "C"); /* for parsing cmdline & prog */

    // (Re)init all parsing variables
    yyinit ();
    yyin = NULL;
    interp->status = AWKS_COMPILING;
    yyparse ();

    setlocale (LC_NUMERIC, ""); /* back to whatever it is locally */
    if (errorflag == 0)
    {
#ifndef NDEBUG
      if (dbg > 2)
        print_tree (winner, 1);
#endif
      interp->prog_root = winner;

      envinit ();
      stdinit ();
      recinit ();
      arginit ();
      initgetrec ();
      run (interp->prog_root);
    }
  }
  catch (int& x)
  {
    interp->status = AWKS_DONE;
    errorflag = x;
  }
  return errorflag;
}

/// Release all resources claimed by AWK interpreter
void awk_end (AWKINTERP *pinter)
{
  interp = (Interpreter*)pinter;
  for (int i = 0; i < interp->argc; i++)
    free (interp->argv[i]);
  dprintf ("freed argv[0] to argv[%d]\n", interp->argc);
  xfree (interp->argv);
  freenode (interp->prog_root);
  dprintf ("freeing symbol table\n");
  freearray (interp->symtab);
  for (int i = 0; i < interp->nprog; i++)
    free (interp->progs[i]);
  xfree (interp->progs);
  xfree (interp->lexprog);
  xfree (interp->files);
  xfree (interp->predefs);
  free (interp);
}

/// Return last error code and optionally the last error message
int awk_err (const char **msg)
{
  if (msg)
    *msg = errorflag ? strdup (errmsg) : strdup ("OK");
  return errorflag;
}

/// Set a new debug level. Return previous debug level
int awk_setdebug (int level)
{
#ifndef NDEBUG
  int prev = dbg;
  dbg = level;
#ifdef YYDEBUG
  if (dbg > 1)
    yydebug = 1;
#endif
  errprintf ("Debug level is %d\n", dbg);
  return prev;
#endif
  return 0;
}

/// Redirect input to a user function
void awk_infunc (AWKINTERP* pinter, inproc user_input)
{
  ((Interpreter*)pinter)->inredir = user_input;
}

/// Redirect output to a user function
void awk_outfunc (AWKINTERP* pinter, outproc user_output)
{
  ((Interpreter*)pinter)->outredir = user_output;
}

/// Redirects output to a file
int awk_setoutput (AWKINTERP* pinter, const char *fname)
{
  FILE_STRUC *fs = &((Interpreter*)pinter)->files[1];
  FILE *f = fopen (fname, "w");
  if (!f)
    return 0; //something wrong

  if (fs->fp != stdout)
    fclose (fs->fp);
  fs->fp = f;

  free (fs->fname);
  fs->fname = strdup (fname);
  return 1;
}

/// Redirects input to read from a file
int awk_setinput (AWKINTERP* pinter, const char *fname)
{
  FILE_STRUC *fs = &((Interpreter*)pinter)->files[0];
  FILE *f = fopen (fname, "r");
  if (!f)
    return 0; //something wrong

  if (fs->fp != stdin)
    fclose (fs->fp);
  fs->fp = f;

  free (fs->fname);
  fs->fname = strdup (fname);
  return 1;
}

/// Retrieve a variable from symbol table
int awk_getvar (AWKINTERP * pinter, awksymb * var)
{
  interp = (Interpreter*)pinter;
  symtab = interp->symtab;
  var->flags = 0;
  try {
    Cell *cp = NULL;
    if (var->name[0] == '$' && is_number (var->name+1) 
     && interp->status == AWKS_RUN)
    {
      int n = (int)atof (var->name+1);
      fldbld ();
      if (n >= 0 && n <= interp->lastfld)
        cp = &interp->fldtab[n];
    }
    else
      cp = lookup (var->name, symtab);
    if (!cp)
    {
      sprintf (errmsg, "awk_getvar: not found %s", var->name);
      return (errorflag = AWK_ERR_NOVAR); //not found
    }

    if (cp->tval & (FCN | EXTFUN))
    {
      sprintf (errmsg, "awk_getvar: bad variable type %s", var->name);
      return (errorflag = AWK_ERR_INVVAR); //invalid variable type
    }

    if (cp->tval & ARR)
    {
      var->flags = AWKSYMB_ARR;
      if (!var->index)
      {
        sprintf (errmsg, "awk_getvar: %s is an array", var->name);
        return AWK_ERR_ARRAY;
      }
      Array *arr = (Array*)cp->sval;
      cp = lookup (var->index, arr);
      if (!cp)
      {
        sprintf (errmsg, "awk_getvar: not found %s[%s]", var->name, var->index);
        return (errorflag = AWK_ERR_NOVAR); //index not found
      }
    }
    var->flags &= ~(AWKSYMB_NUM | AWKSYMB_STR);
    var->fval = 0.;
    var->sval = NULL;
    if (cp->tval & NUM)
    {
      var->flags |= AWKSYMB_NUM;
      var->fval = cp->fval;
    }
    if (cp->tval & STR)
    {
      var->flags |= AWKSYMB_STR;
      var->sval = strdup (cp->sval);
    }
  }
  catch (int& x) {
    return (errorflag = x);
  }

  return 1;
}

/*! 
  Changes the value of a variable from symbol table or creates a new variable
*/
int awk_setvar (AWKINTERP * pinter, awksymb * var)
{
  Cell *cp = NULL;
  int n;
  bool is_field = false;

  interp = (Interpreter*)pinter;
  symtab = interp->symtab;
  try {
    if (var->name[0] == '$' && is_number (var->name+1)
     && interp->status == AWKS_RUN)
    {
      n = (int)atof (var->name+1);
      if (n < 0 || n >= interp->nfields)
      {
        sprintf (errmsg, "awk_setvar: invalid field %s", var->name);
        return (errorflag = AWK_ERR_INVVAR);
      }
      cp = &interp->fldtab[n];
      is_field = true;
    }
    else
      cp = lookup (var->name, symtab);
    if (!cp && !is_field)
    {
      //new variable
      if ((var->flags & AWKSYMB_ARR) != 0 && !var->index)
      {
        sprintf (errmsg, "awk_setvar: variable is an array %s", var->name);
        return (errorflag = AWK_ERR_ARRAY);
      }
      int t = (var->flags & AWKSYMB_ARR) ? ARR : (STR | NUM);
      cp = setsymtab (var->name, "", 0., t, symtab);
      if (cp->tval & ARR)
        cp->sval = (char*)makearray (20); //arbitrary size
    }
    if (cp->tval & ARR)
    {
      //array
      if (!var->index)
      {
        sprintf (errmsg, "awk_setvar: variable is an array %s", var->name);
        return (errorflag = AWK_ERR_ARRAY);
      }
      cp = setsymtab (var->index, "", 0., (STR | NUM), (Array*)cp->sval);
    }
    if ((cp->tval & (STR | NUM)) == 0)
    {
      sprintf (errmsg, "awk_setvar: invalid variable type %s", var->name);
      return AWK_ERR_INVTYPE;
    }
    if (var->flags & AWKSYMB_STR)
      setsval (cp, var->sval);
    else
      setfval (cp, var->fval);
    if (is_field)
    {
      if (n == 0)
        fldbld ();
      else
        recbld ();
    }
  }
  catch (int& x) {
    return (errorflag = x);
  }

  return 1;
}

int awk_addfunc (AWKINTERP *pinter, const char *fname, awkfunc fn, int nargs)
{
  interp = (Interpreter*)pinter;
  symtab = interp->symtab;
  try {
    Cell *cp = setsymtab (fname, NULL, nargs, EXTFUN, symtab);
    cp->tval = EXTFUN;
    cp->fval = nargs;
    xfree (cp->sval);
    cp->sval = (char *)fn;
  }
  catch (int&) {
    return 0;
  }

  return 1;
}


/*!
  Get 1 character from awk program
*/
int pgetc (void)
{
  int c;

  for (;;)
  {
    if (yyin == NULL)
    {
      if (interp->curprog >= interp->nprog)
        return EOF;
      if (strcmp(interp->progs[interp->curprog], "-") == 0)
        yyin = stdin;
      else if ((yyin = fopen(interp->progs[interp->curprog], "r")) == NULL)
        FATAL(AWK_ERR_INPROG, "can't open file %s", interp->progs[interp->curprog]);
      lineno = 1;
    }
    if ((c = getc(yyin)) != EOF)
      return c;
    if (yyin != stdin)
      fclose(yyin);
    yyin = NULL;
    interp->curprog++;
  }
}

/*!
  Return current program file name
*/
char *cursource (void)  /* current source file name */
{
  if (interp && interp->nprog > 0)
    return interp->progs[interp->curprog];
  else
    return NULL;
}

/// Output debug messages to stderr
#include <stdarg.h>

#if defined(WIN32)
// Avoid clashes with windows macrodefs
#undef CHAR
#undef DELETE
#undef IN
#include <Windows.h> //for OutDebugString
#endif

int errprintf (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int ret;
#ifdef WIN32
  static char buffer[1024];
  ret = vsnprintf (buffer, sizeof(buffer)-1, fmt, args);
  OutputDebugStringA (buffer);
#else
  ret = vfprintf (stderr, fmt, args);
#endif
  va_end (args);
  return ret;
}

#ifndef NDEBUG

void print_cell (Cell *c, int indent)
{
  static const char *ctype_str[OJUMP + 1] = {
    "0", "OCELL", "OBOOL", "OJUMP" };

  static const char *csub_str[JNEXTFILE+1] = {
    "CUNK", "CFLD", "CVAR", "CNAME", "CTEMP", "CCON", "CCOPY", "CFREE", "8", "9", "10",
    "BTRUE", "BFALSE", "13", "14", "15", "16", "17", "18", "19", "20",
    "JEXIT", "JNEXT", "JBREAK", "JCONT", "JRET", "JNEXTFILE" };

  dprintf ("%*cCell 0x%p ", indent, ' ', c);
  dprintf (" %s %s Flags: %s", ctype_str[c->ctype], csub_str[c->csub], flags2str(c->tval));
  dprintf ("%*cValue: %s(%lf)", indent, ' ', c->sval, c->fval);
  if (c->nval)
    dprintf (" Name: %s", c->nval);
  dprintf (" Next: %p\n", c->cnext);
}

void print_tree (Node *n, int indent)
{
  int i;
  ptrdiff_t nn = (ptrdiff_t)n;
  errprintf ("%*cNode 0x%p %s", indent, ' ', n, (nn < LASTTOKEN)? tokname((int)nn) : tokname (n->nobj));
  if (nn < LASTTOKEN)
  {
    errprintf ("- FLAG\n");
    return;
  }

  do
  {
    errprintf (" type %s", n->ntype == NVALUE ? "value" : n->ntype == NSTAT ? "statement" : "expression");
    errprintf (" %d arguments\n", n->args);
    indent++;
    for (i = 0; i < n->args; i++)
    {
      errprintf ("%*cArg %d of %s\n", indent, ' ', i, tokname(n->nobj));
      if (n->ntype == NVALUE)
        print_cell ((Cell *)n->narg[i], indent + 1);
      else if (n->narg[i])
        print_tree (n->narg[i], indent + 1);
      else
        errprintf ("%*cNull arg\n", indent + 1, ' ');
    }
    indent--;
    if ((n = n->nnext) != NULL)
      errprintf ("%*cNext node 0x%p %s", indent, ' ', n, tokname (n->nobj));
  } while (n);
}
#endif

/*======================= Error handling function ===========================*/

/// Generate error message and throws an exception
void FATAL (int err, const char *fmt, ...)
{
  va_list varg;
  if (interp)
    interp->err = err;
  errorflag = err;
  va_start (varg, fmt);
  vsprintf (errmsg, fmt, varg);
  va_end (varg);
  throw err;
}

/// Generate syntax error message and throws an exception
void SYNTAX (const char *fmt, ...)
{
  va_list varg;
  char *pb = errmsg;
  va_start (varg, fmt);
  pb += vsprintf (pb, fmt, varg);
  va_end (varg);
  pb += sprintf (pb, " at source line %d", lineno);
  if (curfname != NULL)
    pb += sprintf (pb, " in function %s", curfname);
  if (interp->status == AWKS_COMPILING && cursource () != NULL)
    pb += sprintf (pb, " source file %s", cursource ());
  errorflag = AWK_ERR_SYNTAX;
  throw AWK_ERR_SYNTAX;
}

/// Called by yyparse when for syntax errors
void yyerror (const char *s)
{
  SYNTAX ("%s", s);
}

void eprint (void)  /* try to print context around error */
{
  char *p, *q;
  int c;
  static int been_here = 0;
  extern char ebuf[], *ep;

  if (interp->status != AWKS_COMPILING || been_here++ > 0)
    return;
  if (ebuf == ep)
    return;
  p = ep - 1;
  if (p > ebuf && *p == '\n')
    p--;
  for (; p > ebuf && *p != '\n' && *p != '\0'; p--)
    ;
  while (*p == '\n')
    p++;
  fprintf (stderr, " context is\n\t");
  for (q = ep - 1; q >= p && *q != ' ' && *q != '\t' && *q != '\n'; q--)
    ;
  for (; p < q; p++)
    if (*p)
      putc (*p, stderr);
  fprintf (stderr, " >>> ");
  for (; p < ep; p++)
    if (*p)
      putc (*p, stderr);
  fprintf (stderr, " <<< ");
  if (*ep)
  {
    while ((c = input ()) != '\n' && c != '\0' && c != EOF)
    {
      putc (c, stderr);
    }
  }
  putc ('\n', stderr);
  ep = ebuf;
}

/// Print error location
void error ()
{
  errprintf ("\n");
  if (interp->status == AWKS_RUN && NR > 0)
  {
    errprintf (" input record number %d", (int)(FNR));
    if (strcmp (FILENAME, "-") != 0)
      errprintf (", file %s", FILENAME);
    errprintf ("\n");
  }
  if (interp->status == AWKS_COMPILING)
  {
    if (lineno)
      errprintf (" source line number %d", lineno);
    if (cursource ())
      errprintf (" source file %s", cursource ());
    errprintf ("\n");
  }
  eprint ();
}

/// Floating point error handler
void fpecatch (int n)
{
  FATAL (AWK_ERR_FPE, "floating point exception %d", n);
}

/// Warning sent to dprintf
void WARNING (const char *fmt, ...)
{
  char *fmt1 = (char*)malloc (strlen (fmt) + 10);
  va_list args;
  va_start (args, fmt);

  strcpy (fmt1, "AWK: ");
  strcat (fmt1, fmt);
#ifdef WIN32
  char buffer[1024];
  vsnprintf (buffer, sizeof (buffer) - 1, fmt1, args);
  OutputDebugStringA (buffer);
#else
  vfprintf (stderr, fmt1, args);
#endif
  va_end (args);
  free (fmt1);
  error ();
}

