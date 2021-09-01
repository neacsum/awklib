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
#include <vector>
#include <memory>
#include <random>
#include <regex>

typedef double  Awkfloat;

#define  xfree(a)  { if ((a) != NULL) { free((void *) (a)); (a) = NULL; } }
#define  NN(p)  ((p) ? (p) : "(null)")  /* guaranteed non-null for dprintf */

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
#define  NSYMTAB  50  /* initial size of a symbol table */


class Array;
class Node;

/// Cell:  all information about a variable or constant
class Cell {
public:
  enum class type {
    CELL,
    CTEMP,      // temporary
    CFUNC,      //function definition
    EXTFUNC,    // external function
    FLD,        //input field ($1, $2,...)
    REC,        //input record ($0)
    CNF,        //NF
    //JUMP types
    JUMP_FIRST,
      JEXIT, JNEXT, JBREAK, JCONT, JRET, JNEXTFILE,
    JUMP_LAST,

    BTRUE, //true value
    BFALSE //false value
  };

  Cell (const char *n=nullptr, type t=Cell::type::CELL, unsigned char flags=0, Awkfloat f = 0.);
  ~Cell ();
  Cell& operator =(const Cell& rhs);
  void setsval (const char* s);
  void setfval (Awkfloat f);
  const char* getsval ();
  Awkfloat getfval ();
  const char* getpssval ();
  void clear ();
  void makearray (size_t sz = NSYMTAB);
  bool match (const char* s, bool anchored=false);
  bool pmatch (const char* s, size_t& start, size_t& len, bool anchored = false);

  type ctype;           /* Cell type, see above */
  unsigned char   flags;/* type flags */
#define NUM       0x01  /* number value is valid */
#define STR       0x02  /* string value is valid */
#define CONVC     0x04  /* string was converted from number via CONVFMT */
#define PREDEF    0x08  /* predefined variable*/
#define ARR       0x10
#define REGEX     0x20
#define CONST     0x80

  std::string nval;     /* name */
  std::string sval;     /* string value */
  Awkfloat fval;        /* value as number */
  union {
    void* funptr;       /* pointer to external function body */
    Array* arrval;      /* reuse for array pointer */
    Node* nodeptr;      /* reuse for function pointer */
    std::regex* re;     /* reuse for regex pointer*/
  };

  Cell *cnext;           /* ptr to next in arrays*/
#ifndef NDEBUG
  int id;
#endif

  bool isarr () const { return (flags & ARR) != 0; }
  bool isfcn () const { return (ctype == type::CFUNC) || (ctype == type::EXTFUNC); }
  bool isfld () const { return ctype == type::FLD; }
  bool isnf () const { return ctype == type::CNF; }

  bool isregex () const { return (flags & REGEX) != 0; }
  bool isrec () const { return ctype == type::REC; }
  bool isnext () const  { return ctype == type::JNEXT || ctype == type::JNEXTFILE; }
  bool isret () const { return ctype == type::JRET; }
  bool isbreak () const { return ctype == type::JBREAK; }
  bool iscont () const { return ctype == type::JCONT; }
  bool isjump () const { return ctype > type::JUMP_FIRST && ctype < type::JUMP_LAST; }
  bool isexit () const { return ctype== type::JEXIT; }
  bool istrue () const { return ctype == type::BTRUE; }
  bool istemp () const { return ctype == type::CTEMP; }

private:
  std::string fmt;       /* CONVFMT/OFMT value used to convert from number */
  void update_str_val ();
};

class Array {    /* symbol table array */
public:
  class Iterator {
  public:
    Iterator operator ++(int);
    Iterator& operator ++();
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
  Cell* setsym (const char* n, const char* s, double f, unsigned int t);
  Cell* setsym (const char* n, Array* arr, int addl_flags);
  Cell* removesym (const std::string& n);
  void deletecell (Iterator& p);
  Cell* lookup (const char* name);
  int   length () const;
  int   size () const;

  Iterator begin () const;
  Iterator end () const;
#ifndef NDEBUG
  void print ();
#endif

private:
  void rehash ();
  Cell* insert_sym (const char* n);

  int  nelem;     /* elements in table right now */
  int  sz;        /* size of tab */
  Cell  **tab;    /* hash table pointers */
};

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
class Node
{
public:
  typedef std::vector< std::unique_ptr<Node> > Arguments;
  typedef Cell* (*pfun)(const Node::Arguments& a, int);

  Node ();
  Node (int tokid, pfun fn, int iarg, Node* arg1);
  Node (int tokid, pfun fn, int iarg, Node* arg1, Node* arg2);
  Node (int tokid, pfun fn, int iarg, Node* arg1, Node* arg2, Node* arg3);
  Node (int tokid, pfun fn, int iarg, Node* arg1, Node* arg2, Node* arg3, Node* arg4);
  ~Node ();

  bool isvalue () const;
  Cell* to_cell () const;
  int  ntype;             // node type, see below
#define NVALUE  1         //value node -  has only one argument that is a Cell
#define NSTAT   2         //statement node
#define NEXPR   3
  pfun proc;
  Node *nnext;
  int  lineno;
  int  tokid;             //!< token id
  Arguments arg;          //!< array of node arguments
  int iarg;               //!< int argument
#ifndef NDEBUG
  int id;                 //
#endif
};

typedef Node::pfun pfun;

#define  NIL  ((Node *) 0)

extern  int  pairstack[], paircnt;

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
};


class Interpreter {
public:
  Interpreter ();
  ~Interpreter ();
  void syminit ();
  void envinit ();
  void makefields (size_t nf);

  void std_redirect (int nf, const char* fname);
  void run ();
  void clean_symtab ();
  void closeall ();
  void initgetrec ();
  bool getrec (Cell* cell);
  void nextfile ();
  int getchar (FILE* inf);
  bool readrec (Cell* cell, FILE* inf);
  const char* getargv (int n);
  int putstr (const char* str, FILE* fp);
  void setclvar (const char* s);
  void fldbld ();
  void recbld ();
  void cleanfld (int n1, int n2);
  void setlastfld (int n);
  Cell* fieldadr (int n);
  void growfldtab (size_t n);
  Node* nodealloc (int n);
  Cell* makedfa (const char* s);

  int status;           //!< Interpreter status. See below
#define AWKS_INIT       1   //!< status block initialized
#define AWKS_COMPILING  2   //!< compilation started
#define AWKS_COMPILED   3   //!< Program compiled
#define AWKS_RUN        4   //!< Program running
#define AWKS_DONE       5   //!< Program finished 

  int err;              //!< Last error or warning
  char errmsg[1024];    //!< Last error message
  bool first_run;       //!< true on first run after compile
  int lineno;           //!< line number in awk program
  Array *symtab;        //!< symbol table
  Node *prog_root;      //!< root of parsing tree
  int argno;            //!< current input argument number */
  Array *envir;         //!< environment variables
  Array* argvtab;       //!< ARGV[n] array
  std::mt19937 randgen; //!< Random number generator
  unsigned int rand_seed; //!< Random generator seed
  char *lexprog;        //!< AWK script
  char *lexptr;         //!< pointer in AWK script
  int nprog;            //!< number of programs
  int curprog;          //!< current program being compiled
  char **progs;         //!< array of program filenames
  FILE_STRUC *files;    //!< opened files
                               //!< 0 = stdin
                               //!< 1 = stdout
                               //!< 2 = stderr
  int nfiles;           //!< number of entries in files table
  FILE* infile;         //!< current input file
  inproc inredir;       //!< input redirection function
  outproc outredir;     //!< output redirection function
  struct Frame  fn;     //!< frame data for current function call
  bool donerec;         //!< true if record is valid (no fld has changed)
  bool donefld;         //!< true if record broken into fields
  std::vector< std::unique_ptr<Cell> > fldtab;   //!< $0, $1, ...
  std::vector< std::unique_ptr<Cell> > ratab;    //!< cache of last few regex

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
  Cell* predefs[NPREDEF];  //!< Predefined variables
  Cell retval;     // function return value

private:
  int refldbld (const char* rec);

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

//extern Interpreter *interp;

struct awk_exception {
  awk_exception (Interpreter &interp, int code, const char* msg);
  Interpreter& interp;
  int err;
};

inline
Cell* Node::to_cell () const
{
  assert (ntype == NVALUE);
  return (Cell*)arg[0].get ();
}

inline 
bool Node::isvalue () const
{ 
  return ntype == NVALUE;
}

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
