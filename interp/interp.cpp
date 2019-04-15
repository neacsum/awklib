#include <awklib.h>
#include <stdlib.h>
#include <string.h>

const char *version = "AWKLIB Interpreter " __DATE__ "\n";

int main (int argc, char **argv)
{
  const char **vars = (const char**)calloc (argc, sizeof (char*));
  const char **progs = (const char **)calloc (argc, sizeof (char*));
  const char **args = (const char **)calloc (argc, sizeof (char*));
  int nv = 0;
  int np = 0;
  int na = 0;
  char *fs = 0;

  //parse command line
  for (int i = 1; i < argc; i++)
  {
    if (*argv[i] == '-' && *(argv[i]+1) == 'v')
    {
      if (strlen (argv[i]) > 2)
        vars[nv] = argv[i] + 2;
      else if (i + 1 < argc)
        vars[nv] = argv[++i];
      else
      {
        fprintf (stderr, "Invalid last command line argument (-v)\n");
        exit (1);
      }
      nv++;
    }
    else if (*argv[i] == '-' && *(argv[i] + 1) == 'f')
    {
      if (strlen (argv[i]) > 2)
        progs[np] = argv[i] + 2;
      else if (i + 1 < argc)
        progs[np] = argv[++i];
      else
      {
        fprintf (stderr, "Invalid last command line argument (-f)\n");
        exit (1);
      }
      np++;
    }
    else if (*argv[i] == '-' && *(argv[i] + 1) == 'F')
    {
      if (strlen (argv[i]) > 2)
        fs = argv[i] + 2;
      else if (i + 1 < argc)
        fs = argv[++i];
      else
      {
        fprintf (stderr, "Invalid last command line argument (-F)\n");
        exit (1);
      }
    }
    else if (*argv[i] == '-' && *(argv[i] + 1) == '-')
    {
      if (!strcmp (argv[i] + 2, "version"))
      {
        fprintf (stderr, version);
        exit (0);
      }

    }
    else
      args[na++] = argv[i];
  }

  awk_setdebug (1);
  // initialize interpreter
  AWKINTERP* interp = awk_init (vars);

  // add all programs
  for (int i = 0; i < np; i++)
    awk_addprogfile (interp, progs[i]);

  // add all arguments
  for (int i = 0; i < na; i++)
    awk_addarg (interp, args[i]);

  // compile programs
  if (!awk_compile (interp))
  {
    const char *msg;
    int err = awk_err (&msg);
    fprintf (stderr, "Error %d - %s\n", err, msg);
    exit (1);
  }

  // run program
  if (!awk_exec (interp))
  {
    const char *msg;
    int err = awk_err (&msg);
    fprintf (stderr, "Error %d - %s\n", err, msg);
    exit (1);
  }

  // cleanup
  awk_end (interp);

  return 0;
}