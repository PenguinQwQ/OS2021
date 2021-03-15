#include "co.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <setjmp.h>

#define STACK_SIZE 65536
#define MAX_SIZE   128

enum {
	CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,	
}co_status;

struct co {
	char *name;
	void (*func)(void *);
	void *arg;

	enum    co_status status;
	struct  co* waited;
	jmp_buf context;
	uint8_t stack[STACK_SIZE];
};

struct co* cor[MAX_SIZE];

struct co *cur;

static int sum = 0;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
	struct co *now = (struct co *)malloc(sizeof(struct co));
	assert(co != NULL);
	now -> name = name;
	now -> func = func;
	now -> arg  = arg;

	now -> status = CO_NEW;
	now -> waited = NULL;
	memset(now -> stack, 0, sizeof(now -> stack));
	cor[sum++] = co;
}

void co_yield() {
	
	
}

void co_wait(struct co *co) {

}
