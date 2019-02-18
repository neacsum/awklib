#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
struct awkstat;

awkstat* awk_init (const char **vars);
int awk_err (const char **msg);
int awk_setprog (awkstat* stat, const char *prog);
int awk_addprogfile (awkstat* stat, const char *progfile);
int awk_compile (awkstat* stat);
int awk_addarg (awkstat* stat, const char *arg);
int awk_exec (awkstat* stat);
void awk_end (awkstat* prog);
void awk_setdebug (int level);

#ifdef __cplusplus
}
#endif
