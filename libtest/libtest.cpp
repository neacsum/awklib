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
  }

  void setup (const char *testfname);
  void makefiles ();
  AWKINTERP *interp;
  FILE *tst, *in, *out, *prog, *ref;
};

void awk_tester::setup (const char *testfile)
{
  char *testname, fname[FILENAME_MAX];

  CHECK (tst = fopen (testfile, "r"));

  testname = basename (testfile);
  strcpy (fname, testname);
  strcat (fname, ".awk");
  CHECK (prog = fopen (fname, "w"));

  strcpy (fname, testname);
  strcat (fname, ".in");
  CHECK (in = fopen (fname, "w"));

  strcpy (fname, testname);
  strcat (fname, ".ref");
  CHECK (ref = fopen (fname, "w"));

  makefiles ();
  xclose (tst);
  xclose (in);
  xclose (prog);
  xclose (ref);

  strcpy (fname, testname);
  strcat (fname, ".awk");
  CHECK (awk_addprogfile (interp, fname));
  CHECK (awk_compile (interp));

  strcpy (fname, testname);
  strcat (fname, ".in");
  CHECK (awk_addarg (interp, fname));

  strcpy (fname, testname);
  strcat (fname, ".out");
  CHECK (awk_setoutput (interp, fname));

  CHECK (awk_exec (interp));
}

void awk_tester::makefiles ()
{
  int step = 0;
  char line[1024];
  FILE *out = prog;
  while (fgets (line, sizeof (line), tst))
  {
    if (line[0] == '#' && line[1] == '#')
    {
      switch (step++)
      {
      case 0: // end of program; beginning of input data
        out = in; break;

      case 1: // end of input data; beginning of reference output
        out = ref; break;

      case 2: //The End
        return;

      default:
        CHECK_EX (1, "Too many sections in test file");
        return;
      }
    }
    else
      fputs (line, out);
  }
  return;
}


TEST_FIXTURE (awk_tester, T1)
{
  setup ("..\\testdir\\tests\\1_printall.tst");
  CHECK_FILE_EQUAL ("1_printall.out", "1_printall.ref");

  system ("del 1_printall.*");
}

TEST_FIXTURE (awk_tester, T63)
{
  setup ("..\\testdir\\tests\\63_fun.tst");
  CHECK_FILE_EQUAL ("63_fun.out", "63_fun.ref");

  system ("del 63_fun.*");
}


int main (int argc, char **argv)
{
  int ret;

  awk_setdebug (3);
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


  ret = UnitTest::RunAllTests ();

  return ret;
}