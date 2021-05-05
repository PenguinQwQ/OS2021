#include <common.h>
#define MAX_CPU 128

task_t *task_head;
task_t *current[MAX_CPU];

static void kmt_init() {
	task_head = NULL;
	for (int i = 0; i < MAX_CPU; i++)
		current[i] = NULL;	
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

static void spin_init(spinlock_t *lk, const char *name) {
	lk -> name = name;
	lk -> lock = 0;	
}

int cnt[MAX_CPU];
int status[MAX_CPU];

static void spin_lock(spinlock_t *lk) {
	int i = ienabled();
	iset(false);
	int id = cpu_current();
	if (cnt[id] == 0) status[id] = i;
	cnt[id] = cnt[id] + 1;
	while(atomic_xchg(&lk -> lock, 1));	
}

static void spin_unlock(spinlock_t *lk) {
	atomic_xchg(&lk -> lock, 0);
	int id = cpu_current();
	cnt[id]--;
	if (cnt[id] == 0) {
		if (status[id])
			iset(true);
		else iset(false);	
	}
	else assert(ienabled() == false);
}

MODULE_DEF(kmt) = {
	.init        = kmt_init,
	.create      = kmt_create,
	.teardown    = kmt_teardown,
	.spin_init   = spin_init,
	.spin_lock   = spin_lock,
	.spin_unlock = spin_unlock,
};
