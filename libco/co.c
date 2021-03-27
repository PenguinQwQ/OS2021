#include "co.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define STACK_SIZE 65536
#define MAX_SIZE   128

enum co_status{
	CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,	
};

struct co {
	jmp_buf context2;
	char *name;
	void (*func)(void *);
	void *arg;

	enum    co_status status;
	struct  co* waiter;
	struct  co* caller;
	jmp_buf context;
	uint8_t stackw[STACK_SIZE];
//	uint8_t stackw[STACK_SIZE];
	uint8_t __attribute__((aligned(16)))stack[STACK_SIZE];
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

	now -> status  = CO_RUNNING;
	now -> waiter  = NULL;
	cor[sum++] = now;
	cur = now;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
	struct co *now = (struct co *)malloc(sizeof(struct co));
	assert(now != NULL);
	now -> name = (char *)name;
	now -> func = func;
	now -> arg  = arg;

	now -> status  = CO_NEW;
	now -> waiter  = NULL;
	memset(now -> stack, 0, sizeof(now -> stack));
	cor[sum++] = now;
	return now;
}

static inline void stack_switch_call (void *sp, void *entry, uintptr_t arg, uintptr_t entry2) {
	  asm volatile (
	  #if __x86_64__
	      "movq %0, %%rsp; andq $0xfffffffffffffff0, %%rsp;movq %2, %%rdi;            pushq %3; jmpq *%1"
		     : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg), "r"(entry2)
	  #else
		  "movl %0, %%esp; movl %2, (%0); push %3; jmp *%1"
			 : : "b"((uintptr_t)sp ), "d"(entry), "a"(arg), "r"(entry2)
      #endif
	  );
}

void jmp() {
	longjmp(cur->context2, 2);	
}

int lst;

void co_yield() {
	int val = setjmp(cur -> context);
	struct co* tep = cur; 
	if (val == 0) {
		int id = 0;
		while (cor[id] -> status == CO_WAITING ) {
			id = rand() % sum;
		}
		cur = cor[id];
		if (cur -> status == CO_NEW) {
			int val2 = setjmp(cur -> context2);
			if (val2 == 0) {
				cur -> status = CO_RUNNING;
				cur -> caller = tep;
				stack_switch_call(&(cur->stack[STACK_SIZE - 16]), cur->func, (uintptr_t)cur->arg, (uintptr_t)jmp);
			}
			else {
				cur -> status = CO_DEAD;
				if (cur -> waiter != NULL) {
					cur -> waiter -> status = CO_RUNNING;	
				}
				int tep = 0;
				for (int i = 0; i < sum; i++)
					if (cor[i] == cur) tep = i;	
				for (int i = tep; i < sum - 1; i++)
					cor[i] = cor[i + 1];
				sum--;
				co_yield();
			}
		}
		else {
			longjmp(cur->context, 1);	
		}
	}
	else {
		return;	
	}
}

void co_wait(struct co *co) {
	co -> waiter = cur;
	if (co->status != CO_DEAD) cur -> status = CO_WAITING;
	co_yield();
	free(co);	
}
