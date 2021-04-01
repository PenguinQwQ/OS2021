#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <kernel.h>
#include <stdint.h>
#include <unistd.h>
#define ROUNDUP(a, sz) ((((uintptr_t)a) + (sz) - 1) & ~((sz)- 1))

int cpu_count();

int cpu_current();

int cpu_count();
