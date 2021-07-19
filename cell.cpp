#include "awk.h"
#include "proto.h"
#include <stdlib.h>
#include <math.h>

void print_cell (Cell* c, int indent);

Cell::Cell (type t, subtype s, const char* n, void* v, Awkfloat f, unsigned char flags)
  : ctype{ t }
  , csub{ s }
  , nval{ tostring (n) }
  , sval{ (char*)v }
  , tval{ flags }
  , fval{ f }
  , fmt{ 0 }
  , cnext{ 0 }
{
}

/// Free a symbol table entry
Cell::~Cell ()
{
#ifndef NDEBUG
  dprintf ("Deleting...");
  print_cell (this, 1);
#endif
  if (isarr ())
  {
    dprintf ("Deleting array\n");
    delete arrval;
  }
  else
  {
    dprintf ("freed %s\n", sval);
    delete sval;
  }
  delete nval;
}

///  Set string val of a Cell
void Cell::setsval (const char* s)
{
  char* t;
  int fldno;
  Awkfloat f;

  dprintf ("starting setsval %s = <%s>, t=%s, r,f=%d,%d\n",
    NN (nval), s, flags2str (tval), interp->donerec, interp->donefld);
  if ((tval & (ARR | FCN | EXTFUN)) != 0)
    funnyvar (this, "assign to");
  if (csub == subtype::CFLD)
  {
    interp->donerec = 0;  /* mark $0 invalid */
    fldno = atoi (nval);
    if (fldno > NF)
      interp->newfld (fldno);
    dprintf ("setting $%d to %s\n", fldno, s);
  }
  else if (csub == subtype::CREC)
  {
    interp->donefld = 0;  /* mark $1... invalid */
    interp->donerec = 1;
  }
  else if (this == interp->CELL_OFS)
  {
    if (interp->donerec == 0)
      recbld ();
  }
  t = tostring (s);  /* in case it's self-assign */
  delete sval;
  tval &= ~(NUM | CONVC);
  tval |= STR;
  fmt = NULL;
  sval = t;
  if (is_number (t))
  {
    fval = atof (t);
    tval |= NUM;
  }
  else
  {
    fval = 0.;
    tval &= ~NUM;
  }
  if (this == interp->CELL_NF)
  {
    interp->donerec = 0;  /* mark $0 invalid */
    f = getfval ();
    interp->setlastfld ((int)f);
    dprintf ("setting NF to %g\n", f);
  }

  dprintf ("setsval %s = <%s>, t=%s r,f=%d,%d\n",
    NN (nval), t, flags2str (tval), interp->donerec, interp->donefld);
}

///  Set float val of a Cell
void Cell::setfval (Awkfloat f)
{
  int fldno;

  f += 0.0;    /* normalize negative zero to positive zero */
  if (tval & (ARR | FCN | EXTFUN))
    funnyvar (this, "assign to");
  delete sval; /* free any previous string */
  sval = 0;
  tval &= ~(STR | CONVC); /* mark string invalid */
  fmt = NULL;
  tval |= NUM;  /* mark number ok */
  if (isfld ())
  {
    interp->donerec = false;  /* mark $0 invalid */
    fldno = atoi (nval);
    if (fldno > NF)
      interp->newfld (fldno);
  }
  else if (this == interp->CELL_NF)
  {
    interp->donerec = false;  /* mark $0 invalid */
    interp->setlastfld ((int)f);
    dprintf ("setting NF to %g\n", f);
  }
  else if (isrec ())
  {
    interp->donefld = 0;  /* mark $1... invalid */
    interp->donerec = 1;
  }
  fval = f;
  if (isrec () || isfld ())
  {
    //fields have always a string value
    fmt = CONVFMT;
    update_str_val ();
    dprintf ("setfval $%s = %s, t=%s\n", nval, sval, flags2str (tval));
  }
  else
    dprintf ("setfval %s = %g, t=%s\n", NN (nval), f, flags2str (tval));
}

///  Get float val of a Cell
Awkfloat Cell::getfval ()
{
  if ((tval & (NUM | STR)) == 0)
    funnyvar (this, "read float value of");
  if (isfld () && !interp->donefld)
    interp->fldbld ();
  else if (isrec () && !interp->donerec)
    recbld ();
  if (!(tval & NUM))  /* not a number */
    fval = sval ? atof (sval) : 0.;  /* best guess */

  dprintf ("getfval %s = %g, t=%s\n",
    NN (nval), fval, flags2str (tval));
  return fval;
}

/// Get string value of a Cell
const char* Cell::getsval ()
{
  if ((tval & (NUM | STR)) == 0)
    funnyvar (this, "read string value of");

  if (isfld ())
  {
    if (!interp->donefld)
      interp->fldbld ();
    dprintf ("getsval: $%s: <%s> t=%s\n", nval, sval, flags2str (tval));
  }
  else if (isrec ())
  {
    if (!interp->donerec)
      recbld ();
    dprintf ("getsval: $0: <%s>, t=%s\n", sval, flags2str (tval));
  }
  else
  {
    //this is not $0, $1,...
    if ((tval & STR) == 0)
    {
      // don't have a string value but can make one
      fmt = CONVFMT;
      update_str_val ();
      tval |= CONVC;
    }
    dprintf ("getsval %s = <%s>, t=%s\n", NN (nval), NN (sval), flags2str (tval));
  }
  if (!sval)
    sval = tostring (0);

  return sval;
}

/// Get string val of a Cell for print */
const char* Cell::getpssval ()
{
  if ((tval & (NUM | STR)) == 0)
    funnyvar (this, "print string value of");

  if (isfld ())
  {
    if (!interp->donefld)
      interp->fldbld ();
    if (!sval)
    {
      sval = tostring (0);
      tval |= STR;
    }

    dprintf ("getpsval: $%s: <%s> t=%s\n", nval, sval, flags2str (tval));
    return sval;
  }
  else if (isrec ())
  {
    if (!interp->donerec)
      recbld ();
    dprintf ("getpsval: $0: <%s> t=%s\n", sval, flags2str (tval));
    return sval;
  }

  //this is not $0, $1, ...
  if ((tval & STR) == 0)
  {
    fmt = OFMT;
    update_str_val ();
    tval &= ~CONVC;
  }
  if (!sval)
    sval = tostring (0);

  dprintf ("getpsval %s = <%s>, t=%s\n",
    NN (nval), NN (sval), flags2str (tval));
  return sval;
}

/* Don't duplicate the code for actually updating the value */
void Cell::update_str_val ()
{
  char s[100];
  double dtemp;
  if (modf (fval, &dtemp) == 0)  /* it's integral */
    sprintf (s, "%.30g", fval);
  else
    sprintf (s, fmt, fval);
  delete sval;
  sval = tostring (s);
  //  vp->tval |= STR;
}

