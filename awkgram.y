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

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "awk.h"
#include "proto.h"
#define YY_NO_LEAKS
#define YYLEX_PARAM ii
#define YYPARSE_PARAM ii

void checkdup(Node *list, Cell *item);
void yyinit (void);
void yyend (void);
int  yylex (void *ii);
extern Interpreter *interp;

Node    *beginloc;
Node    *endloc;
Node    *winner;
int     infunc;         /* = 1 if in arglist or body of func */
int     inloop;         /* = 1 if in while, for, do */
std::string curfname;      /* current function name */
Node    *arglist;       /* list of args for current function */
char*   lexbuf;         /* buffer for lexical analyzer */
size_t  lexbuf_sz;      /* size of lex buffer */

%}

%union {
    Node    *p;
    Cell    *cp;
    int     i;
    char    *s;
}

%token   <i>    FIRSTTOKEN    /* must be first */
%token   <p>    PROGRAM PASTAT PASTAT2 XBEGIN XEND
%token   <i>    NL ',' '{' '(' '|' ';' '/' ')' '}' '[' ']'
%token   <i>    ARRAY
%token   <i>    MATCH NOTMATCH MATCHOP
%token   <i>    FINAL DOT ALL CCL NCCL CHAR OR STAR QUEST PLUS EMPTYRE
%token   <i>    AND BOR APPEND EQ GE GT LE LT NE IN
%token   <i>    ARG BLTIN BREAK CLOSE CONTINUE DELETE DO EXIT FOR FUNC 
%token   <i>    SUB GSUB IF INDEX LSUBSTR MATCHFCN NEXT NEXTFILE
%token   <i>    ADD MINUS MULT DIVIDE MOD RETURN
%token   <i>    ASSIGN ASGNOP ADDEQ SUBEQ MULTEQ DIVEQ MODEQ POWEQ
%token   <i>    PRINT PRINTF SPRINTF
%token   <p>    ELSE INTEST CONDEXPR
%token   <i>    POSTINCR PREINCR POSTDECR PREDECR
%token   <cp>   VAR IVAR VARNF CALL NUMBER STRING
%token   <s>    REGEXPR

%type    <p>    pas pattern ppattern plist pplist patlist prarg term re
%type    <p>    pa_pat pa_stat pa_stats
%type    <s>    reg_expr
%type    <p>    simple_stmt opt_simple_stmt stmt stmtlist
%type    <p>    var varname funcname varlist
%type    <p>    for if else while
%type    <i>    do st
%type    <i>    pst opt_pst lbrace rbrace rparen comma nl opt_nl and bor
%type    <i>    subop print

%right   ASGNOP
%right   '?'
%right   ':'
%left    BOR
%left    AND
%left    GETLINE
%nonassoc APPEND EQ GE GT LE LT NE MATCHOP IN '|'
%left    ARG BLTIN BREAK CALL CLOSE CONTINUE DELETE DO EXIT FOR FUNC 
%left    GSUB IF INDEX LSUBSTR MATCHFCN NEXT NUMBER
%left    PRINT PRINTF RETURN SPLIT SPRINTF STRING SUB SUBSTR
%left    REGEXPR VAR VARNF IVAR WHILE '('
%left    CAT
%left    '+' '-'
%left    '*' '/' '%'
%left    NOT UMINUS UPLUS
%right   POWER
%right   DECR INCR
%left    INDIRECT
%token   LASTTOKEN    /* must be last */

%%

program:
      pas
        { if (interp->err==0)
            winner = (Node *)stat3(PROGRAM, program, beginloc, $1, endloc);
          yyend (); }
    | error
        { yyclearin; SYNTAX("bailing out"); }
    ;

and:
      AND | and NL
    ;

bor:
      BOR | bor NL
    ;

comma:
      ',' | comma NL
    ;

do:
      DO | do NL
    ;

else:
      ELSE | else NL
    ;

for:
      FOR '(' opt_simple_stmt ';' opt_nl pattern ';' opt_nl opt_simple_stmt rparen {inloop++;} stmt
        { --inloop; $$ = stat4(FOR, forstat, $3, notnull($6), $9, $12); }
    | FOR '(' opt_simple_stmt ';'  ';' opt_nl opt_simple_stmt rparen {inloop++;} stmt
        { --inloop; $$ = stat4(FOR, forstat, $3, NIL, $7, $10); }
    | FOR '(' varname IN varname rparen {inloop++;} stmt
        { --inloop; $$ = stat3(IN, instat, $3, makearr($5), $8); }
    ;

funcname:
      VAR   { setfname($1); }
    | CALL  { setfname($1); }
    ;

if:
      IF '(' pattern rparen
        { $$ = notnull($3); }
    ;

lbrace:
      '{' | lbrace NL
    ;

nl:
      NL | nl NL
    ;

opt_nl:
      /* empty */    { $$ = 0; }
    | nl
    ;

opt_pst:
      /* empty */    { $$ = 0; }
    | pst
    ;


opt_simple_stmt:
      /* empty */    { $$ = 0; }
    | simple_stmt
    ;

pas:
      opt_pst
        { $$ = 0; }
    | opt_pst pa_stats opt_pst
        { $$ = $2; }
    ;

pa_pat:
      pattern
        { $$ = notnull($1); }
    ;

pa_stat:
      pa_pat
        { $$ = stat2(PASTAT, pastat, $1, stat2(PRINT, printstat, rectonode(), NIL)); }
    | pa_pat lbrace stmtlist '}'
        { $$ = stat2(PASTAT, pastat, $1, $3); }
    | pa_pat ',' opt_nl pa_pat
        { $$ = pa2stat($1, $4, stat2(PRINT, printstat, rectonode(), NIL)); }
    | pa_pat ',' opt_nl pa_pat lbrace stmtlist '}'
        { $$ = pa2stat($1, $4, $6); }
    | lbrace stmtlist '}'
        { $$ = stat2(PASTAT, pastat, NIL, $2); }
    | XBEGIN lbrace stmtlist '}'
        { beginloc = linkum(beginloc, $3); $$ = 0; }
    | XEND lbrace stmtlist '}'
        { endloc = linkum(endloc, $3); $$ = 0; }
    | FUNC funcname '(' varlist rparen {infunc++;} lbrace stmtlist '}'
        { infunc--; curfname.clear (); defn((Cell *)$2, $4, $8); $$ = 0; }
    ;

pa_stats:
      pa_stat
    | pa_stats opt_pst pa_stat
        { $$ = linkum($1, $3); }
    ;

patlist:
      pattern
    | patlist comma pattern
        { $$ = linkum($1, $3); }
    ;

ppattern:
      var ASGNOP ppattern
        { $$ = op2($2, assign, $1, $3, $2); }
    | ppattern '?' ppattern ':' ppattern %prec '?'
        { $$ = op3(CONDEXPR, condexpr, notnull($1), $3, $5); }
    | ppattern bor ppattern %prec BOR
        { $$ = op2(BOR, boolop, notnull($1), notnull($3), BOR); }
    | ppattern and ppattern %prec AND
        { $$ = op2(AND, boolop, notnull($1), notnull($3), AND); }
    | ppattern MATCHOP reg_expr
        { $$ = op3($2, matchop, $1, nodedfa($3, 0), NIL, $2); }
    | ppattern MATCHOP ppattern
        { if (constnode($3))
            $$ = op3($2, matchop, $1, nodedfa(strnode($3), 0), NIL, $2);
          else
            $$ = op3($2, matchop, $1, NIL, $3, $2); }
    | ppattern IN varname
        { $$ = op2(INTEST, intest, $1, makearr($3)); }
    | '(' plist ')' IN varname
        { $$ = op2(INTEST, intest, $2, makearr($5)); }
    | ppattern term %prec CAT
        { $$ = op2(CAT, cat, $1, $2); }
    | re
    | term
    ;

pattern:
      var ASGNOP pattern
        { $$ = op2($2, assign, $1, $3, $2); }
    | pattern '?' pattern ':' pattern %prec '?'
        { $$ = op3(CONDEXPR, condexpr, notnull($1), $3, $5); }
    | pattern bor pattern %prec BOR
        { $$ = op2(BOR, boolop, notnull($1), notnull($3), BOR); }
    | pattern and pattern %prec AND
        { $$ = op2(AND, boolop, notnull($1), notnull($3), AND); }
    | pattern EQ pattern
        { $$ = op2($2, relop, $1, $3, $2); }
    | pattern GE pattern
        { $$ = op2($2, relop, $1, $3, $2); }
    | pattern GT pattern
        { $$ = op2($2, relop, $1, $3, $2); }
    | pattern LE pattern
        { $$ = op2($2, relop, $1, $3, $2); }
    | pattern LT pattern
        { $$ = op2($2, relop, $1, $3, $2); }
    | pattern NE pattern
        { $$ = op2($2, relop, $1, $3, $2); }
    | pattern MATCHOP reg_expr
        { $$ = op3($2, matchop, $1, nodedfa($3, 0), NIL, $2); }
    | pattern MATCHOP pattern
        { if (constnode($3))
            $$ = op3($2, matchop, $1, nodedfa(strnode($3), 0), NIL, $2);
          else
            $$ = op3($2, matchop, $1, NIL, $3, $2); }
    | pattern IN varname
        { $$ = op2(INTEST, intest, $1, makearr($3)); }
    | '(' plist ')' IN varname
        { $$ = op2(INTEST, intest, $2, makearr($5)); }
    | pattern '|' GETLINE var
        { $$ = op2(GETLINE, awkgetline, $4, $1, $2); }
    | pattern '|' GETLINE
        { $$ = op2(GETLINE, awkgetline, (Node*)0, $1, $2); }
    | pattern term %prec CAT
        { $$ = op2(CAT, cat, $1, $2); }
    | re
    | term
    ;

plist:
      pattern comma pattern
        { $$ = linkum($1, $3); }
    | plist comma pattern
        { $$ = linkum($1, $3); }
    ;

pplist:
      ppattern
    | pplist comma ppattern
        { $$ = linkum($1, $3); }
    ;

prarg:
      /* empty */   { $$ = rectonode(); }
    | pplist
    | '(' plist ')'
        { $$ = $2; }
    ;

print:
      PRINT | PRINTF
    ;

pst:
      NL | ';' | pst NL | pst ';'
    ;

rbrace:
      '}' | rbrace NL
    ;

re:
       reg_expr
        { $$ = op3(MATCH, matchfun, rectonode(), nodedfa($1, 0), NIL); }
    | NOT re
        { $$ = op2(NOT, boolop, notnull($2), NIL, NOT); }
    ;

reg_expr:
      '/' {startreg();} REGEXPR '/'
        { $$ = $3; }
    ;

rparen:
      ')' | rparen NL
    ;

simple_stmt:
      print prarg '|' term
        { $$ = stat2($1, ($1==PRINT)?printstat : awkprintf, $2, $4, $3); }
    | print prarg APPEND term
        { $$ = stat2($1, ($1==PRINT)?printstat : awkprintf, $2, $4, $3); }
    | print prarg GT term
        { $$ = stat2($1, ($1==PRINT)?printstat : awkprintf, $2, $4, $3); }
    | print prarg
        { $$ = stat3($1, ($1==PRINT)?printstat : awkprintf, $2, NIL, NIL); }
    | DELETE varname '[' patlist ']'
        { $$ = stat2(DELETE, awkdelete, makearr($2), $4); }
    | DELETE varname
        { $$ = stat2(DELETE, awkdelete, makearr($2), 0); }
    | pattern
        { $$ = exptostat($1); }
    | error
        { yyclearin; SYNTAX("illegal statement"); }
    ;

st:
      nl
    | ';' opt_nl
    ;

stmt:
      BREAK st
        { if (!inloop)
            SYNTAX("break illegal outside of loops");
          $$ = stat1($1, jump, NIL, $1); }
    | CONTINUE st
        {  if (!inloop)
             SYNTAX("continue illegal outside of loops");
           $$ = stat1($1, jump, NIL, $1); }
    | do {inloop++;} stmt {--inloop;} WHILE '(' pattern ')' st
        { $$ = stat2(DO, dostat, $3, notnull($7)); }
    | EXIT pattern st
        { $$ = stat1($1, jump, $2, $1); }
    | EXIT st
        { $$ = stat1($1, jump, NIL, $1); }
    | for
    | if stmt else stmt
        { $$ = stat3(IF, ifstat, $1, $2, $4); }
    | if stmt
        { $$ = stat3(IF, ifstat, $1, $2, NIL); }
    | lbrace stmtlist rbrace
        { $$ = $2; }
    | NEXT st
        { if (infunc)
            SYNTAX("next is illegal inside a function");
          $$ = stat1($1, jump, NIL, $1); }
    | NEXTFILE st
        { if (infunc)
            SYNTAX("nextfile is illegal inside a function");
          $$ = stat1($1, jump, NIL, $1); }
    | RETURN pattern st
        { $$ = stat1($1, jump, $2, $1); }
    | RETURN st
        { $$ = stat1($1, jump, NIL, $1); }
    | simple_stmt st
    | while {inloop++;} stmt
        { --inloop; $$ = stat2(WHILE, whilestat, $1, $3); }
    | ';' opt_nl
        { $$ = 0; }
    ;

stmtlist:
      stmt
    | stmtlist stmt
        { $$ = linkum($1, $2); }
    ;

subop:
      SUB | GSUB
    ;

term:
       term '/' ASGNOP term
        { $$ = op2(DIVEQ, assign, $1, $4, DIVEQ); }
    | term '+' term
        { $$ = op2(ADD, arith, $1, $3, ADD); }
    | term '-' term
        { $$ = op2(MINUS, arith, $1, $3, MINUS); }
    | term '*' term
        { $$ = op2(MULT, arith, $1, $3, MULT); }
    | term '/' term
        { $$ = op2(DIVIDE, arith, $1, $3, DIVIDE); }
    | term '%' term
        { $$ = op2(MOD, arith, $1, $3, MOD); }
    | term POWER term
        { $$ = op2(POWER, arith, $1, $3, POWER); }
    | '-' term %prec UMINUS
        { $$ = op2(UMINUS, arith, $2, NIL, UMINUS); }
    | '+' term %prec UMINUS
        { $$ = op2(UPLUS, arith, $2, NIL, UPLUS); }
    | NOT term %prec UMINUS
        { $$ = op2(NOT, boolop, notnull($2), NIL, NOT); }
    | BLTIN '(' ')'
        { $$ = op1(BLTIN, bltin, rectonode(), $1); }
    | BLTIN '(' patlist ')'
        { $$ = op1(BLTIN, bltin, $3, $1); }
    | BLTIN
        { $$ = op1(BLTIN, bltin, rectonode(), $1); }
    | CALL '(' ')'
        { $$ = op2(CALL, call, celltonode($1,Cell::type::CFUNC), NIL); }
    | CALL '(' patlist ')'
        { $$ = op2(CALL, call, celltonode($1,Cell::type::CFUNC), $3); }
    | CLOSE term
        { $$ = op1(CLOSE, closefile, $2); }
    | DECR var
        { $$ = op1(PREDECR, incrdecr, $2, PREDECR); }
    | INCR var
        { $$ = op1(PREINCR, incrdecr, $2, PREINCR); }
    | var DECR
        { $$ = op1(POSTDECR, incrdecr, $1, POSTDECR); }
    | var INCR
        { $$ = op1(POSTINCR, incrdecr, $1, POSTINCR); }
    | GETLINE var LT term
        { $$ = op2(GETLINE, awkgetline, $2, $4, $3); }
    | GETLINE LT term
        { $$ = op2(GETLINE, awkgetline, NIL, $3, $2); }
    | GETLINE var
        { $$ = op3(GETLINE, awkgetline, $2, NIL, NIL); }
    | GETLINE
        { $$ = op3(GETLINE, awkgetline, NIL, NIL, NIL); }
    | INDEX '(' pattern comma pattern ')'
        { $$ = op2(INDEX, sindex, $3, $5); }
    | INDEX '(' pattern comma reg_expr ')'
        { SYNTAX("index() doesn't permit regular expressions");
          $$ = op2(INDEX, sindex, $3, (Node*)$5); }
    | '(' pattern ')'
        { $$ = $2; }
    | MATCHFCN '(' pattern comma reg_expr ')'
        { $$ = op3(MATCHFCN, matchfun, $3, nodedfa($5, 1), NIL); }
    | MATCHFCN '(' pattern comma pattern ')'
        { if (constnode($5))
            $$ = op3(MATCHFCN, matchfun, $3, nodedfa(strnode($5), 1), NIL);
          else
            $$ = op3(MATCHFCN, matchfun, $3, NIL, $5); }
    | NUMBER
        { $$ = celltonode($1); }
    | SPLIT '(' pattern comma varname comma pattern ')'     /* string */
        { $$ = op3(SPLIT, split, $3, makearr($5), $7, STRING); }
    | SPLIT '(' pattern comma varname comma reg_expr ')'    /* const /regexp/ */
        { $$ = op3(SPLIT, split, $3, makearr($5), nodedfa($7, 1), REGEXPR); }
    | SPLIT '(' pattern comma varname ')'
        { $$ = op3(SPLIT, split, $3, makearr($5), NIL, STRING); }  /* default */
    | SPRINTF '(' patlist ')'
        { $$ = op1($1, awksprintf, $3); }
    | STRING
        { $$ = celltonode($1); }
    | subop '(' reg_expr comma pattern ')'
        { $$ = op4($1, ($1==SUB)?sub:gsub, NIL, nodedfa($3, 1), $5, rectonode()); }
    | subop '(' pattern comma pattern ')'
        { if (constnode($3))
            $$ = op4($1, ($1==SUB)?sub:gsub, NIL, nodedfa(strnode($3), 1), $5, rectonode());
          else
            $$ = op4($1, ($1==SUB)?sub:gsub, $3, NIL, $5, rectonode()); }
    | subop '(' reg_expr comma pattern comma var ')'
        { $$ = op4($1, ($1==SUB)?sub:gsub, NIL, nodedfa($3, 1), $5, $7); }
    | subop '(' pattern comma pattern comma var ')'
        { if (constnode($3))
            $$ = op4($1, ($1==SUB)?sub:gsub, NIL, nodedfa(strnode($3), 1), $5, $7);
          else
            $$ = op4($1, ($1==SUB)?sub:gsub, $3, NIL, $5, $7); }
    | SUBSTR '(' pattern comma pattern comma pattern ')'
        { $$ = op3(SUBSTR, substr, $3, $5, $7); }
    | SUBSTR '(' pattern comma pattern ')'
        { $$ = op3(SUBSTR, substr, $3, $5, NIL); }
    | var
    ;

var:
      varname
    | varname '[' patlist ']'
        { $$ = op2(ARRAY, array, makearr($1), $3); }
    | IVAR
        { $$ = op1(INDIRECT, indirect, celltonode($1)); }
    | INDIRECT term
        { $$ = op1(INDIRECT, indirect, $2); }
    ;    

varlist:
      /* nothing */ { arglist = $$ = 0; }
    | VAR
        { arglist = $$ = celltonode($1); }
    | varlist comma VAR
        { checkdup($1, $3);
          arglist = $$ = linkum($1,celltonode($3)); }
    ;

varname:
      VAR
        { $$ = celltonode($1); }
    | ARG
        { $$ = op1(ARG, arg, nullnode(), $1); }
    | VARNF
        { $$ = op1(VARNF, getnf, celltonode($1)); }
    ;

while:
      WHILE '(' pattern rparen
        { $$ = notnull($3); }
    ;

%%

void setfname(Cell *p)
{
  if (p->isarr())
    SYNTAX("%s is an array, not a function", p->nval);
  else if (p->isfcn() && p->funptr)
    SYNTAX("you can't define function %s more than once", p->nval);
  curfname = p->nval;
}

int constnode(Node *p)
{
  return p->isvalue() && (p->to_cell()->flags & CONST) != 0;
}

const char *strnode(Node *p)
{
  return p->to_cell()->sval.c_str();
}

Node *notnull(Node *n)
{
  switch (n->tokid) {
    case LE: case LT: case EQ: case NE: case GT: case GE:
    case BOR: case AND: case NOT:
      return n;
    default:
      return op2(NE, relop, n, nullnode(), NE);
  }
}

void checkdup(Node *vl, Cell *cp)    /* check if name already in list */
{
  auto s = cp->nval;
  for ( ; vl; vl = vl->nnext) {
    if (s == vl->to_cell()->nval) {
      SYNTAX("duplicate argument %s", s.c_str());
      break;
    }
  }
}

void yyinit (void)
{
  beginloc = endloc = winner = 0;
  infunc = inloop = 0;
  curfname.clear ();
  arglist = 0;
  lexbuf_sz = 256;
  lexbuf = new char[lexbuf_sz];
}

void yyend (void)
{
  delete lexbuf;
}