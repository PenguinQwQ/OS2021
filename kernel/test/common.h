#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#define ROUNDUP(a, sz) ((((uintptr_t)a) + (sz) - 1) & ~((sz)- 1))

int cpu_count();

int cpu_current();
