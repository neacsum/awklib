#include <stdio.h>
#include <string.h>

int main (int argc, char *argv[])
{
  int i, start, minusn;

  start = 1;
  minusn = 0;
  if (argc > 1 && strcmp (argv[1], "-n") == 0)
  {
    start = 2;
    minusn = 1;
  }

  for (i = start; i < argc; i++)
  {
    printf ("%s", argv[i]);
    if (i != argc - 1)
      putchar (' ');
  }
  if (minusn == 0)
    putchar ('\n');
}
