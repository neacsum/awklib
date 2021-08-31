#include <stdlib.h>
#include <string.h>
#include <awklib/err.h>

#include "awk.h"
#include "proto.h"

#define  FULLTAB  2 /* rehash when table gets this x full */
#define  GROWTAB  4 /* grow table by this factor */

static int  hash (const char*, int);
extern Interpreter* interp;

/// Make a new symbol table
Array::Array (int n)
  : nelem (0)
  , sz (n)
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

  for (i = 0; i < sz; i++)
  {
    for (cp = tab[i]; cp != NULL; cp = temp)
    {
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
  Cell* p;
  assert (n != NULL);
  if ((p = lookup (n)) == NULL)
  {
    p = insert_sym (n);
    p->fval = f;
    p->flags = t;
    if (s)
      p->sval = s;
    else
      p->sval.clear ();
#ifndef  NDEBUG
    dprintf ("setsym added n=%s t=%s",p->nval.c_str() , flags2str (p->flags));
    if ((p->flags & NUM) != 0)
      dprintf (" f=%g", p->fval);
    if ((p->flags & STR) != 0)
      dprintf (" s=<%s>", quote (p->sval).c_str ());
    dprintf ("\n");
#endif
  }
  else
    dprintf ("setsym found n=%s t=%s\n", p->nval.c_str(), flags2str (p->flags));
  return p;
}

/// Add an array as element to this array.
/// n - array name
/// arry - element to be added
/// addl_flags - flags OR'ed to existing ones
Cell* Array::setsym (const char* n, Array* arry, int addl_flags)
{
  Cell* p;
  if (n != NULL && (p = lookup (n)) != NULL)
    dprintf ("setsym found %s", n);
  else
  {
    p = insert_sym (n);
    p->flags = ARR;
    p->arrval = arry;
  }
  p->flags |= addl_flags;
  return p;
}

Cell* Array::insert_sym (const char* n)
{
  int h;

  dprintf ("Inserting symbol %s\n");
  Cell *p = new Cell (n);

  nelem++;
  if (nelem > FULLTAB * sz)
    rehash ();
  h = hash (n, sz);
  p->cnext = tab[h];
  tab[h] = p;
  return p;
}

/// Unchain an element from array
Cell* Array::removesym (const std::string& n)
{
  int h = hash (n.c_str(), sz);
  Cell* p, * prev = NULL;
  for (p = tab[h]; p != NULL; prev = p, p = p->cnext)
  {
    if (n == p->nval)
    {
      if (prev == NULL)  /* 1st one */
        tab[h] = p->cnext;
      else      /* middle somewhere */
        prev->cnext = p->cnext;
      --nelem;
      return p;
    }
  }
  return 0;
}

/// Delete an element pointed by an iterator
void Array::deletecell (Iterator& p)
{
  if (p.owner != this || p.ipos >= sz)
    throw awk_exception (*interp, AWK_ERR_OTHER, "Invalid iterator in delete_cell");

  Cell *cp = tab[p.ipos];
  Cell* prev = 0;
  while (cp && cp != p.ptr)
  {
    prev = cp;
    cp = cp->cnext;
  }
  if (!cp)
    throw awk_exception (*interp, AWK_ERR_OTHER, "Invalid iterator in delete_cell");

  if (prev)
    prev->cnext = cp->cnext;
  else
    tab[p.ipos] = cp->cnext;

  //advance iterator to next valid position
  p.ptr = cp->cnext;
  while (p.ptr == 0 && p.ipos < sz)
  {
    p.ipos++;
    p.ptr = tab[p.ipos];
  }

  delete cp;
  --nelem;
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

  nsz = GROWTAB * sz;
  np = (Cell**)calloc (nsz, sizeof (Cell*));
  if (np == NULL)    /* can't do it, but can keep running. */
    return;    /* someone else will run out later. */
  for (i = 0; i < sz; i++)
  {
    for (cp = tab[i]; cp; cp = op)
    {
      op = cp->cnext;
      nh = hash (cp->nval.c_str(), nsz);
      cp->cnext = np[nh];
      np[nh] = cp;
    }
  }
  free (tab);
  tab = np;
  sz = nsz;
}

/// Look for s in tp
Cell* Array::lookup (const char* s)
{
  Cell* p;
  int h;

  h = hash (s, sz);
  std::string ss (s);
  for (p = tab[h]; p != NULL; p = p->cnext)
    if (ss == p->nval)
      return p;   /* found it */
  return NULL;    /* not found */
}

#ifndef NDEBUG
void Array::print ()
{
  for (auto p : *this)
    print_cell (p, 0);
}
#endif

Array::Iterator Array::begin () const
{
  Iterator it(this);
  while (it.ipos < sz && tab[it.ipos] == 0)
    it.ipos++;
  if (it.ipos < sz)
    it.ptr = tab[it.ipos];
  return it;
}

Array::Iterator Array::end () const
{
  Iterator it (this);
  it.ipos = sz;
  return it;
}


Array::Iterator::Iterator (const Array* a)
  : owner{a}
  , ipos{ 0 }
  , ptr{ 0 }
{
}


/// Post-increment operator 
Array::Iterator Array::Iterator::operator++ (int)
{
  Iterator me = *this;
  if (ptr)
    ptr = ptr->cnext;
  if (!ptr)
  {
    while (++ipos < owner->sz && owner->tab[ipos] == 0)
      ;
    if (ipos < owner->sz)
      ptr = owner->tab[ipos];
    else
      ptr = 0;
  }
  return me;
}

/// Pre-increment operator 
Array::Iterator& Array::Iterator::operator++ ()
{
  if (ptr)
    ptr = ptr->cnext;
  if (!ptr)
  {
    while (++ipos < owner->sz && owner->tab[ipos] == 0)
      ;
    if (ipos < owner->sz)
      ptr = owner->tab[ipos];
    else
      ptr = 0;
  }
  return *this;
}

Cell* Array::Iterator::operator* ()
{
  if (ptr)
    return ptr;

  FATAL (AWK_ERR_OTHER, "Dereferencing invalid array iterator");
  return 0;
}

bool Array::Iterator::operator==(const Iterator& other) const
{
  return (owner == other.owner) && (ipos == other.ipos) && (ptr == other.ptr);
}

