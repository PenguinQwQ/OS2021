#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <kernel.h>
#include <stdint.h>
#define ROUNDUP(a, sz) ((((uintptr_t)a) + (sz) - 1) & ~((sz)- 1))

int cpu_count();

int cpu_current();

int atomic_xchg(int *addr, int newval);

int cpu_count();
