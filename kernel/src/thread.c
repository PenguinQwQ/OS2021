#include <common.h>

task_t *task_head;
task_t *current;

static void kmt_init() {
	
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
	task -> stack = pmm -> alloc(STACK_SIZE);
	assert(task -> stack != NULL);
	task -> name  = name;
	Area kstack;
	kstack.start  = task -> stack, kstack.end = (char *)task -> stack + STACK_SIZE;
	task -> ctx	  = kcontext(kstack, entry, arg);
	task -> status = RUNNING;
	if (task_head != NULL) task_head = task;
	else {
		task_t *now = task_head;
		while (now -> next != NULL) now = now -> next;
		now -> next = task;
	}
	return 0;
}

static void kmt_teardown(task_t *task) {
	if (task_head == task) task_head = NULL;
	else {
		task_t *now = task_head;
		while(now -> next != task) {
			assert(now -> next != NULL);
			now = now -> next;
		}
		now -> next = now -> next -> next;
	}

	pmm -> free(task -> stack);
	task -> stack = NULL;
	task -> name  = NULL;
	task -> ctx	  = NULL;
	task -> next  = 0;
	pmm  -> free(task);
}

MODULE_DEF(kmt) = {
	.init     = kmt_init,
	.create   = kmt_create,
	.teardown = kmt_teardown,
};
