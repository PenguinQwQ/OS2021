#include <common.h>
static void kmt_init() {
	
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
	task -> stack = pmm -> alloc(STACK_SIZE);
	assert(task -> stack != NULL);
	task -> name  = name;
	Area kstack;
	kstack.start  = task -> stack, kstack.end = (char *)task -> stack + STACK_SIZE;
	task -> ctx	  = kcontext(kstack, entry, arg);
	return 0;
}

static void kmt_teardown(task_t *task) {
	pmm -> free(task -> stack);
	task -> stack = NULL;
	task -> name  = NULL;
	task -> ctx	  = NULL;
}

MODULE_DEF(kmt) = {
	.init     = kmt_init,
	.create   = kmt_create,
	.teardown = kmt_teardown,
};
