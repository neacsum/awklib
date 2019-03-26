#pragma once

#include <stdio.h>
#pragma comment (lib, "awk")
#ifdef __cplusplus
extern "C" {
#endif
struct AWKINTERP;
typedef int (*inproc)();
typedef int (*outproc)(const char *buf, size_t len);

struct awksymb {
  const char *name;
  const char *index;

  unsigned int flags;   //variable type flags
#define AWKSYMB_NUM   1   //numeric value
#define AWKSYMB_STR   2   //string value
#define AWKSYMB_ARR   4   //variable is an array
#define AWKSYMB_INV   8   //invalid variable type
  double fval;
  char *sval;

};

typedef int (*awkfunc)(AWKINTERP *pinter, awksymb *ret, int nargs, ...);

AWKINTERP* awk_init (const char **vars);
int awk_err (const char **msg);
int awk_setprog (AWKINTERP* pinter, const char *prog);
int awk_addprogfile (AWKINTERP* pinter, const char *progfile);
int awk_compile (AWKINTERP* pinter);
int awk_addarg (AWKINTERP* pinter, const char *arg);
int awk_exec (AWKINTERP* pinter);
void awk_end (AWKINTERP* pinter);
void awk_setdebug (int level);
void awk_inredir (AWKINTERP* pinter, inproc user_input);
void awk_outredir (AWKINTERP* pinter, outproc user_output);
void awk_addfunc (AWKINTERP *pinter, awkfunc fn);
int awk_getvartype (AWKINTERP *pinter, awksymb* var);
int awk_getvar (AWKINTERP *pinter, awksymb* var);
int awk_setvar (AWKINTERP *pinter, awksymb* var);

#ifdef __cplusplus
}
#endif
