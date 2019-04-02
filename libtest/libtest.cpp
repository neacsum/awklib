#define _CRT_SECURE_NO_WARNINGS
#include <awklib.h>
#include <strstream>
#include <sstream>
#include <utpp/utpp.h>
#include <sys/stat.h>

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
  awk_infunc (interp, []()->int {return instr.get (); });
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
  awk_infunc (interp, []()->int {return instr.get (); });
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
  awk_infunc (interp, []()->int {return instr.get (); });
  CHECK (awk_setvar (interp, &v));

  v.fval = 35;
  CHECK (awk_setvar (interp, &v));

  instr.clear ();
  instr.seekg (0);
  awk_exec (interp);
  awksymb vo{ "myarray", "index" };
  CHECK (awk_getvar (interp, &vo));
  CHECK_EQUAL (35, vo.fval);
}

char *basename (const char *path)
{
  static char buf[FILENAME_MAX];
  char *ptr;

  strncpy (buf, path, sizeof (buf) - 1);
  buf[sizeof (buf) - 1] = 0;
  for (ptr = buf + strlen (buf); ptr > buf; ptr--)
  {
    if (*ptr == '\\' || *ptr == '/')
      return ptr + 1;
    if (*ptr == '.')
      *ptr = 0;
  }
  return ptr;
}


#define xclose(f) { if (f) {fclose (f); f = 0;} }
struct awk_tester
{
public:
  awk_tester () :
    interp (awk_init(NULL)),
    in (0), out(0), prog(0), ref(0)
  {
  }

  ~awk_tester ()
  {
    awk_end (interp);
    xclose (tst);
    xclose (in);
    xclose (out);
    xclose (prog);
    xclose (ref);
    cleanup ();
  }

  void setup (const char *testfname);
  void makefiles ();
  void cleanup ();

  AWKINTERP *interp;
  string progname, inname, refname, outname;
  FILE *tst, *in, *out, *prog, *ref;
};

void awk_tester::setup (const char *testfile)
{
  char *testname;

  CHECK (tst = fopen (testfile, "r"));

  testname = basename (testfile);
  progname = testname + string(".awk");
  CHECK (prog = fopen (progname.c_str(), "w"));

  inname = testname + string (".in");
  CHECK (in = fopen (inname.c_str(), "w"));

  refname = testname + string (".ref");
  CHECK (ref = fopen (refname.c_str(), "w"));

  outname = testname + string (".out");

  makefiles ();
  xclose (tst);
  xclose (in);
  xclose (prog);
  xclose (ref);

  CHECK (awk_addprogfile (interp, progname.c_str()));
  CHECK (awk_compile (interp));
  CHECK (awk_addarg (interp, inname.c_str()));
  CHECK (awk_setoutput (interp, outname.c_str()));
  CHECK (awk_exec (interp));
}

void awk_tester::makefiles ()
{
  int step = 0;
  char line[1024];
  FILE *output = prog;
  while (fgets (line, sizeof (line), tst))
  {
    if (line[0] == '#' && line[1] == '#')
    {
      switch (step++)
      {
      case 0: // end of program; beginning of input data
        output = in; break;

      case 1: // end of input data; beginning of reference output
        output = ref; break;

      case 2: //The End
        return;

      default:
        CHECK_EX (1, "Too many sections in test file");
        return;
      }
    }
    else
      fputs (line, output);
  }
  return;
}

void awk_tester::cleanup ()
{
  remove (progname.c_str());
  remove (inname.c_str());
  remove (refname.c_str());
  remove (outname.c_str());
}

TEST_FIXTURE (awk_tester, T1)
{
  setup ("..\\testdir\\tests\\1_printall.tst");
  CHECK_FILE_EQUAL ("1_printall.out", "1_printall.ref");
}

TEST_FIXTURE (awk_tester, T63)
{
  setup ("..\\testdir\\tests\\63_fun.tst");
  CHECK_FILE_EQUAL ("63_fun.out", "63_fun.ref");
}

TEST_FIXTURE (awk_tester, T64)
{
  setup ("..\\testdir\\tests\\64_funarg.tst");
  CHECK_FILE_EQUAL ("64_funarg.out", "64_funarg.ref");
}

TEST_FIXTURE (awk_tester, T65)
{
  int lvl = awk_setdebug (0);
  UnitTest::Timer t;
  t.Start ();
  setup ("..\\testdir\\tests\\65_funackermann.tst");
  int ms = t.GetTimeInMs ();
  printf ("Ackermann test done in %d msec\n", ms);
  CHECK_FILE_EQUAL ("65_funackermann.out", "65_funackermann.ref");
  awk_setdebug (lvl);
}

int main (int argc, char **argv)
{
  int ret;
  awk_setdebug (3);
#if 0
  AWKINTERP *interp = awk_init (NULL);
  if (awk_setprog (interp, prog)
   && awk_compile (interp))
  {
    awk_infunc (interp, [] ()->int {return instr.get (); });
    awk_outfunc (interp, [](const char *buf, size_t sz)->int {out.write (buf, sz); return sz; });
//    awk_addarg (interp, "test.data");
    ret = awk_exec (interp);
  }
  printf ("Output:\n");
  puts (out.str ().c_str ());
  awk_end (interp);

#endif

  ret = UnitTest::RunAllTests ();

  return ret;
}