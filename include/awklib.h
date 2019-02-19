#pragma once

#include <stdio.h>
#pragma comment (lib, "awk")
#ifdef __cplusplus
extern "C" {
#endif
struct AWKINTERP;
typedef int (*inproc)();
typedef int (*outproc)(const char *buf, size_t len);

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

#ifdef __cplusplus
}
#endif
