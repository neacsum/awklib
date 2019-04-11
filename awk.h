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

typedef double  Awkfloat;

/* unsigned char is more trouble than it's worth */

typedef  unsigned char uschar;

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
#define YYDBGOUT if (dbg>1) errprintf
#endif

#define  RECSIZE  (8 * 1024)  /* sets limit on records, fields, etc., etc. */
extern int  recsize;  /* size of current record, orig RECSIZE */

extern char  **FS;
extern char  **RS;
extern char  **ORS;
extern char  **OFS;
extern char  **OFMT;
extern Awkfloat *NR;
extern Awkfloat *FNR;
extern Awkfloat *NF;
extern const char** FILENAME;
extern char  **SUBSEP;
extern Awkfloat *RSTART;
extern Awkfloat *RLENGTH;

extern int  lineno;    /* line number in awk program */
extern int  errorflag;  /* 1 if error has occurred */
extern int  donefld;  /* 1 if record broken into fields */
extern int  donerec;  /* 1 if record is valid (no fld has changed */
extern char  inputFS[];  /* FS at time of input, for field splitting */

extern  char  *patbeg;  /* beginning of pattern matched */
extern  int  patlen;    /* length of pattern matched.  set in b.c */

/* Cell:  all information about a variable or constant */

typedef struct Cell {
  uschar  ctype;    /* Cell type, see below */
#define OCELL   1
#define OBOOL   2
#define OJUMP   3

  uschar  csub;    /* Cell subtype, see below */
#define CARG    6     //Cell is function call argument
#define CCON    5     //Cell is constant (number or string)
#define CTEMP   4
#define CVAR    2
#define CFLD    1

/* bool subtypes */
#define BTRUE   11
#define BFALSE  12

/* jump subtypes */
#define JEXIT   21
#define JNEXT   22
#define JBREAK  23
#define JCONT   24
#define JRET    25
#define JNEXTFILE  26

  char  *nval;        /* name, for variables only */
  char  *sval;        /* string value */
  Awkfloat fval;      /* value as number */
  int   tval;         /* type info: STR|NUM|ARR|FCN|FLD|CON|DONTFREE|CONVC|CONVO */
#define NUM       0x001   /* number value is valid */
#define STR       0x002   /* string value is valid */
#define ARR       0x004   /* this is an array */
#define FCN       0x008   /* this is a function name */
#define FLD       0x010   /* this is a field $1, $2, ... */
#define REC       0x020   /* this is $0 */
#define CONVC     0x040   /* string was converted from number via CONVFMT */
#define EXTFUN    0x080   /* external function */

  char  *fmt;         /* CONVFMT/OFMT value used to convert from number */
  struct Cell *cnext; /* ptr to next in arrays*/
} Cell;

typedef struct Array {    /* symbol table array */
  int  nelem;     /* elements in table right now */
  int  size;      /* size of tab */
  Cell  **tab;    /* hash table pointers */
} Array;

#define  NSYMTAB  50  /* initial size of a symbol table */
extern Array  *symtab;

extern Cell*    nrloc;      /* NR */
extern Cell*    fnrloc;     /* FNR */
extern Cell*    nfloc;      /* NF */
extern Cell*    rstartloc;  /* RSTART */
extern Cell*    rlengthloc; /* RLENGTH */
extern Cell**   fldtab; /* $0, $1, ...*/


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
  struct  Node *nnext;
  int  lineno;
  int  nobj;              ///<token id
  int  args;              ///< number of arguments
  struct  Node *narg[1];  ///< variable: actual size set by calling malloc
} Node;

#define  NIL  ((Node *) 0)

extern Node  *nullnode;

extern  int  pairstack[], paircnt;

inline int isvalue (const Node* n) { return n->ntype == NVALUE; }
inline int isrec (const Cell* c) { return (c->tval & REC) != 0; }
inline int isfld (const Cell *c) {return (c->tval & FLD) != 0;}
inline int isstr (const Cell *c) { return (c->tval & STR) != 0; }
inline int isnum (const Cell* c) { return (c->tval & NUM) != 0; }
inline int isarr (const Cell *c) { return (c->tval & ARR) != 0; }
inline int isfcn (const Cell *c) { return (c->tval & (FCN | EXTFUN)) != 0; }

/* structures used by regular expression matching machinery, mostly b.c: */

#define NCHARS  (256+3)    /* 256 handles 8-bit chars; 128 does 7-bit */
        /* watch out in match(), etc. */
#define NSTATES  32

typedef struct rrow {
  long  ltype;  /* long avoids pointer warnings on 64-bit */
  union {
    int i;
    Node *np;
    uschar *up;
  } lval;    /* because Al stores a pointer in it! */
  int  *lfollow;
} rrow;

typedef struct fa {
  uschar  gototab[NSTATES][NCHARS];
  uschar  out[NSTATES];
  uschar  *restr;
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

typedef struct AWKINTERP {
  int status;           ///< Interpreter status. See below
#define AWKS_INIT       1   ///< status block initialized
#define AWKS_COMPILING  2   ///< compilation started
#define AWKS_COMPILED   3   ///< Program compiled
#define AWKS_RUN        4   ///< Program running
#define AWKS_END        5   ///< Program finished 
#define AWKS_FATAL      -1  ///< fatal error occurred
  int err;              ///< Last error or warning
  Array *symtab;        ///< symbol table
  Node *prog_root;      ///< root of parsing tree
  int argc;             ///< number of arguments
  char **argv;          ///< arguments array
  int argno;            ///< current input argument number */
  Array *envir;         ///< environment variables
  Awkfloat srand_seed;  ///< seed for random number generator
  char *lexprog;        ///< AWK script
  char *lexptr;         ///< pointer in AWK script
  int nprog;            ///< number of programs
  int curprog;          ///< current program being compiled
  char **progs;         ///< array of program filenames
  struct FILE_STRUC *files;    ///< opened files
  inproc inredir;       ///< input redirection function
  outproc outredir;     ///< output redirection function
  struct Frame  fn;     ///< frame data for current function call
} AWKINTERP;

extern AWKINTERP *interp;

#include "proto.h"
