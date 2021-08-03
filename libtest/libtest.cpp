#define _CRT_SECURE_NO_WARNINGS
#include <awklib/awk.h>
#include <sstream>
#include <utpp/utpp.h>
#include <sys/stat.h>

using namespace std;

#ifdef _WIN32
const char* basename (const char* path)
{
  static char buf[FILENAME_MAX];
  char* ptr;

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
#endif

/*
  Separate a test file in components.

  Each test file contains a AWK program, an input file and an expected results file.
  Components are separated by a line starting with '##'. This function creates
  three files:
   - *.awk - the program text
   - *.in  - input data
   - *.ref - reference output

*/
bool setup_test (const string& testfile)
{
  char buf[1024];
  const char* name = basename (testfile.c_str());
  FILE* in;
  if (!(in = fopen (testfile.c_str(), "r")))
    return false;

  char fname[FILENAME_MAX];
  strcpy (fname, name);
  strcat (fname, ".awk");
  FILE* output = fopen (fname, "w");
  if (!output)
    return false;

  int step = 0;
  while (fgets (buf, sizeof (buf), in))
  {
    if (buf[0] == '#' && buf[1] == '#')
    {
      fclose (output);
      output = 0;
      if (step > 1)
        break;
      strcpy (fname, name);
      strcat (fname, step? ".ref" : ".in");
      output = fopen (fname, "w");
      if (!output)
        return false;
      ++step;
    }
    else
      fputs (buf, output);
  }
  if (output)
    fclose (output);
  return (step > 1);
}

const char *dat{
  "Record 1\n"
  "Record 2\n"
};

istringstream instr;

ostringstream out;

const char *prog =
  "{print NR, $0}\n"
  "END {print \"Total records=\", NR}"
;

#define REDO_TEST

#define TESTNAME "18_array"
TEST (failing)
{
  ABORT (setup_test ("../testdir/tests/" TESTNAME ".tst"));

  AWKINTERP* interp = awk_init (NULL);
  CHECK (awk_addprogfile (interp, TESTNAME ".awk"));
  CHECK (awk_compile (interp));
  CHECK (awk_setinput (interp, TESTNAME ".in"));
  CHECK (awk_setoutput (interp, TESTNAME ".out"));
  CHECK (awk_exec (interp) >= 0);
  CHECK_FILE_EQUAL (TESTNAME ".ref",  TESTNAME ".out");
#ifdef REDO_TEST
  CHECK (awk_exec (interp) >= 0);
  CHECK_FILE_EQUAL (TESTNAME ".ref", TESTNAME ".out");
#endif
  awk_end (interp);
  remove (TESTNAME ".awk");
  remove (TESTNAME ".in");
  remove (TESTNAME ".ref");
  remove (TESTNAME ".out");
}

//Test fixture
struct fixt {
  AWKINTERP *interp;
  fixt () : interp{ awk_init (NULL) } {}
  ~fixt () { awk_end (interp); }
};

int strout (const char *buf, size_t sz)
{
  out.write (buf, sz);
  return out.bad ()? - 1 : 1;
}

TEST_FIXTURE (fixt, outredir)
{
  awk_setprog (interp, "BEGIN {print \"Output redirected\"}");
  awk_compile (interp);
  awk_outfunc (interp, strout);
  awk_exec (interp);
  CHECK_EQUAL ("Output redirected\n", out.str ());
}

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
  awksymb v{ "ENVIRON", 
#ifdef _WIN32
    "Path"
#else
    "PATH" 
#endif
  };
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

  instr.str (dat);
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

void fact (AWKINTERP *pinter, awksymb* ret, int nargs, awksymb* args)
{
  int prod = 1;
  for (int i = 2; i <= args[0].fval; i++)
    prod *= i;
  ret->fval = prod;
  ret->flags = AWKSYMB_NUM;
}

TEST_FIXTURE (fixt, extfun)
{
  awk_setprog (interp, " BEGIN {n = factorial(3); print \"factorial(\" 3 \")=\", n}");
  awk_compile (interp);
  awk_addfunc (interp, "factorial", fact, 1);
  awk_exec (interp);
  awksymb n{ "n" };
  awk_getvar (interp, &n);
  CHECK_EQUAL (6, n.fval);
}

void change_rec (AWKINTERP *pinter, awksymb* ret, int nargs, awksymb* args)
{
  awksymb rec{ "$0" };
  printf ("change_rec called\n");

  if (nargs >= 1)
  {
    rec.sval = args[0].sval;
    rec.flags |= AWKSYMB_STR;
    awk_setvar (pinter, &rec);
  }
}

istringstream instr1 (dat);

TEST_FIXTURE (fixt, recset)
{
  awk_setprog (interp, "NR == 1 {change(\"This is \" $0); print}");
  awk_compile (interp);
  awk_addfunc (interp, "change", change_rec, 1);

  awk_infunc (interp, []()->int {return instr1.get (); });
  out.str (std::string ());

  awk_outfunc (interp, strout);
  awk_exec (interp);
  CHECK_EQUAL ("This is Record 1\n", out.str ());
}

void add1 (AWKINTERP *pinter, awksymb* ret, int nargs, awksymb* args)
{
  awksymb fld{ "$2" };
  awk_getvar (pinter, &fld);

  ret->fval = fld.fval + 1;
  ret->flags = AWKSYMB_NUM;
}

TEST_FIXTURE (fixt, fldget)
{
  awk_setprog (interp, "{print add1()}");
  awk_compile (interp);
  awk_addfunc (interp, "add1", add1, 1);

  instr.str (dat);
  instr.clear ();
  awk_infunc (interp, []()->int {return instr.get (); });

  out.str (string ());
  awk_outfunc (interp, strout);
  awk_exec (interp);
  CHECK_EQUAL ("2\n3\n", out.str ());
}

TEST_FIXTURE (fixt, do_run)
{
  awk_addfunc (interp, "add1", add1, 1);

  instr.str (dat);
  instr.clear ();
  awk_infunc (interp, []()->int {return instr.get (); });

  out.str (string ());
  awk_outfunc (interp, strout);

  CHECK_EQUAL (0, awk_run (interp, "{print add1()}"));
  CHECK_EQUAL ("2\n3\n", out.str ());
}


SUITE (one_true_awk)
{

struct awk_frame {
  awk_frame ();
  ~awk_frame ();
  bool setup (const string& test_name);
  string name;
  AWKINTERP* interp;
};

awk_frame::awk_frame ()
  : interp (awk_init (NULL))
{
}

bool awk_frame::setup (const string& test_name)
{
  name = test_name;
  if (!setup_test ("../testdir/tests/" + name + ".tst"))
    return false;
  CHECK (awk_addprogfile (interp, (name + ".awk").c_str()));
  CHECK (awk_compile (interp));
  CHECK (awk_addarg (interp, (name + ".in").c_str ()));
  CHECK (awk_setoutput (interp, (name + ".out").c_str()));
  return true;
}

awk_frame::~awk_frame ()
{
  remove ((name + ".awk").c_str());
  remove ((name + ".in").c_str());
  remove ((name + ".out").c_str());
  remove ((name + ".ref").c_str());
  awk_end (interp);
}

#define AWK_TEST(A) \
  TEST_FIXTURE (awk_frame, _##A)\
  {\
    ABORT (setup (#A));\
    CHECK (awk_exec (interp) >= 0);\
    CHECK_FILE_EQUAL (#A ".out", #A ".ref");\
    CHECK (awk_exec (interp) >= 0);\
    CHECK_FILE_EQUAL (#A ".out", #A ".ref"); \
  }

  AWK_TEST (1_printall);
  AWK_TEST (2_printnr);
  AWK_TEST (3_changefs);
  AWK_TEST (4_changeofs);
  AWK_TEST (5_concat);
  AWK_TEST (6_fieldassign);
  AWK_TEST (7_select);
  AWK_TEST (8_while);
  AWK_TEST (9_patmatch);
  AWK_TEST (10_field_expression);
  AWK_TEST (11_field_change);
  AWK_TEST (12_regexp_pattern);
  AWK_TEST (13_nf);
  AWK_TEST (14_forloop);
  AWK_TEST (15_diveq);
  AWK_TEST (16_field_change1);
  AWK_TEST (17_field_change2);
  AWK_TEST (18_array);
  AWK_TEST (19_array);
  AWK_TEST (20_array);
  AWK_TEST (21_array);
  AWK_TEST (22_arith);
  AWK_TEST (23_arith);
  AWK_TEST (24_regexp_pattern);
  AWK_TEST (25_assert);
  AWK_TEST (25_rgexp_pattern);
  AWK_TEST (26_avg);
  AWK_TEST (27_field_change);
  AWK_TEST (28_filename);
  AWK_TEST (29_beginexit);
  AWK_TEST (30_beginnext);
  AWK_TEST (31_break);
  AWK_TEST (32_break);
  AWK_TEST (33_break);
  AWK_TEST (34_missing_field);
  AWK_TEST (35_builtins);
  AWK_TEST (36_cat);
  AWK_TEST (37_cat);
  AWK_TEST (38_cat);
  AWK_TEST (39_cmp);
  AWK_TEST (40_coerce);
  AWK_TEST (41_coerce);
  AWK_TEST (42_comment);
  AWK_TEST (43_comment);
  AWK_TEST (44_concat);
  AWK_TEST (45_cond);
  AWK_TEST (46_contin);
  AWK_TEST (47_count);
  AWK_TEST (48_crlf);
  AWK_TEST (49_acum);
  AWK_TEST (50_fsofs);
  AWK_TEST (51_delete);
  AWK_TEST (52_delete);
  AWK_TEST (53_delete);
  AWK_TEST (54_delete);
  AWK_TEST (55_do);
  AWK_TEST (56_or_expr);
  AWK_TEST (57_else);
  AWK_TEST (58_exit);
  AWK_TEST (59_exit);
  AWK_TEST (63_fun);
  AWK_TEST (64_funarg);
  TEST_FIXTURE (awk_frame, _65_funackermann)
  {
    ABORT (setup ("65_funackermann"));
    int d = awk_setdebug (0);
    CHECK (awk_exec (interp) >= 0);
    CHECK_FILE_EQUAL ("65_funackermann.out", "65_funackermann.ref");
    CHECK (awk_exec (interp) >= 0);
    CHECK_FILE_EQUAL ("65_funackermann.out", "65_funackermann.ref");
    awk_setdebug (d);
  };
  AWK_TEST (66_expconv);
  AWK_TEST (67_for);
  AWK_TEST (68_for);
  AWK_TEST (69_for);
  AWK_TEST (70_for);
  AWK_TEST (71_sqrt);
  AWK_TEST (72_expr);
  AWK_TEST (73_match);
  AWK_TEST (74_func);
  AWK_TEST (75_func);
  AWK_TEST (76_func);
  AWK_TEST (77_func);
  AWK_TEST (78_func);
  AWK_TEST (80_format4);
  AWK_TEST (81_func);
  AWK_TEST (82_func);
  AWK_TEST (83_func);
  AWK_TEST (84_func);
  AWK_TEST (85_func);
  AWK_TEST (86_factorial);
  AWK_TEST (87_fibbo);
  AWK_TEST (88_multidim);
  AWK_TEST (89_func);
  AWK_TEST (90_func);
  AWK_TEST (91_func);
  AWK_TEST (92_func);
  AWK_TEST (93_vf);
  AWK_TEST (94_vf);
  AWK_TEST (95_vf);
  AWK_TEST (96_vf);
  AWK_TEST (97_set);
  AWK_TEST (98_set);
  AWK_TEST (99_set);
}

int main (int argc, char **argv)
{
  int ret;
  awk_setdebug (2);

  ret = UnitTest::RunAllTests ();

  return ret;
}