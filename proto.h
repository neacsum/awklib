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

void  setfname (Cell *);
int   constnode (Node *);
char* strnode (Node *);
Node* notnull (Node *);
int   yylex (void);
int   yyparse (void);
void  yyinit (void);
void  startreg (void);

fa*   makedfa (const char *, int);
int   match (fa *, const char *);
int   pmatch (fa *, const char *);
int   nematch (fa *, const char *);
void  freefa (fa *);

int   pgetc (void);
char* cursource (void);

Node* exptostat (Node *);
Node* stat3 (int, Node *, Node *, Node *);
Node* op2 (int, Node *, Node *);
Node* op1 (int, Node *);
Node* stat1 (int, Node *);
Node* op3 (int, Node *, Node *, Node *);
Node* op4 (int, Node *, Node *, Node *, Node *);
Node* stat2 (int, Node *, Node *);
Node* stat4 (int, Node *, Node *, Node *, Node *);
Node* celltonode (Cell *c, int csub);
Node* rectonode (void);
Node* makearr (Node *);
Node* pa2stat (Node *, Node *, Node *);
Node* linkum (Node *, Node *);
void  defn (Cell *, Node *, Node *);
int   isarg (const char *);
int   ptoi (void *);
Node* itonp (int);
void  freenode (Node* n);
void  syminit (void);
void  arginit (void);
void  envinit (void);
Array*  makearray (int);
void  freearray (Array *);
void freecell (Cell *);
void  freeelem (Cell *, const char *);
Cell* setsymtab (const char *name, const char *sval, double nval, unsigned int type, Array *tab);
Cell* lookup (const char *name, Array *tab);
void setfval (Cell *, Awkfloat);
void  funnyvar (Cell *, const char *);
void setsval (Cell *, const char *);
double getfval (Cell *);
const char* getsval (Cell *);
const char* getpssval (Cell *);     /* for print */
char* tostring (const char *);
char* qstring (const char *str, int delim);

void  recinit (void);
void  makefields (int, int);
void  freefields (void);
void  growfldtab (size_t n);
int   getrec (Cell *c);
int   awkputs (const char *str, FILE *fp);

void  nextfile (void);
int   readrec (Cell* c, FILE *inf);
const char* getargv (int);
void  setclvar (const char *);
void  fldbld (void);
void  cleanfld (int, int);
void  newfld (int);
void  setlastfld (int);
void  recbld (void);
Cell* fieldadr (int);
void  yyerror (const char *);
void  fpecatch (int);
int   input (void);
void  WARNING (const char *, ...);
double errcheck (double, const char *);
int  isclvar (const char *);
int  is_number (const char *);

int  adjbuf (char **pb, int *sz, int min, int q, char **pbp);
void  run (Node *);
void  stdinit (void);
void initgetrec (void);
void tempfree (Cell *a);

Cell* program (Node **, int);
Cell* call (Node **, int);
Cell* arg (Node **, int);
Cell* jump (Node **, int);
Cell* awkgetline (Node **, int);
Cell* getnf (Node **, int);
Cell* array (Node **, int);
Cell* awkdelete (Node **, int);
Cell* intest (Node **, int);
Cell* matchop (Node **, int);
Cell* boolop (Node **, int);
Cell* relop (Node **, int);
Cell* indirect (Node **, int);
Cell* substr (Node **, int);
Cell* sindex (Node **, int);
Cell* awksprintf (Node **, int);
Cell* awkprintf (Node **, int);
Cell* arith (Node **, int);
Cell* incrdecr (Node **, int);
Cell* assign (Node **, int);
Cell* cat (Node **, int);
Cell* pastat (Node **, int);
Cell* dopa2 (Node **, int);
Cell* split (Node **, int);
Cell* condexpr (Node **, int);
Cell* ifstat (Node **, int);
Cell* whilestat (Node **, int);
Cell* dostat (Node **, int);
Cell* forstat (Node **, int);
Cell* instat (Node **, int);
Cell* bltin (Node **, int);
Cell* printstat (Node **, int);
Cell* nullproc (Node **, int);
FILE* redirect (int, Node *);
FILE* openfile (int, const char *);
const char* filename (FILE *);
Cell* closefile (Node **, int);
Cell* sub (Node **, int);
Cell* gsub (Node **, int);

FILE* popen (const char *, const char *);
int   pclose (FILE *);

void  SYNTAX (const char *, ...);
void  FATAL (int err, const char *, ...);

const char *flags2str (int flags);
const char *quote (const char *in);
