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
#define  dprintf  (dbg==0)? 0 : errprintf
#define YYDBGOUT  (dbg<=2)? 0 : errprintf
#endif

#define  RECSIZE  (8 * 1024)  /* sets limit on records, fields, etc., etc. */

extern  char  *patbeg;  /* beginning of pattern matched */
extern  size_t  patlen;    /* length of pattern matched.  set in b.c */

class Array;


/// Cell:  all information about a variable or constant
struct Cell {
  enum class type {OCELL=1, OBOOL=2, OJUMP=3};  //cell type
  enum class subtype {
    // OCELL subtypes
    CARR=8,   //array
    CFUNC=7,  //function definition
    CARG=6,   //function argument
    CCON=5,   //constant 
    CTEMP=4,  //temporary
    CVAR=3,   //variable 
    CREC=2,   //input record ($0)
    CFLD=1,   //input field ($1, $2,...)
    CNONE=0,  //not specified

    //OBOOL subtypes
    BTRUE=11, //true value
    BFALSE=12, //false value

    //OJUMP subtypes
    JEXIT=21,   //
    JNEXT=22, JBREAK=23, JCONT=24, JRET=25, JNEXTFILE=26};
  Cell (type t, subtype s, const char *n, void *v, Awkfloat f, unsigned char flags);
  ~Cell ();
  void setsval (const char* s);
  void setfval (Awkfloat f);
  const char* getsval ();
  Awkfloat getfval ();
  const char* getpssval ();
  void update_str_val ();

  type ctype;    /* Cell type, see above */

  subtype csub;    /* Cell subtype, see above */

  char  *nval;        /* name, for variables only */
  union {
    char* sval;        /* string value */
    Array* arrval;     /* reuse for array pointer */
  };
  Awkfloat fval;      /* value as number */
  unsigned char   flags;   /* type flags */
#define NUM       0x01   /* number value is valid */
#define STR       0x02   /* string value is valid */
#define CONVC     0x04   /* string was converted from number via CONVFMT */
#define PREDEF    0x08   /* predefined variable*/
#define EXTFUNC   0x10   /* external function*/

  char  *fmt;         /* CONVFMT/OFMT value used to convert from number */
  Cell *cnext; /* ptr to next in arrays*/

  bool isarr () const { return csub == subtype::CARR; }
  bool isfcn () const { return csub == subtype::CFUNC; }
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

class Array {    /* symbol table array */
public:
  class Iterator {
  public:
    Iterator& operator ++(int);
    Cell* operator *();
    Cell* operator ->() { return operator *(); };
    bool operator == (const Iterator& other) const;
    bool operator != (const Iterator& other) const { return !operator== (other); }

  private:
    Iterator (const Array* a);
    const Array* owner;
    int ipos;
    Cell* ptr;
    friend class Array;
  };

  Array (int n);
  ~Array ();
  Cell* setsym (const char* n, const char* s, double f, unsigned int t, Cell::subtype sub=Cell::subtype::CNONE);
  Cell* setsym (const char* n, Array* arr, unsigned int t);
  Cell* removesym (const char* n);
  void deletecell (Iterator& p);
  Cell* lookup (const char* name);
  int   length () const;
  int   size () const;

  Iterator begin () const;
  Iterator end () const;

private:
  void rehash ();
  Cell* insert_sym (const char* n);

  int  nelem;     /* elements in table right now */
  int  sz;        /* size of tab */
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
  void recbld ();
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
  char errmsg[1024];    ///< Last error message
  bool first_run;       ///< true on first run after compile
  int lineno;           ///< line number in awk program
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

struct awk_exception {
  awk_exception (Interpreter &interp, int code, const char* msg);
  Interpreter& interp;
  int err;
};

inline
int Array::length () const
{
  return nelem;
}

inline
int Array::size () const
{
  return sz;
}
#include "proto.h"
