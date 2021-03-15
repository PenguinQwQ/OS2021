#include "co.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>

#define STACK_SIZE 65536
#define MAX_SIZE   128

enum co_status{
	CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,	
};

struct co {
	char *name;
	void (*func)(void *);
	void *arg;

	enum    co_status status;
	struct  co* waiter;
	jmp_buf context;
	uint8_t stack[STACK_SIZE];
};

struct co* cor[MAX_SIZE];

struct co *cur;

char MainName[5] = {"main"};

static int sum = 0;

__attribute__((constructor)) static void init() {
	struct co *now = (struct co *)malloc(sizeof(struct co));
	assert(now != NULL);
	now -> name = MainName;
	now -> func = NULL;
	now -> arg  = NULL;

	now -> status = CO_RUNNING;
	now -> waiter = NULL;
	
	cor[sum++] = now;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
	struct co *now = (struct co *)malloc(sizeof(struct co));
	assert(now != NULL);
	now -> name = (char *)name;
	now -> func = func;
	now -> arg  = arg;

	now -> status = CO_NEW;
	now -> waiter = NULL;
	memset(now -> stack, 0, sizeof(now -> stack));
	cor[sum++] = now;
	return now;
}
static inline void stack_switch_call (void *sp, void *entry, uintptr_t arg) {
	  asm volatile (
	  #if __x86_64__
	      "andq $0xfffffffffffffff0, %%rsp; movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
		     : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
	  #else
		  "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
			 : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
      #endif
	  );
}



void co_yield() {
	
	
}

void co_wait(struct co *co) {

}
