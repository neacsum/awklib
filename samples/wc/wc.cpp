#include <awklib.h>

int main (int argc, char **argv)
{
  AWKINTERP* interp = awk_init (NULL);
  awk_setprog (interp,
    "{wc += NF; bc += length($0)}\n"
    "END {print NR, wc, bc, ARGV[1]}");
  awk_compile (interp);
  if (argc > 1)
    awk_addarg (interp, argv[1]);
  awk_exec (interp);
  awk_end (interp);
}
