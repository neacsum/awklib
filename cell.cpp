#include "awk.h"
#include "proto.h"
#include <stdlib.h>
#include <math.h>

using namespace std;

extern Interpreter* interp;
int cell_count = 0;
std::vector<Cell*> all_cells;

Cell::Cell (const char* n, type t, unsigned char flags, Awkfloat f)
  : ctype{ t }
  , nval{ n? n : string() }
  , flags{ flags }
  , funptr{ nullptr }
  , fval{ f }
  , fmt{ 0 }
  , cnext{ 0 }
{
#ifndef NDEBUG
  dprintf ("Allocated cells = %d\n", id = ++cell_count);
  all_cells.push_back (this);
#endif
}

/// Free a symbol table entry
Cell::~Cell ()
{
#ifndef NDEBUG
  dprintf ("Deleting %s(%d)...", nval.c_str(), id);
  print_cell (this, 1);
  for (auto p = all_cells.begin(); p != all_cells.end(); ++p)
  {
    if (*p == this)
    {
      all_cells.erase (p);
      break;
    }
  }
#endif
  if (isarr ())
  {
    dprintf (" array\n");
    delete arrval;
  }
  else if (isregex ())
  {
    dprintf ("regex\n");
    delete re;
  }
  dprintf ("Remaining cells = %d\n", --cell_count);
}

// Assignment operator copies the value from another cell
Cell& Cell::operator= (const Cell& rhs)
{
  if ((rhs.flags & (NUM | STR)) == 0)
    funnyvar (&rhs, "operator=");

  flags &= ~(NUM | STR);
  sval.clear ();
  fval = 0;
  switch (rhs.flags & (STR | NUM))
  {
  case (NUM | STR):
    sval = rhs.sval;
    fval = rhs.fval;
    flags |= NUM | STR;
    break;
  case STR:
    sval = rhs.sval;
    flags |= STR;
    break;
  case NUM:
    fval = rhs.fval;
    flags |= NUM;
    break;
  }
  return *this;
}

///  Set string val of a Cell
void Cell::setsval (const char* s)
{
  int fldno;
  Awkfloat f;

  dprintf ("starting setsval %s = <%s>, t=%s, r,f=%d,%d\n",
    nval.c_str(), s, flags2str (flags), interp->donerec, interp->donefld);
  if (isarr() || isfcn ())
    funnyvar (this, "assign to");
  if (isfld())
  {
    interp->donefld = false;  /* mark $1... invalid */
    fldno = stoi (nval);
    if (fldno > NF)
      interp->setlastfld (fldno);
    dprintf ("setting $%d to %s\n", fldno, s);
  }
  else if (isrec())
  {
    interp->donefld = false;  /* mark $1... invalid */
    interp->donerec = true;
  }
  else if (this == interp->CELL_OFS)
  {
    if (!interp->donerec)
      interp->recbld ();
  }
  flags &= ~(NUM | CONVC);
  flags |= STR;
  fmt.clear();
  sval = s;
  if (is_number (s))
  {
    fval = atof (s);
    flags |= NUM;
  }
  else
    fval = 0.;

  if (isnf())
  {
    interp->donerec = false;  /* mark $0 invalid */
    f = getfval ();
    interp->setlastfld ((int)f);
    dprintf ("setting NF to %g\n", f);
  }

  dprintf ("setsval %c%s = <%s>, t=%s r,f=%d,%d\n",
    (isfld () || isrec ()) ? '$' : ' ', nval.c_str (), sval.c_str (), flags2str (flags), interp->donerec, interp->donefld);
}

///  Set float val of a Cell
void Cell::setfval (Awkfloat f)
{
  int fldno;

  f += 0.0;    /* normalize negative zero to positive zero */
  if (isarr() || isfcn())
    funnyvar (this, "assign to");
  sval.clear (); /* free any previous string */
  flags &= ~(STR | CONVC); /* mark string invalid */
  fmt.clear ();
  flags |= NUM;  /* mark number ok */
  if (isfld ())
  {
    interp->donerec = false;  /* mark $0 invalid */
    fldno = stoi (nval);
    if (fldno > NF)
      interp->setlastfld (fldno);
  }
  else if (isnf ())
  {
    interp->donerec = false;  /* mark $0 invalid */
    interp->setlastfld ((int)f);
    dprintf ("setting NF to %g\n", f);
  }
  else if (isrec ())
  {
    interp->donefld = false;  /* mark $1... invalid */
    interp->donerec = true;
  }
  fval = f;
  if (isrec () || isfld ())
  {
    //fields have always a string value
    fmt = CONVFMT;
    update_str_val ();
  }
  else
    dprintf ("setfval %c%s = %g, t=%s\n", 
      (isfld () || isrec ()) ? '$' : ' ', nval.c_str(), f, flags2str (flags));
}

///  Get float val of a Cell
Awkfloat Cell::getfval ()
{
  if ((flags & (NUM | STR)) == 0)
    funnyvar (this, "read float value of");

  if ((isfld () || isnf ()) && !interp->donefld)
    interp->fldbld ();
  else if (isrec () && !interp->donerec)
    interp->recbld ();

  if (!(flags & NUM))  /* no numeric value */
    fval = sval.empty() ? 0. : atof (sval.c_str());  /* best guess */

  dprintf ("getfval %c%s = %g, t=%s\n",
    (isfld () || isrec ()) ? '$' : ' ', nval.c_str (), fval, flags2str (flags));
  return fval;
}

/// Get string value of a Cell
const char* Cell::getsval ()
{
  if ((flags & (NUM | STR)) == 0)
    funnyvar (this, "read string value of");

  if ((isfld () || isnf ()) && !interp->donefld)
      interp->fldbld ();
  else if (isrec () && !interp->donerec)
      interp->recbld ();

  if ((flags & STR) == 0)
  {
    // don't have a string value but can make one
    fmt = CONVFMT;
    update_str_val ();
    flags |= CONVC;
  }

  dprintf ("getsval %c%s = <%s>, t=%s\n", 
    (isfld () || isrec ()) ? '$' : ' ', nval.c_str (), sval.c_str (), flags2str (flags));
  return sval.c_str();
}

/// Get string val of a Cell for print */
const char* Cell::getpssval ()
{
  if ((flags & (NUM | STR)) == 0)
    funnyvar (this, "print string value of");

  if ((isfld () || isnf()) && !interp->donefld)
      interp->fldbld ();
  else if (isrec () && !interp->donerec)
      interp->recbld ();

  if ((flags & STR) == 0)
  {
    fmt = OFMT;
    update_str_val ();
    flags &= ~CONVC;
  }

  dprintf ("getpsval %c%s = <%s>, t=%s\n",
    (isfld () || isrec ()) ? '$' : ' ', nval.c_str (), sval.c_str (), flags2str (flags));
  return sval.c_str();
}

/// Clear cell content
void Cell::clear ()
{
  sval.clear ();
  flags = STR;
  fval = 0.;
  fmt.clear ();
}

///Turn cell into an array discarding any previous content
void Cell::makearray (size_t sz)
{
  sval.clear ();
  fmt.clear ();
  fval = 0.;
  flags = ARR;
  arrval = new Array (NSYMTAB);
}


/// Check if string matches regular expression of this cell
bool Cell::match (const char* str, bool anchored)
{
  assert (isregex ());
  regex_constants::match_flag_type flags = regex_constants::match_default;
  if (anchored)
    flags |= (regex_constants::match_not_bol | regex_constants::match_not_eol);

  bool result = regex_search (str, *re, flags);
  return result;
}

/// pattern match, for sub
bool Cell::pmatch (const char* p0, size_t& start, size_t& len, bool anchored)
{
  assert (isregex ());
  regex_constants::match_flag_type flags = regex_constants::match_default;
  if (anchored)
    flags |= (regex_constants::match_not_bol | regex_constants::match_not_eol);

  cmatch m;
  if (regex_search (p0, m, *re, flags))
  {
    start = m.position ();
    len = m.length ();
    return true;
  }
  return false;
}



/* Don't duplicate the code for actually updating the value */
void Cell::update_str_val ()
{
  char s[100];
  double dtemp;
  if (modf (fval, &dtemp) == 0)  /* it's integral */
    sprintf (s, "%.30g", fval);
  else
    sprintf (s, fmt.c_str(), fval);
  sval = s;
}

