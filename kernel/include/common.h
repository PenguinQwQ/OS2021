#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#define STACK_SIZE 4096 * 8

struct task{
	const char *name;	
	Context *ctx;
	void *stack;
};
