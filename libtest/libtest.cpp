#include <awklib.h>
#include <strstream>
#include <sstream>
#include <utpp/utpp.h>

using namespace std;
istrstream instr{
  "Record 1\n"
  "Record 2\n"
};

ostringstream out;

const char *prog =
  "{print NR, $0}\n"
  "END {print \"Total records=\", NR}"
;

//Test fixture
struct fixt {
  AWKINTERP *interp;
  fixt () : interp{ awk_init (NULL) } {}
  ~fixt () { awk_end (interp); }
};

TEST_FIXTURE (fixt, getvar)
{
  awk_setprog (interp, "{print NR, $0}\n");
  awk_compile (interp);
  awksymb v{ "NR" };
  CHECK (awk_getvar (interp, &v));
  CHECK (v.flags & AWKSYMB_NUM);
  CHECK_EQUAL (0, v.fval);
}

TEST_FIXTURE (fixt, getvar_array)
{
  awk_setprog (interp, "{print NR, $0}\n");
  awk_compile (interp);
  awksymb v{ "ENVIRON", "Path" };
  CHECK (awk_getvar (interp, &v));
  CHECK (v.flags & AWKSYMB_STR);
  free (v.sval);
}

TEST_FIXTURE (fixt, setvar_newvar)
{
  awksymb v{ "myvar", NULL, AWKSYMB_NUM, 25.0 };
  awk_setprog (interp, "{myvar++; print myvar, $0}\n");
  awk_compile (interp);
  awk_inredir (interp, []()->int {return instr.get (); });
  CHECK (awk_setvar (interp, &v));

  instr.clear ();
  instr.seekg (0);
  awk_exec (interp);
  awksymb vo{ "myvar"};
  CHECK (awk_getvar (interp, &vo));
  CHECK_EQUAL (27, vo.fval);
}

TEST_FIXTURE (fixt, setvar_existingvar)
{
  awksymb v{ "myvar", NULL, AWKSYMB_NUM, 25.0 };
  awk_setprog (interp, "{myvar++; print myvar, $0}\n");
  awk_compile (interp);
  awk_inredir (interp, []()->int {return instr.get (); });
  CHECK (awk_setvar (interp, &v));

  instr.clear ();
  instr.seekg (0);

  v.fval = 35;
  CHECK (awk_setvar (interp, &v));
  awk_exec (interp);
  awksymb vo{ "myvar" };
  CHECK (awk_getvar (interp, &vo));
  CHECK_EQUAL (37, vo.fval);
}

TEST_FIXTURE (fixt, setvar_newarray)
{
  awksymb v{ "myarray", "index", AWKSYMB_ARR | AWKSYMB_NUM, 25.0 };
  awk_setprog (interp, "END {print \"at end: \" myarray[\"index\"]}\n");
  awk_compile (interp);
  awk_inredir (interp, []()->int {return instr.get (); });
  CHECK (awk_setvar (interp, &v));

  instr.clear ();
  instr.seekg (0);
  awk_exec (interp);
  awksymb vo{ "myarray", "index" };
  CHECK (awk_getvar (interp, &vo));
  CHECK_EQUAL (25, vo.fval);
}

int main (int argc, char **argv)
{
  int ret;

  awk_setdebug (3);
  AWKINTERP *interp = awk_init (NULL);
  if (awk_setprog (interp, prog)
   && awk_compile (interp))
  {
    awk_inredir (interp, [] ()->int {return instr.get (); });
    awk_outredir (interp, [](const char *buf, size_t sz)->int {out.write (buf, sz); return sz; });
//    awk_addarg (interp, "test.data");
    ret = awk_exec (interp);
  }
  printf ("Output:\n");
  puts (out.str ().c_str ());
  awk_end (interp);

  UnitTest::RunAllTests ();

  return 0;
}