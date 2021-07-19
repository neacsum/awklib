#include <stdlib.h>
#include <string.h>
#include <awklib/err.h>

#include "awk.h"
#include "proto.h"

#define  FULLTAB  2 /* rehash when table gets this x full */
#define  GROWTAB  4 /* grow table by this factor */

static int  hash (const char*, int);


/// Make a new symbol table
Array::Array (int n)
  : nelem (0)
  , size (n)
  , tab{ (Cell**)calloc (n, sizeof (Cell*)) }
{
  if (tab == NULL)
    FATAL (AWK_ERR_NOMEM, "out of space in makearray");
}

/// Free a hash table
Array::~Array ()
{
  Cell* cp, * temp;
  int i;

  for (i = 0; i < size; i++)
  {
    for (cp = tab[i]; cp != NULL; cp = temp)
    {
      dprintf ("freeing %s...", cp->nval);
      if (cp->isarr ())
      {
        dprintf ("...cell is an array\n");
        delete cp->arrval;
      }
      else if (cp->tval & (FCN | EXTFUN))
      {
        dprintf ("... skipping function\n");
      }
      else
      {
        dprintf (" (value=\"%s\")\n", quote (cp->sval));
        delete cp->sval;
      }
      cp->sval = 0;
      temp = cp->cnext;  /* avoids freeing then using */
      delete cp;
      nelem--;
    }
    tab[i] = 0;
  }
  if (nelem != 0)
    FATAL (AWK_ERR_OTHER, "inconsistent element count freeing array");
  free (tab);
}

Cell* Array::setsym (const char* n, const char* s, double f, unsigned int t)
{
  int h;
  Cell* p;

  if (n != NULL && (p = lookup (n)) != NULL)
  {
    dprintf ("setsym found n=%s s=<%s> f=%g t=%s\n",
      NN (p->nval), quote (p->sval), p->fval, flags2str (p->tval));
    return p;
  }
  p = new Cell (Cell::type::OCELL, Cell::subtype::CNONE, n, s ? tostring (s) : 0, f, t);

  nelem++;
  if (nelem > FULLTAB * size)
    rehash ();
  h = hash (n, size);
  p->cnext = tab[h];
  tab[h] = p;
  dprintf ("setsym set n=%s s=<%s> f=%g t=%s\n",
    p->nval, quote (p->sval), p->fval, flags2str (p->tval));
  return p;
}

/// Unchain an element from an array
Cell* Array::removesym (const char* n)
{
  int h = hash (n, size);
  Cell* p, * prev = NULL;
  for (p = tab[h]; p != NULL; prev = p, p = p->cnext)
  {
    if (strcmp (n, p->nval) == 0)
    {
      if (prev == NULL)  /* 1st one */
        tab[h] = p->cnext;
      else      /* middle somewhere */
        prev->cnext = p->cnext;
      nelem--;
      return p;
    }
  }
  return 0;
}

/// Form hash value for string s
int hash (const char* s, int n)
{
  unsigned hashval;

  for (hashval = 0; *s != '\0'; s++)
    hashval = (*s + 31 * hashval);
  return hashval % n;
}

///  Rehash items in small table into big one
void Array::rehash ()
{
  int i, nh, nsz;
  Cell* cp, * op, ** np;

  nsz = GROWTAB * size;
  np = (Cell**)calloc (nsz, sizeof (Cell*));
  if (np == NULL)    /* can't do it, but can keep running. */
    return;    /* someone else will run out later. */
  for (i = 0; i < size; i++)
  {
    for (cp = tab[i]; cp; cp = op)
    {
      op = cp->cnext;
      nh = hash (cp->nval, nsz);
      cp->cnext = np[nh];
      np[nh] = cp;
    }
  }
  free (tab);
  tab = np;
  size = nsz;
}

/// Look for s in tp
Cell* Array::lookup (const char* s)
{
  Cell* p;
  int h;

  h = hash (s, size);
  for (p = tab[h]; p != NULL; p = p->cnext)
    if (strcmp (s, p->nval) == 0)
      return p;   /* found it */
  return NULL;    /* not found */
}
