#pragma once

#include <stdio.h>
#pragma comment (lib, "awk")
#ifdef __cplusplus
extern "C" {
#endif
struct AWKINTERP;

AWKINTERP* awk_init (const char **vars);
int awk_err (const char **msg);
int awk_setprog (AWKINTERP* pinter, const char *prog);
int awk_addprogfile (AWKINTERP* pinter, const char *progfile);
int awk_compile (AWKINTERP* pinter);
int awk_addarg (AWKINTERP* pinter, const char *arg);
int awk_exec (AWKINTERP* pinter);
void awk_end (AWKINTERP* prog);
void awk_setdebug (int level);

#ifdef __cplusplus
}
#endif
