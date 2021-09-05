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
const char* strnode (Node *);
Node* notnull (Node *);
int   yylex (void*);
int   yyparse (void*);
void  yyinit (void);
void  startreg (void);

Node* nodedfa (const char* str);

int   pgetc (void);
char* cursource (void);

Node* exptostat (Node *);
Node* stat1 (int tok, pfun fn, Node* a0, int iarg = 0);
Node* stat2 (int tok, pfun fn, Node* a0, Node* a1, int iarg = 0);
Node* stat3 (int tok, pfun fn, Node* a0, Node* a1, Node* a2, int iarg = 0);
Node* stat4 (int tok, pfun fn, Node* a0, Node* a1, Node* a2, Node* a3, int iarg = 0);
Node* op1 (int tok, pfun fn, Node *, int iarg = 0);
Node* op2 (int tok, pfun fn, Node *, Node *, int iarg = 0);
Node* op3 (int tok, pfun fn, Node *, Node *, Node *, int iarg = 0);
Node* op4 (int tok, pfun fn, Node *, Node *, Node *, Node *, int iarg = 0);
Node* celltonode (Cell *c, Cell::type t = Cell::type::CELL, int flags = 0);
Node* rectonode ();
Node* nullnode ();
Node* makearr (Node *);
Node* pa2stat (Node *, Node *, Node *);
Node* linkum (Node *, Node *);
void  defn (Cell *, Node *, Node *);
int   isarg (const char *);
void  funnyvar (const Cell *, const char *);
char* tostring (const char *);
char* qstring (const char *str, int delim);

void  yyerror (const char *);
void  fpecatch (int);
int   input (void);
void  WARNING (const char *, ...);
double errcheck (double, const char *);
int  isclvar (const char *);
bool  is_number (const char *);
inline
bool is_number (const std::string& c)
{
  return is_number (c.c_str ());
}

void  adjbuf (char **pb, size_t *sz, size_t min, int q, char **pbp);
void tempfree (Cell *a);

Cell* program (const Node::Arguments&, int);
Cell* call (const Node::Arguments&, int);
Cell* arg (const Node::Arguments&, int);
Cell* jump (const Node::Arguments&, int);
Cell* awkgetline (const Node::Arguments&, int);
Cell* getnf (const Node::Arguments&, int);
Cell* array (const Node::Arguments&, int);
Cell* awkdelete (const Node::Arguments&, int);
Cell* intest (const Node::Arguments&, int);
Cell* matchop (const Node::Arguments&, int);
Cell* matchfun (const Node::Arguments& a, int);
Cell* boolop (const Node::Arguments&, int);
Cell* relop (const Node::Arguments&, int);
Cell* indirect (const Node::Arguments&, int);
Cell* substr (const Node::Arguments&, int);
Cell* sindex (const Node::Arguments&, int);
Cell* awksprintf (const Node::Arguments&, int);
Cell* awkprintf (const Node::Arguments&, int);
Cell* arith (const Node::Arguments&, int);
Cell* incrdecr (const Node::Arguments&, int);
Cell* assign (const Node::Arguments&, int);
Cell* cat (const Node::Arguments&, int);
Cell* pastat (const Node::Arguments&, int);
Cell* dopa2 (const Node::Arguments&, int);
Cell* split (const Node::Arguments&, int);
Cell* condexpr (const Node::Arguments&, int);
Cell* ifstat (const Node::Arguments&, int);
Cell* whilestat (const Node::Arguments&, int);
Cell* dostat (const Node::Arguments&, int);
Cell* forstat (const Node::Arguments&, int);
Cell* instat (const Node::Arguments&, int);
Cell* bltin (const Node::Arguments&, int);
Cell* printstat (const Node::Arguments&, int);
Cell* nullproc (const Node::Arguments&, int);
FILE* redirect (int, Node& );
FILE* openfile (int, const char *);
const char* filename (FILE *);
Cell* closefile (const Node::Arguments&, int);
Cell* sub (const Node::Arguments&, int);
Cell* gsub (const Node::Arguments&, int);
void requote (std::string& s);

// TODO: make it member of Node
Cell* execute (const Node* u);
inline
Cell* execute (const std::unique_ptr<Node>& u)
{
  return execute (u.get ());
}

FILE* popen (const char *, const char *);
int   pclose (FILE *);

void  SYNTAX (const char*, ...);
void  FATAL (int err, const char*, ...);

#ifndef NDEBUG
void print_cell (Cell* c, int indent);
const char *flags2str (int flags);
std::string quote (const std::string& in);
#else
inline void print_cell (Cell* c, int indent) {};
inline const char* flags2str (int flags) { return ""; };
inline std::string quote (const std::string& in) {
  return in;
};
#endif
