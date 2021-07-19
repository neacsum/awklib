#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "awk.h"
#include "ytab.h"
#include "proto.h"
#include <awklib/err.h>

#define  DEFAULT_FLD  2         // Initial number of fields
#define  DEFAULT_ARGV 3         // Initial number of entries in ARGV

static int file_cmp (const char* f1, const char* f2);

Node* nullnode;    /* zero&null, converted into a node for comparisons */
Cell* literal0;

Interpreter::Interpreter ()
  : status{ AWKS_INIT }
  , err{ 0 }
  , first_run{ true }
  , symtab{ new Array (NSYMTAB) }
  , prog_root{ 0 }
  , argno{ 1 }
  , envir{ 0 }
  , argvtab{ 0 }
  , srand_seed{ 1. }
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
  syminit ();
  envinit ();
  srand ((unsigned int)srand_seed);

  // Initialize $0 and fields
  //first allocation - deal with $0
  fldtab = new Cell (Cell::type::OCELL, Cell::subtype::CREC, "0", NULL, 0., NUM | STR);
  nfields = 0;
  makefields (DEFAULT_FLD);

  files = new FILE_STRUC[nfiles];
  memset (files, 0, sizeof (FILE_STRUC) * nfiles);
  files[0].fp = stdin;
  files[0].fname = strdup ("/dev/stdin");;
  files[0].mode = LT;
  files[1].fp = stdout;
  files[1].fname = strdup ("/dev/stdout");
  files[1].mode = GT;
  files[2].fp = stderr;
  files[2].fname = strdup ("/dev/stderr");
  files[2].mode = GT;
}

Interpreter::~Interpreter ()
{
  if (files[0].fp != stdin)
    fclose (files[0].fp);

  freenode (prog_root);
  dprintf ("freeing symbol table\n");
  symtab->removesym ("SYMTAB"); //break recursion
  delete symtab;
  for (int i = 0; i < nprog; i++)
    delete progs[i];
  delete progs;
  delete lexprog;
  delete files;

  delete []fldtab;
}

/// Initialize symbol table with built-in vars
void Interpreter::syminit ()
{
  literal0 = symtab->setsym ("0", "0", 0.0, NUM | STR);
  /*
    this is used for if(x)... tests:
  TODO Get rid of nullnode !!
  How do you ensure there are no other nodes linked to it. Also reusing the
  same node creates problems when parsing tree has to be freed.
  */
  nullnode = celltonode (symtab->setsym ("$zero&null", "", 0.0, NUM | STR)
    , Cell::subtype::CCON);

  CELL_FS = symtab->setsym ("FS", " ", 0.0, STR);
  CELL_RS = symtab->setsym ("RS", "\n", 0.0, STR);
  CELL_OFS = symtab->setsym ("OFS", " ", 0.0, STR);
  CELL_ORS = symtab->setsym ("ORS", "\n", 0.0, STR);
  CELL_OFMT = symtab->setsym ("OFMT", "%.6g", 0.0, STR);
  CELL_CONVFMT = symtab->setsym ("CONVFMT", "%.6g", 0.0, STR);
  CELL_FILENAME = symtab->setsym ("FILENAME", "", 0.0, STR);
  CELL_NF = symtab->setsym ("NF", "", 0.0, NUM);
  CELL_NR = symtab->setsym ("NR", "", 0.0, NUM);
  CELL_FNR = symtab->setsym ("FNR", "", 0.0, NUM);
  CELL_SUBSEP = symtab->setsym ("SUBSEP", "\034", 0.0, STR);
  CELL_RSTART = symtab->setsym ("RSTART", "", 0.0, NUM);
  CELL_RLENGTH = symtab->setsym ("RLENGTH", "", 0.0, NUM);
  CELL_ARGC = symtab->setsym ("ARGC", "", 1.0, NUM);
  symtab->setsym ("SYMTAB", (char*)symtab, 0.0, ARR);

  symtab->setsym ("ARGV", (char*)(argvtab = new Array (DEFAULT_ARGV)), 0., ARR);

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
  cp = symtab->setsym ("ENVIRON", "", 0.0, ARR);
  envtab = new Array (NSYMTAB);
  cp->arrval = envtab;
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

/// Create fields up to $nf inclusive
void Interpreter::makefields (int nf)
{
  if (nf < nfields)
    return;   //all good

  size_t sz = (nf + 1) * sizeof (Cell);
  fldtab = (Cell*)realloc (fldtab, sz);
  if (!fldtab)
    FATAL (AWK_ERR_NOMEM, "out of space growing fields to %d", nf);

  for (int i = nfields + 1; i <= nf; i++)
  {
    char temp[50];
    sprintf (temp, "%d", i);
    memset (&fldtab[i], 0, sizeof (Cell));
    fldtab[i].ctype = Cell::type::OCELL;
    fldtab[i].csub = Cell::subtype::CFLD;
    fldtab[i].nval = tostring (temp);
    fldtab[i].tval =  NUM | STR;
  }
  nfields = nf;
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
    //terminate redirection
    fname = (nf == 0) ? "/dev/stdin" :
      (nf == 1) ? "/dev/stdout" : "/dev/stderr";
    f = (nf == 0) ? stdin :
      (nf == 1) ? stdout : stderr;
    if (fs.fp != f)
      fclose (fs.fp);
  }
  else
  {
    f = fopen (fname, nf == 0 ? "r" : "w");
    if (!f)
      return; //something wrong

    if (fs.fp != stdin && fs.fp != stdout && fs.fp != stderr)
      fclose (fs.fp);
  }
  fs.fp = f;
  free (fs.fname);
  fs.fname = strdup (fname);
}

/// Execution of parse tree starts here
void Interpreter::run ()
{
  status = AWKS_RUN;

  //open I/O streams
  if (files[0].fp != stdin)
    files[0].fp = fopen (files[0].fname, "r");
  if (files[1].fp != stdout)
    files[1].fp = fopen (files[1].fname, "w");
  if (files[2].fp != stderr)
    files[2].fp = fopen (files[2].fname, "w");
  if (!first_run)
    clean_symtab ();
  initgetrec ();

  execute (prog_root);
  if (err)
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

      xfree (files[i].fname);
      files[i].fp = NULL;
    }
  }
}

/// Clear symbol table after a previous run
void Interpreter::clean_symtab ()
{
  Cell* cp;
  dprintf ("Starting symtab cleanup\n");
  for (int i = 0; i < symtab->size; i++)
  {
    for (cp = symtab->tab[i]; cp; cp = cp->cnext)
    {
      if (cp->csub != Cell::subtype::CVAR || cp->isfcn())
        continue;
      dprintf ("cleaning %s t=%s\n", NN (cp->nval), flags2str (cp->tval));
      if (cp->isarr ())
      {
        if (cp->arrval != symtab)
          delete cp->arrval;
      }
      else if (!cp->isfcn ())
        delete cp->sval;
      cp->sval = 0;
      cp->tval = NUM;
      cp->fval = 0.;
      cp->fmt = 0;
    }
  }
  dprintf ("Done symtab cleanup\n");
}
/// Find first filename argument
void Interpreter::initgetrec ()
{
  int i;
  const char* p;

  infile = NULL;
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
      return;
    argno++;
  }
  infile = files[0].fp;    /* no filenames, so use stdin (or its redirect)*/
  MY_FILENAME = strdup (files[0].fname);
}

/// Get next input record
int Interpreter::getrec (Cell* cell)
{
  const char* file = 0;

  dprintf ("RS=<%s>, FS=<%s>, ARGC=%d, FILENAME=%s\n",
    quote (MY_RS), quote (MY_FS), (int)MY_ARGC, MY_FILENAME);
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
      xfree (MY_FILENAME);
      MY_FILENAME = strdup (file);
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
      return 1;
    }
    /* EOF arrived on this file; set up next */
    nextfile ();
  }
  return 0;  /* true end of file(s) */
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
int Interpreter::readrec (Cell* cell, FILE* inf)
{
  int sep, c;
  char* rr, * buf;
  size_t bufsize = RECSIZE;

  delete cell->sval;
  cell->sval = 0;
  buf = new char[bufsize];

  if ((sep = *MY_RS) == 0)
  {
    sep = '\n';
    while ((c = getchar (inf)) == '\n' && c != EOF)  /* skip leading \n's */
      ;
    if (c != EOF)
      ungetc (c, inf);
  }
  for (rr = buf; ; )
  {
    for (; (c = getchar (inf)) != sep && c != EOF; )
    {
      if ((size_t)(rr - buf + 1) > bufsize)
        adjbuf (&buf, &bufsize, 1 + rr - buf, RECSIZE, &rr);
      *rr++ = c;
    }
    if (*MY_RS == sep || c == EOF)
      break;

    /*
      **RS != sep and sep is '\n' and c == '\n'
      This is the case where RS = 0 and records are separated by two
      consecutive \n
    */
    if ((c = getchar (inf)) == '\n' || c == EOF) /* 2 in a row */
      break;
    adjbuf (&buf, &bufsize, 2 + rr - buf, RECSIZE, &rr);
    *rr++ = '\n';
    *rr++ = c;
  }
  * rr = 0;
  dprintf ("readrec saw <%s>, returns %d\n", buf, c == EOF && rr == buf ? 0 : 1);
  cell->sval = buf;
  cell->tval = STR;
  if (is_number (cell->sval))
  {
    cell->fval = atof (cell->sval);
    cell->tval |= NUM | CONVC;
  }
  return c == EOF && rr == buf ? 0 : 1;
}

/// Get ARGV[n]
const char* Interpreter::getargv (int n)
{
  Cell* x;
  const char* s;
  char temp[50];

  sprintf (temp, "%d", n);
  if (argvtab->lookup (temp) == NULL)
    return NULL;
  x = argvtab->setsym (temp, "", 0.0, STR);
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
    q->fval = atof (q->sval);
    q->tval |= NUM;
  }
  dprintf ("command line set %s to |%s|\n", s, p);
}

/// Create fields from current record
void Interpreter::fldbld ()
{
  char* fields, * fb, * fe, sep;
  Cell* p;
  int i, j;

  if (donefld)
    return;

  fields = strdup (fldtab[0].getsval ());
  i = 0;  /* number of fields accumulated here */
  fb = fields;        //beginning of field
  if (strlen (MY_FS) > 1)
  {
    /* it's a regular expression */
    i = refldbld (fields, MY_FS);
  }
  else if ((sep = *MY_FS) == ' ')
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
      if (++i > nfields)
        growfldtab (i);
      delete fldtab[i].sval;
      fldtab[i].sval = tostring (fb);
      fb = fe;
    }
  }
  else if ((sep = *MY_FS) == 0)
  {
    /* new: FS="" => 1 char/field */
    for (i = 0; *fb != 0; fb++)
    {
      char buf[2];
      if (++i > nfields)
        growfldtab (i);
      buf[0] = *fb;
      buf[1] = 0;
      delete fldtab[i].sval;
      fldtab[i].sval = tostring (buf);
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
    if (strlen (MY_RS) > 0)
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

      if (++i > nfields)
        growfldtab (i);
      delete fldtab[i].sval;
      fldtab[i].sval = tostring (fb);
    }
  }
  cleanfld (i + 1, lastfld);  /* clean out junk from previous record */
  lastfld = i;
  donefld = true;

  for (j = 1, p = &fldtab[1]; j <= lastfld; j++, p++)
  {
    p->tval = STR;
    if (is_number (p->sval))
    {
      p->fval = atof (p->sval);
      p->tval |= (NUM | CONVC);
    }
  }
  MY_NF = lastfld;
  donerec = true; /* restore */
  free (fields);
#ifndef NDEBUG
  if (dbg)
  {
    for (j = 0; j <= lastfld; j++)
      dprintf ("field %d (%s): |%s|\n", j, fldtab[j].nval, fldtab[j].sval);
  }
#endif
}

/// Build fields from reg expr in FS
int Interpreter::refldbld (const char* rec, const char* fs)
{
  char* fr, * fields;
  int i, tempstat;
  fa* pfa;

  fields = strdup (rec);
  fr = fields;
  *fr = '\0';
  if (*rec == '\0')
    return 0;
  pfa = makedfa (fs, 1);
  dprintf ("into refldbld, rec = <%s>, pat = <%s>\n", rec, fs);
  tempstat = pfa->initstat;
  for (i = 1; ; i++)
  {
    if (i > nfields)
      growfldtab (i);
    delete fldtab[i].sval;
    fldtab[i].tval = STR;
    fldtab[i].sval = tostring (fr);
    dprintf ("refldbld: i=%d\n", i);
    if (nematch (pfa, rec))
    {
      pfa->initstat = 2;  /* horrible coupling to b.c */
      dprintf ("match %s (%d chars)\n", patbeg, patlen);
      strncpy (fr, rec, patbeg - rec);
      fr += patbeg - rec + 1;
      *(fr - 1) = '\0';
      rec = patbeg + patlen;
    }
    else
    {
      dprintf ("no match %s\n", rec);
      strcpy (fr, rec);
      pfa->initstat = tempstat;
      break;
    }
  }
  free (fields);
  return i;
}

/// Clean out fields n1 .. n2 inclusive
void Interpreter::cleanfld (int n1, int n2)
{
  /* nvals remain intact */
  int i;

  for (i = n1; i <= n2; i++)
  {
    delete fldtab[i].sval;
    fldtab[i].sval = 0;
    fldtab[i].tval = STR | NUM;
    fldtab[i].fval = 0.;
    fldtab[i].fmt = NULL;
  }
}

/// Add field n after end of existing lastfld
void Interpreter::newfld (int n)
{
  if (n > nfields)
    growfldtab (n);
  MY_NF = lastfld = n;
}

/// Set lastfld cleaning fldtab cells if necessary
void Interpreter::setlastfld (int n)
{
  if (n > nfields)
    growfldtab (n);

  if (lastfld < n)
    cleanfld (lastfld + 1, n);
  else
    cleanfld (n + 1, lastfld);

  lastfld = n;
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
  if (n > nfields)  /* fields after NF are empty */
    growfldtab (n);
  return &fldtab[n];
}

/// Make new fields up to at least $n
void Interpreter::growfldtab (size_t n)
{
  size_t nf = 2 * nfields;

  if (n > nf)
    nf = n;
  makefields ((int)nf);
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