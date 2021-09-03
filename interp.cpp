#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <mutex>

#include "awk.h"
#include "ytab.h"
#include "proto.h"
#include <awklib/err.h>

#define  DEFAULT_FLD  2         // Initial number of fields
#define  DEFAULT_ARGV 3         // Initial number of entries in ARGV
#define  NFA  20                // Cache this many dynamic regex's

static int file_cmp (const char* f1, const char* f2);

Cell* literal_null;
Cell* literal0;

extern std::string curfname;  ///<current function name
extern Interpreter* interp;

Interpreter::Interpreter ()
  : status{ AWKS_INIT }
  , err{ 0 }
  , first_run{ true }
  , lineno{ 1 }
  , symtab{ new Array (NSYMTAB) }
  , prog_root{ 0 }
  , argno{ 1 }
  , envir{ 0 }
  , argvtab{ 0 }
  , rand_seed{ std::mt19937::default_seed }
  , lexprog{ 0 }
  , lexptr{ 0 }
  , nprog{ 0 }
  , curprog{ 0 }
  , progs{ 0 }
  , files{ 0 }
  , nfiles{ FOPEN_MAX }
  , infile{ 0 }
  , inredir{ 0 }
  , outredir{ 0 }
  , donerec{ false }
  , donefld{ false }
{
  *errmsg = 0;
  syminit ();
  envinit ();

  // Initialize $0 and fields
  //first allocation - deal with $0
  fldtab.push_back (std::make_unique<Cell> ("0", Cell::type::REC, STR));
  makefields (DEFAULT_FLD);

  files = new FILE_STRUC[nfiles];
  memset (files, 0, sizeof (FILE_STRUC) * nfiles);
  files[0].fp = stdin;
  files[0].fname = tostring ("-");
  files[0].mode = LT;
  files[1].fp = stdout;
  files[1].fname = tostring ("/dev/stdout");
  files[1].mode = GT;
  files[2].fp = stderr;
  files[2].fname = tostring ("/dev/stderr");
  files[2].mode = GT;
}

Interpreter::~Interpreter ()
{
  if (files[0].fp && files[0].fp != stdin)
    fclose (files[0].fp);
  delete files[0].fname;
  if (files[1].fp && files[1].fp != stdout)
    fclose (files[1].fp);
  delete files[1].fname;
  if (files[2].fp && files[2].fp != stderr)
    fclose (files[2].fp);
  delete files[2].fname;

  delete prog_root;
  dprintf ("freeing symbol table\n");
  Cell *p = symtab->removesym ("SYMTAB"); //break recursive link
  delete p;
  for (int i = 0; i < nprog; i++)
    delete progs[i];
  delete progs;
  delete lexprog;
  delete files;
}

/// Initialize symbol table with built-in vars
void Interpreter::syminit ()
{
  literal0 = symtab->setsym ("0", "0", 0.0, (NUM | STR | PREDEF | CONST));
  literal_null = symtab->setsym ("$zero&null", "", 0.0, (NUM | STR | PREDEF | CONST));

  CELL_FS = symtab->setsym ("FS", " ", 0.0, STR | PREDEF);
  CELL_RS = symtab->setsym ("RS", "\n", 0.0, STR | PREDEF);
  CELL_OFS = symtab->setsym ("OFS", " ", 0.0, STR | PREDEF);
  CELL_ORS = symtab->setsym ("ORS", "\n", 0.0, STR | PREDEF);
  CELL_OFMT = symtab->setsym ("OFMT", "%.6g", 0.0, STR | PREDEF);
  CELL_CONVFMT = symtab->setsym ("CONVFMT", "%.6g", 0.0, STR | PREDEF);
  CELL_FILENAME = symtab->setsym ("FILENAME", "", 0.0, STR | PREDEF);
  CELL_NF = symtab->setsym ("NF", "", DEFAULT_FLD, NUM | PREDEF);
  CELL_NF->ctype = Cell::type::CNF;
  CELL_NR = symtab->setsym ("NR", "", 0.0, NUM | PREDEF);
  CELL_FNR = symtab->setsym ("FNR", "", 0.0, NUM | PREDEF);
  CELL_SUBSEP = symtab->setsym ("SUBSEP", "\034", 0.0, STR | PREDEF);
  CELL_RSTART = symtab->setsym ("RSTART", "", 0.0, NUM | PREDEF);
  CELL_RLENGTH = symtab->setsym ("RLENGTH", "", 0.0, NUM | PREDEF);
  CELL_ARGC = symtab->setsym ("ARGC", "", 1.0, NUM | PREDEF);

  symtab->setsym ("ARGV", (argvtab = new Array (DEFAULT_ARGV)), PREDEF);
  symtab->setsym ("SYMTAB", symtab, PREDEF);

  //add a fake argv[0]
  argvtab->setsym ("0", "AWKLIB", 0., STR);
}

/// Set up ENVIRON variable
void Interpreter::envinit ()
{
  Cell* cp;
  char* p;
  Array* envtab;     /* symbol table containing ENVIRON[...] */
  char** envp;
  envp = environ;
  cp = symtab->setsym ("ENVIRON", (envtab = new Array (NSYMTAB)), PREDEF);
  ;
  for (; *envp; envp++)
  {
    if ((p = strchr (*envp, '=')) == NULL)
      continue;
    if (p == *envp) /* no left hand side name in env string */
      continue;
    *p++ = 0;  /* split into two strings at = */
    if (is_number (p))
      envtab->setsym (*envp, p, atof (p), STR | NUM);
    else
      envtab->setsym (*envp, p, 0.0, STR);
    p[-1] = '=';  /* restore in case env is passed down to a shell */
  }
}

/// Create fields up to nf inclusive
void Interpreter::makefields (size_t nf)
{
  if (nf < fldtab.size())
    return;   //all good

  for (size_t i = fldtab.size (); i <= nf; i++)
  {
    char temp[50];
    sprintf (temp, "%zu", i);
    fldtab.push_back (
      std::make_unique<Cell> (temp, Cell::type::FLD, STR));
  }
}

/// Redirect one of the standard streams (stdin, stdout, stderr) to another file
/// or terminates a previous redirection
void Interpreter::std_redirect (int nf, const char* fname)
{
  FILE* f;
  if (!file_cmp (fname, files[nf].fname))
    return; //same file

  FILE_STRUC& fs = files[nf];
  if (!strcmp (fname, "-"))
  {
    f = (nf == 0) ? stdin :
        (nf == 1) ? stdout :
                    stderr;
    if (fs.fp != f)
      fclose (fs.fp);
  }
  else
  {
    f = fopen (fname, nf == 0 ? "r" : "w");
    if (!f)
      return; //something wrong

    if (fs.fp && fs.fp != stdin && fs.fp != stdout && fs.fp != stderr)
      fclose (fs.fp);
  }
  fs.fp = f;
  delete fs.fname;
  fs.fname = tostring (fname);
}

/// Execution of parse tree starts here
void Interpreter::run ()
{
  status = AWKS_RUN;

  //open input stream...
  if (!files[0].fp)
  {
    if (!strcmp (files[0].fname, "-"))
      files[0].fp = stdin;
    else
      files[0].fp = fopen (files[0].fname, "r");
  }

  //...and output streams
  if (!files[1].fp)
  {
    if (!strcmp (files[1].fname, "/dev/stdout"))
      files[1].fp = stdout;
    else
      files[1].fp = fopen (files[1].fname, "w");
  }
  if (!files[2].fp)
  {
    if (!strcmp (files[2].fname, "/dev/stderr"))
      files[2].fp = stderr;
    else
      files[2].fp = fopen (files[2].fname, "w");
  }

  if (!first_run)
    clean_symtab ();
  initgetrec ();

  execute (prog_root);
  if (err < 0)
    status = AWKS_DONE;
  else
    status = AWKS_COMPILED; //ready to run again
  first_run = false;
  closeall ();
}

void Interpreter::closeall ()
{
  int i, stat;

  //skip stdin, stdout and stderr
  if (files[0].fp != stdin)
  {
    fclose (files[0].fp);
    files[0].fp = 0;
  }
  if (files[1].fp != stdout)
  {
    fclose (files[1].fp);
    files[1].fp = 0;
  }
  if (files[2].fp != stderr)
  {
    fclose (files[2].fp);
    files[2].fp = 0;
  }
  for (i = 3; i < nfiles; i++)
  {
    if (files[i].fp)
    {
      if (ferror (files[i].fp))
        WARNING ("i/o error occurred on %s", files[i].fname);
      if (files[i].mode == '|' || files[i].mode == LE)
        stat = pclose (files[i].fp);
      else
        stat = fclose (files[i].fp);
      if (stat == EOF)
        WARNING ("i/o error occurred while closing %s", files[i].fname);

      delete files[i].fname;
      files[i].fp = NULL;
    }
  }
}

/// Clear symbol table after a previous run
void Interpreter::clean_symtab ()
{
  dprintf ("Starting symtab cleanup\n");
  for (auto cp : *symtab)
  {
    if (cp->ctype == Cell::type::CELL)
    {
      if ((cp->flags & (CONST | PREDEF)) == 0)
      {
        dprintf ("cleaning %s t=%s\n", cp->nval.c_str(), flags2str (cp->flags));
        cp->sval.clear ();
        cp->fval = 0.;
        if (cp->flags & ARR)
        {
          dprintf ("%s is an array\n", cp->nval.c_str());
          delete cp->arrval;
          cp->arrval = new Array (NSYMTAB);
        }
      }
      else
        dprintf ("skipping %s t=%s\n", cp->nval.c_str(), flags2str (cp->flags));
    }
  }

  //Revert to default values
  MY_FS = MY_OFS = " ";
  MY_RS = MY_ORS = "\n";
  MY_OFMT = MY_CONVFMT = "%.6g";
  MY_SUBSEP = "\034";
  MY_FILENAME.clear();
  dprintf ("Done symtab cleanup\n");
}
/// Find first filename argument
void Interpreter::initgetrec ()
{
  int i;
  const char* p;

  argno = 1;
  CELL_FNR->setfval (0.);
  CELL_NR->setfval (0.);
  for (i = 1; i < MY_ARGC; i++)
  {
    p = getargv (i); /* find 1st real filename */
    if (p == NULL || *p == '\0')
    {  /* deleted or zapped */
      argno++;
      continue;
    }
    if (isclvar (p))
      setclvar (p);  /* a command line assignment before filename */
    else
    {
      if ((infile = fopen (p, "r")) == NULL)
        FATAL (AWK_ERR_INFILE, "can't open file %s", infile);
      return;
    }
    argno++;
  }
  infile = files[0].fp;    /* no filenames, so use stdin (or its redirect)*/
}

/// Get next input record
bool Interpreter::getrec (Cell* cell)
{
  const char* file = 0;

  dprintf ("RS=<%s>, FS=<%s>, ARGC=%d, FILENAME=%s\n",
    quote (MY_RS).c_str(), quote (MY_FS).c_str (), (int)MY_ARGC, MY_FILENAME.c_str());
  if (cell->isrec ())
  {
    donefld = false;
    donerec = true;
  }
  while (argno < (int)MY_ARGC || infile == files[0].fp)
  {
    dprintf ("argno=%d, file=|%s|\n", argno, NN (file));
    if (infile == NULL)
    {
      /* have to open a new file */
      file = getargv (argno);
      if (file == NULL || *file == '\0')
      {
        /* deleted or zapped */
        argno++;
        continue;
      }
      if (isclvar (file))
      {
        /* a var=value arg */
        setclvar (file);
        argno++;
        continue;
      }
      MY_FILENAME = file;
      dprintf ("opening file %s\n", file);
      if (*file == '-' && *(file + 1) == '\0')
        infile = stdin;
      else if ((infile = fopen (file, "r")) == NULL)
        FATAL (AWK_ERR_INFILE, "can't open file %s", file);
      MY_FNR = 0;
    }
    if (readrec (cell, infile))
    {
      MY_NR = MY_NR + 1;
      MY_FNR = MY_FNR + 1;
      return true;
    }
    /* EOF arrived on this file; set up next */
    nextfile ();
  }
  return false;  /* end of file(s) */
}

void Interpreter::nextfile ()
{
  if (infile != NULL && infile !=files[0].fp)
    fclose (infile);
  infile = NULL;
  argno++;
}

/// Get input char from input file or from input redirection function
int Interpreter::getchar (FILE* inf)
{
  int c = (inf == files[0].fp && inredir) ? inredir () : getc (inf);
  return c;
}

/// Read one record in cell's string value
bool Interpreter::readrec (Cell* cell, FILE* inf)
{
  int sep, c;

  cell->sval.clear ();

  if (MY_RS.empty ())
  {
    sep = '\n';
    while ((c = getchar (inf)) == '\n' && c != EOF)  /* skip leading \n's */
      ;
    if (c != EOF)
      ungetc (c, inf);
  }
  else
    sep = MY_RS[0];

  while (1)
  {
    for (; (c = getchar (inf)) != sep && c != EOF; )
      cell->sval.push_back (c);

    if (MY_RS[0] == sep || c == EOF)
      break;

    /*
      **RS != sep and sep is '\n' and c == '\n'
      This is the case where RS = 0 and records are separated by two
      consecutive \n
    */
    if ((c = getchar (inf)) == '\n' || c == EOF) /* 2 in a row */
      break;
    cell->sval.push_back ('\n');
    cell->sval.push_back (c);
  }
  cell->flags = STR;
  if (is_number (cell->sval))
  {
    cell->fval = atof (cell->sval.c_str());
    cell->flags |= NUM | CONVC;
  }

  bool ret = c != EOF || !cell->sval.empty ();
  dprintf ("readrec saw <%s>, returns %s\n",
    cell->sval.c_str (), ret? "true" : "false");
  return ret;
}

/// Get ARGV[n]
const char* Interpreter::getargv (int n)
{
  Cell* x;
  const char* s;
  char temp[50];

  sprintf (temp, "%d", n);
  if (!(x = argvtab->lookup (temp)))
    return NULL;

  s = x->getsval ();
  dprintf ("getargv(%d) returns |%s|\n", n, s);
  return s;
}

// write string to output file or send it to output redirection function
int Interpreter::putstr (const char* str, FILE* fp)
{
  if (fp == files[1].fp && outredir)
    return outredir (str, strlen (str));
  else
    return fputs (str, fp);
}


/*!
  Command line variable.
  Set var=value from s

  Assumes input string has correct format.
*/
void Interpreter::setclvar (const char* s)
{
  const char* p;
  Cell* q;

  for (p = s; *p != '='; p++)
    ;
  p = qstring (p + 1, '\0');
  s = qstring (s, '=');
  q = symtab->setsym (s, p, 0.0, STR);
  q->setsval (p);
  if (is_number (q->sval))
  {
    q->fval = stof (q->sval);
    q->flags |= NUM;
  }
  dprintf ("command line set %s to |%s|\n", s, p);
}

/// Create fields from current record
void Interpreter::fldbld ()
{
  char* fields, * fb, * fe, sep;

  if (donefld)
    return;

  fields = strdup (fldtab[0]->getsval ());
  int i = 0;  /* number of fields accumulated here */
  fb = fields;        //beginning of field
  if (MY_FS.size() > 1)
  {
    /* it's a regular expression */
    i = refldbld (fields);
  }
  else if ((sep = MY_FS[0]) == ' ')
  {
    /* default whitespace */
    for (i = 0; ; )
    {
      while (*fb && (*fb == ' ' || *fb == '\t' || *fb == '\n'))
        fb++;
      if (!*fb)
        break;
      fe = fb;        //end of field
      while (*fe && *fe != ' ' && *fe != '\t' && *fe != '\n')
        fe++;
      if (*fe)
        *fe++ = 0;
      if (++i >= fldtab.size())
        growfldtab (i);
      fldtab[i]->sval = fb;
      fb = fe;
    }
  }
  else if (!sep)
  {
    /* new: FS="" => 1 char/field */
    for (i = 0; *fb != 0; fb++)
    {
      char buf[2];
      if (++i >= fldtab.size ())
        growfldtab (i);
      buf[0] = *fb;
      buf[1] = 0;
      fldtab[i]->sval = buf;
    }
  }
  else if (*fb != 0)
  {
    /* if 0, it's a null field */

 /* subtlecase : if length(FS) == 1 && length(RS > 0)
  * \n is NOT a field separator (cf awk book 61,84).
  * this variable is tested in the inner while loop.
  */
    int rtest = '\n';  /* normal case */
    int end_seen = 0;
    if (MY_RS.size() > 0)
      rtest = '\0';
    fe = fb;
    for (i = 0; !end_seen; )
    {
      fb = fe;
      while (*fe && *fe != sep && *fe != rtest)
        fe++;
      if (*fe)
        *fe++ = 0;
      else
        end_seen = 1;

      if (++i >= fldtab.size ())
        growfldtab (i);
      fldtab[i]->sval = fb;
    }
  }
  cleanfld (i + 1, (int)MY_NF);  /* clean out junk from previous record */
  MY_NF = i;
  donefld = true;

  for (i = 1; i <= MY_NF; i++)
  {
    assert (fldtab[i]);
    fldtab[i]->flags = STR;
    if (is_number (fldtab[i]->sval))
    {
      fldtab[i]->fval = atof (fldtab[i]->sval.c_str());
      fldtab[i]->flags |= (NUM | CONVC);
    }
  }
  donerec = true; /* restore */
  free (fields);
#ifndef NDEBUG
  if (dbg)
  {
    for (i = 0; i <= MY_NF; i++)
      dprintf ("field %d (%s): |%s|\n", i, fldtab[i]->nval.c_str(), fldtab[i]->sval.c_str());
  }
#endif
}

/// Create $0 from $1..$NF if necessary
void Interpreter::recbld ()
{
  if (donerec)
    return;

  std::string& rec = fldtab[0]->sval;
  rec.clear ();
  for (int i = 1; i <= MY_NF; i++)
  {
    rec += fldtab[i]->getsval ();
    if (i < MY_NF)
      rec += MY_OFS;
  }
  dprintf ("in recbld $0=|%s|\n", rec.c_str());
  fldtab[0]->flags = STR;
  if (is_number (rec))
  {
    fldtab[0]->flags |= NUM;
    fldtab[0]->fval = atof (rec.c_str());
  }

  donerec = true;
}

/// Build fields from reg expr in FS
int Interpreter::refldbld (const char* in)
{
  std::string rec = in;
  if (rec.empty())
    return 0;
  if (!CELL_FS->isregex())
    CELL_FS->re = new std::regex (CELL_FS->sval, std::regex_constants::awk);

  dprintf ("into refldbld, rec = <%s>, pat = <%s>\n", rec, CELL_FS->sval.c_str());
  int i = 1;
  size_t pstart, plen;
  while (CELL_FS->pmatch (rec.c_str(), pstart, plen))
  {
    if (i >= fldtab.size ())
      growfldtab (i);
    fldtab[i]->flags = STR;
    fldtab[i]->sval = rec.substr (0, pstart);

    dprintf ("match $%d = %s\n", i, fldtab[i]->sval.c_str());
    rec = rec.substr (pstart + plen);
    dprintf ("remaining <%s>\n", rec);
    ++i;
  }
  fldtab[i]->flags = STR;
  fldtab[i]->sval = rec;
  dprintf ("last field $%d = %s\n", i, fldtab[i]->sval.c_str ());

  return i;
}

/// Clean out fields n1 .. n2 inclusive
void Interpreter::cleanfld (int n1, int n2)
{
  for (auto i = n1; i <= n2; i++)
    fldtab[i]->clear ();
}

/// Set last field cleaning fldtab cells if necessary
void Interpreter::setlastfld (int n)
{
  if (n >= fldtab.size ())
    growfldtab (n);

  int lastfld = (int)MY_NF;
  if (lastfld < n)
    cleanfld (lastfld + 1, n);
  else
    cleanfld (n + 1, lastfld);

  MY_NF = n;
}

/// Get nth field
Cell* Interpreter::fieldadr (int n)
{
  if (n < 0)
    FATAL (AWK_ERR_ARG, "trying to access out of range field %d", n);
  if (n && !donefld)
    fldbld ();
  else if (!n && !donerec)
    recbld ();
  if (n >= fldtab.size())  /* fields after NF are empty */
    growfldtab (n);
  return fldtab[n].get();
}

/// Make new fields up to at least $n
void Interpreter::growfldtab (size_t n)
{
  size_t nf = 2 * fldtab.size ();

  if (n > nf)
    nf = n;
  makefields (nf);
}

/// Allocate a node with n descendants
Node* Interpreter::nodealloc (int n)
{
  Node* x =new Node ();
  x->nnext = NULL;
  x->lineno = lineno;
  x->arg.resize (n);
  return x;
}

Cell* Interpreter::makedfa (const char* s)
{
  std::string ss(s);
  requote (ss);
  if (status == AWKS_COMPILING)  /* a constant for sure */
  {
    // TODO reuse regex when compiling
    std::regex* re = new std::regex (ss, std::regex_constants::awk);
    Cell* x = new Cell (s, Cell::type::CELL, (CONST | REGEX));
    x->re = re;
    return x;
  }

  for (int i = 0; i < ratab.size (); i++)  /* is it there already? */
  {
    if (ratab[i]->nval == ss)
    {
      ratab[i]->fval += 1.;
      return ratab[i].get();
    }
  }

  Cell* x = new Cell (s, Cell::type::CELL, REGEX);
  x->re = new std::regex (ss, std::regex_constants::awk);
  if (ratab.size() < NFA)
  {
    /* room for another */
    ratab.emplace_back (x);
    return x;
  }

  Awkfloat use = ratab[0]->fval;  /* replace least-recently used */
  int nuse = 0;
  for (int i = 1; i < ratab.size(); i++)
  {
    if (ratab[i]->fval < use)
    {
      use = ratab[i]->fval;
      nuse = i;
    }
  }

  ratab[nuse] = std::unique_ptr<Cell> (x);
  return x;
}

/// Generate error message and throws an exception
void FATAL (int err, const char* fmt, ...)
{
  static char errmsg[1024];
  va_list varg;
  va_start (varg, fmt);
  vsprintf (errmsg, fmt, varg);
  va_end (varg);
  throw awk_exception (*interp, err, errmsg);
}

// compare two filenames. Windows filenames are case insensitive
int file_cmp (const char* f1, const char* f2)
{
#ifdef _WINDOWS
  return strcmpi (f1, f2);
#else
  return strcmp (f1, f2);
#endif
}

/// Generate syntax error message and throws an exception
void SYNTAX (const char* fmt, ...)
{
  va_list varg;
  static char errmsg[1024];
  char* pb = errmsg;
  va_start (varg, fmt);
  pb += vsprintf (pb, fmt, varg);
  va_end (varg);
  pb += sprintf (pb, " at source line %d", interp->lineno);
  if (!curfname.empty())
    pb += sprintf (pb, " in function %s", curfname.c_str());
  if (interp->status == AWKS_COMPILING && cursource () != NULL)
    pb += sprintf (pb, " source file %s", cursource ());
  throw awk_exception (*interp, AWK_ERR_SYNTAX, errmsg);
}

awk_exception::awk_exception (Interpreter& i, int code, const char* msg)
  : interp{ i }
  , err (code)
{
  interp.err = code;
  strncpy (interp.errmsg, msg, sizeof(interp.errmsg)-1);
}

