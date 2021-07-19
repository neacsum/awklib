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

typedef Cell* (*pfun)(Node** a, int);

Node* exptostat (Node *);
Node* stat1 (int tok, pfun fn, Node *a0);
Node* stat2 (int tok, pfun fn, Node *a0, Node *a1);
Node* stat3 (int tok, pfun fn, Node *a0, Node *a1, Node *a2);
Node* stat4 (int tok, pfun fn, Node *a0, Node *a1, Node *a2, Node *a3);
Node* op1 (int tok, pfun fn, Node *);
Node* op2 (int tok, pfun fn, Node *, Node *);
Node* op3 (int tok, pfun fn, Node *, Node *, Node *);
Node* op4 (int tok, pfun fn, Node *, Node *, Node *, Node *);
Node* celltonode (Cell *c, Cell::subtype csub);
Node* rectonode (void);
Node* makearr (Node *);
Node* pa2stat (Node *, Node *, Node *);
Node* linkum (Node *, Node *);
void  defn (Cell *, Node *, Node *);
int   isarg (const char *);
int   ptoi (void *);
Node* itonp (int);
void  freenode (Node* n);
void  freeelem (Cell *, const char *);
void  funnyvar (Cell *, const char *);
char* tostring (const char *);
char* qstring (const char *str, int delim);

void  recbld (void);
void  yyerror (const char *);
void  fpecatch (int);
int   input (void);
void  WARNING (const char *, ...);
double errcheck (double, const char *);
int  isclvar (const char *);
int  is_number (const char *);

void  adjbuf (char **pb, size_t *sz, size_t min, int q, char **pbp);
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
Cell* execute (Node* u);

FILE* popen (const char *, const char *);
int   pclose (FILE *);

void  SYNTAX (const char *, ...);
void  FATAL (int err, const char *, ...);

const char *flags2str (int flags);
const char *quote (const char *in);
