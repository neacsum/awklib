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
#pragma once
#include <assert.h>
#include <stdio.h>

typedef double  Awkfloat;

/* unsigned char is more trouble than it's worth */

//typedef  unsigned char uschar;

#define  xfree(a)  { if ((a) != NULL) { free((void *) (a)); (a) = NULL; } }
#define  NN(p)  ((p) ? (p) : "(null)")  /* guaranteed non-null for dprintf 
*/

int errprintf (const char *fmt, ...);
#ifdef  NDEBUG
/* errprintf should be optimized out of existence */
# define  dprintf 1? 0 : errprintf 
#else
extern int  dbg;
#define  dprintf  if (dbg) errprintf
#define YYDBGOUT if (dbg>2) errprintf
#endif

#define  RECSIZE  (8 * 1024)  /* sets limit on records, fields, etc., etc. */

extern int  lineno;    /* line number in awk program */
extern int  errorflag;  /* 1 if error has occurred */

extern  char  *patbeg;  /* beginning of pattern matched */
extern  size_t  patlen;    /* length of pattern matched.  set in b.c */

struct Array;


/// Cell:  all information about a variable or constant
struct Cell {
  enum class type {OCELL=1, OBOOL=2, OJUMP=3};  //cell type
  enum class subtype {CARG=6, CCON=5, CTEMP=4, CVAR=3, CREC=2, CFLD=1, CNONE=0,
                BTRUE=11, BFALSE=12,
                JEXIT=21, JNEXT=22, JBREAK=23, JCONT=24, JRET=25, JNEXTFILE=26};
  Cell (type t, subtype s, const char *n, void *v, Awkfloat f, unsigned char flags);
  ~Cell ();
  void setsval (const char* s);
  void setfval (Awkfloat f);
  const char* getsval ();
  Awkfloat getfval ();
  const char* getpssval ();
  void update_str_val ();

  type ctype;    /* Cell type, see below */

  subtype csub;    /* Cell subtype, see below */

  char  *nval;        /* name, for variables only */
  union {
    char* sval;        /* string value */
    Array* arrval;     /* reuse for array pointer */
  };
  Awkfloat fval;      /* value as number */
  unsigned char   tval;   /* type flags: STR|NUM|ARR|FCN|CONVC */
#define NUM       0x001   /* number value is valid */
#define STR       0x002   /* string value is valid */
#define ARR       0x004   /* this is an array */
#define FCN       0x008   /* this is a function name */
#define CONVC     0x010   /* string was converted from number via CONVFMT */
#define EXTFUN    0x020   /* external function */

  char  *fmt;         /* CONVFMT/OFMT value used to convert from number */
  Cell *cnext; /* ptr to next in arrays*/

  bool isarr () const { return (tval & ARR) != 0; }
  bool isfcn () const { return (tval & (FCN | EXTFUN)) != 0; }
  bool isfld () const { return csub == subtype::CFLD; }
  bool isrec () const { return csub == subtype::CREC; }
  bool isnext () const  { return csub == subtype::JNEXT || csub == subtype::JNEXTFILE; }
  bool isret () const { return csub == subtype::JRET; }
  bool isbreak () const { return csub == subtype::JBREAK; }
  bool iscont () const { return csub == subtype::JCONT; }
  bool isjump () const { return ctype == type::OJUMP; }
  bool isexit () const { return csub == subtype::JEXIT; }
  bool istrue () const { return csub == subtype::BTRUE; }
};

struct Array {    /* symbol table array */
  Array (int n);
  ~Array ();
  Cell* setsym (const char* n, const char* s, double f, unsigned int t);
  Cell* removesym (const char* n);
  Cell* lookup (const char* name);
  void rehash ();

  int  nelem;     /* elements in table right now */
  int  size;      /* size of tab */
  Cell  **tab;    /* hash table pointers */
};

#define  NSYMTAB  50  /* initial size of a symbol table */

/* function types */
#define FLENGTH   1
#define FSQRT     2
#define FEXP      3
#define FLOG      4
#define FINT      5
#define FSYSTEM   6
#define FRAND     7
#define FSRAND    8
#define FSIN      9
#define FCOS      10
#define FATAN     11
#define FTOUPPER  12
#define FTOLOWER  13
#define FFLUSH    14

/* Node:  parse tree is made of nodes, with Cell's at bottom */

typedef struct Node {
  int  ntype;   // node type, see below
#define NVALUE  1         //value node -  has only one argument that is a Cell
#define NSTAT   2         //statement node
#define NEXPR   3
  Cell* (*proc) (struct Node** a, int tok);
  struct  Node *nnext;
  int  lineno;
  int  nobj;              ///<token id
  int  args;              ///< number of arguments
  struct  Node *narg[1];  ///< variable: actual size set by calling malloc
} Node;

#define  NIL  ((Node *) 0)

extern Node  *nullnode;

extern  int  pairstack[], paircnt;

inline bool isvalue (const Node* n) { return n->ntype == NVALUE; }

/* structures used by regular expression matching machinery, mostly b.c: */

#define NCHARS  (256+3)    /* 256 handles 8-bit chars; 128 does 7-bit */
        /* watch out in match(), etc. */
#define NSTATES  32

typedef struct rrow {
  long  ltype;  /* long avoids pointer warnings on 64-bit */
  union {
    int i;
    Node *np;
    unsigned char *up;
  } lval;    /* because Al stores a pointer in it! */
  int  *lfollow;
} rrow;

typedef struct fa {
  unsigned char  gototab[NSTATES][NCHARS];
  unsigned char  out[NSTATES];
  unsigned char  *restr;
  int  *posns[NSTATES];
  int  anchor;
  int  use;
  int  initstat;
  int  curstat;
  int  accept;
  int  reset;
  struct  rrow re[1];  /* variable: actual size set by calling malloc */
} fa;

struct FILE_STRUC {
  FILE  *fp;
  char  *fname;
  int  mode;  /* '|', 'a', 'w' => LE/LT, GT */
};

typedef int (*inproc)();
typedef int (*outproc)(const char *buf, size_t len);

// Function call frame
struct Frame {
  Cell *fcn;    //the function
  Cell **args;  //arguments
  int nargs;    //number of arguments
  Cell *ret;    // return value
};

struct Interpreter {
  Interpreter ();
  ~Interpreter ();
  void syminit ();
  void envinit ();
  void makefields (int nf);

  void std_redirect (int nf, const char* fname);
  void run ();
  void clean_symtab ();
  void closeall ();
  void initgetrec ();
  int getrec (Cell* cell);
  void nextfile ();
  int getchar (FILE* inf);
  int readrec (Cell* cell, FILE* inf);
  const char* getargv (int n);
  int putstr (const char* str, FILE* fp);
  void setclvar (const char* s);
  void fldbld ();
  int refldbld (const char* rec, const char* fs);
  void cleanfld (int n1, int n2);
  void newfld (int n);
  void setlastfld (int n);
  Cell* fieldadr (int n);
  void growfldtab (size_t n);

  int status;           ///< Interpreter status. See below
#define AWKS_INIT       1   ///< status block initialized
#define AWKS_COMPILING  2   ///< compilation started
#define AWKS_COMPILED   3   ///< Program compiled
#define AWKS_RUN        4   ///< Program running
#define AWKS_DONE       5   ///< Program finished 

  int err;              ///< Last error or warning
  bool first_run;       ///< true on first run after compile
  Array *symtab;        ///< symbol table
  Node *prog_root;      ///< root of parsing tree
  int argno;            ///< current input argument number */
  Array *envir;         ///< environment variables
  Array* argvtab;       ///< ARGV[n] array
  Awkfloat srand_seed;  ///< seed for random number generator
  char *lexprog;        ///< AWK script
  char *lexptr;         ///< pointer in AWK script
  int nprog;            ///< number of programs
  int curprog;          ///< current program being compiled
  char **progs;         ///< array of program filenames
  struct FILE_STRUC *files;    ///< opened files
                               ///< 0 = stdin
                               ///< 1 = stdout
                               ///< 2 = stderr
  int nfiles;           ///< number of entries in files table
  FILE* infile;         ///< current input file
  inproc inredir;       ///< input redirection function
  outproc outredir;     ///< output redirection function
  struct Frame  fn;     ///< frame data for current function call
  bool donerec;         ///< true if record broken into fields
  bool donefld;         ///< true if record is valid (no fld has changed)
  Cell *fldtab;        ///< $0, $1, ...
  int nfields;          ///< last allocated field in fldtab
  int lastfld;          ///< last used field
#define CELL_FS         predefs[0]
#define CELL_RS         predefs[1]
#define CELL_OFS        predefs[2]
#define CELL_ORS        predefs[3]
#define CELL_OFMT       predefs[4]
#define CELL_CONVFMT    predefs[5]
#define CELL_NF         predefs[6]
#define CELL_FILENAME   predefs[7]
#define CELL_NR         predefs[8]
#define CELL_FNR        predefs[9]
#define CELL_SUBSEP     predefs[10]
#define CELL_RSTART     predefs[11]
#define CELL_RLENGTH    predefs[12]
#define CELL_ARGC       predefs[13]

#define NPREDEF 14
  Cell* predefs[NPREDEF];  ///< Predefined variables

  //TODO remove next line when finished converting to OO
  void* interp; //only to highlight inconsistent use.
};

#define FS        (interp->CELL_FS->sval)
#define RS        (interp->CELL_RS->sval)
#define OFS       (interp->CELL_OFS->sval)
#define ORS       (interp->CELL_ORS->sval)
#define OFMT      (interp->CELL_OFMT->sval)
#define CONVFMT   (interp->CELL_CONVFMT->sval)
#define NF        (interp->CELL_NF->fval)
#define FILENAME  (interp->CELL_FILENAME->sval)
#define NR        (interp->CELL_NR->fval)
#define FNR       (interp->CELL_FNR->fval)
#define SUBSEP    (interp->CELL_SUBSEP->sval)
#define RSTART    (interp->CELL_RSTART->fval)
#define RLENGTH   (interp->CELL_RLENGTH->fval)
#define ARGC      (interp->CELL_ARGC->fval)

#define MY_FS       (CELL_FS->sval)
#define MY_RS       (CELL_RS->sval)
#define MY_OFS      (CELL_OFS->sval)
#define MY_ORS      (CELL_ORS->sval)
#define MY_OFMT     (CELL_OFMT->sval)
#define MY_CONVFMT  (CELL_CONVFMT->sval)
#define MY_NF       (CELL_NF->fval)
#define MY_FILENAME (CELL_FILENAME->sval)
#define MY_NR       (CELL_NR->fval)
#define MY_FNR      (CELL_FNR->fval)
#define MY_SUBSEP   (CELL_SUBSEP->sval)
#define MY_RSTART   (CELL_RSTART->fval)
#define MY_RLENGTH  (CELL_RLENGTH->fval)
#define MY_ARGC     (CELL_ARGC->fval)

extern Interpreter *interp;

#include "proto.h"
