#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *basename (const char *path)
{
  static char buf[FILENAME_MAX];
  char *ptr;

  strncpy (buf, path, sizeof (buf)-1);
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

int main (int argc, char *argv[])
{
  char *testname, oname[FILENAME_MAX], line[256];
  FILE *in, *out;
  int step;

  if (argc < 2)
  {
    fprintf (stderr, "Usage: maketest <test file>\n");
    exit (1);
  }

  if (!(in = fopen (argv[1], "r")))
  {
    perror ("Cannot open input file");
    exit (1);
  }

  testname = basename (argv[1]);
  strcpy (oname, testname);
  strcat (oname, ".awk");

  if (!(out = fopen (oname, "w")))
  {
    perror ("Cannot create program file");
    fclose (in);
    exit (1);
  }

  step = 0;
  while (fgets (line, sizeof (line), in))
  {
    if (line[0] == '#' && line[1] == '#')
    {
      fclose (out);
      switch (step++)
      {
      case 0:
        // end of program; beginning of input data
        strcpy (oname, testname);
        strcat (oname, ".in");
        if (!(out = fopen (oname, "w")))
        {
          perror ("Cannot create test input data file");
          fclose (in);
          exit (1);
        }
        break;

      case 1:
        // end of input data; beginning of reference output
        strcpy (oname, testname);
        strcat (oname, ".ref");
        if (!(out = fopen (oname, "w")))
        {
          perror ("Cannot create test reference output file");
          fclose (in);
          exit (1);
        }
        break;

      case 2:
        //The End
        fclose (in);
        fclose (out);
        exit (0);

      default:
        fprintf (stderr, "Too many sections in test file");
        fclose (in);
        fclose (out);
        exit (1);
      }
    }
    else
      fputs (line, out);
  }
  fclose (in);
  fclose (out);
  exit (0);
}