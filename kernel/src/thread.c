#include <common.h>
#define MAX_CPU 128

task_t *task_head;
task_t *current[MAX_CPU];
extern spinlock_t trap_lock;

static void kmt_init() {
	task_head = NULL;
	for (int i = 0; i < MAX_CPU; i++)
		current[i] = NULL;	
}
extern uint32_t ProcLoc;
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
	kmt -> spin_lock(&trap_lock);
//	printf("%x\n", ProcLoc);
	task -> stack = pmm -> alloc(STACK_SIZE);
	assert(task -> stack != NULL);
	task -> name  = name;
	Area kstack;
	kstack.start  = task -> stack, kstack.end = (char *)task -> stack + STACK_SIZE;
	task -> ctx	  = kcontext(kstack, entry, arg);
	task -> status = SUITABLE;
	task -> on = false;
	task -> times = 0;
	task -> sleep_flag = false;
	task -> inode = 0x200000;
	if (task_head == NULL) task_head = task;
	else {
		task_t *now = task_head;
		while (now -> next != NULL) now = now -> next;
		now -> next = task;
	}
	kmt -> spin_unlock(&trap_lock);
	return 0;
}

static void kmt_teardown(task_t *task) {
	kmt -> spin_lock(&trap_lock);
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
	kmt -> spin_unlock(&trap_lock);
}

static void spin_init(spinlock_t *lk, const char *name) {
	lk -> name = name;
	lk -> lock = 0;	
	lk -> cpu_id = MAX_CPU;
}

int cnt[MAX_CPU];
int status[MAX_CPU];

static void spin_lock(spinlock_t *lk) {
	int i = ienabled();
	iset(false);
	assert(lk -> cpu_id != cpu_current() || lk -> lock == 0);
	while(atomic_xchg(&lk -> lock, 1));	
	int id = cpu_current();
	if (cnt[id] == 0) status[id] = i;
	cnt[id] = cnt[id] + 1;
	lk -> cpu_id = id;
	assert(ienabled() == false);
}

static void spin_unlock(spinlock_t *lk) {
	assert(ienabled() == false);
	int id = cpu_current();
	assert(lk-> cpu_id == id);
	cnt[id]--;
	lk -> cpu_id = MAX_CPU;
	assert(ienabled() == false);
	assert(atomic_xchg(&lk -> lock, 0) == 1);
	if (cnt[id] == 0) {
		if (status[id])
			iset(true);
		else iset(false);	
	}
	else assert(ienabled() == false);
}

static void sem_init(sem_t *sem, const char *name, int value) {
	kmt -> spin_init(&sem -> lock, name);
	sem -> name = name;
	sem -> count = value;
	sem -> head = NULL;
}

static void sem_wait(sem_t *sem) {
	kmt -> spin_lock(&trap_lock);
//	kmt -> spin_lock(&sem -> lock);
	assert(ienabled() == false);
	sem -> count --;
	int flag = 0;
//  printf("%s\n", sem->name);
	if (sem -> count < 0) {
		flag = 1;
		int id = cpu_current();
		assert(current[id] != NULL);
		current[id] -> status = BLOCKED;
		struct WaitList *tep = sem -> head;
		sem -> head = pmm -> alloc(sizeof(struct WaitList));
		sem -> head -> task = current[id];
		sem -> head -> next = tep;
	/*	
		while(sem -> count <= 0) {*/
//			kmt->spin_unlock(&sem -> lock);
			kmt -> spin_unlock(&trap_lock);
			yield();
		//	kmt -> spin_unlock(&sem -> lock);
		//	kmt -> spin_unlock(&trap_lock);
	//	}
	}
	if (flag == 0) {
//		sem -> count--;
//		kmt -> spin_unlock(&sem -> lock);
		kmt -> spin_unlock(&trap_lock);
	}
}

static void sem_signal(sem_t *sem) {
	kmt -> spin_lock(&sem -> lock);
	int flag = (trap_lock.cpu_id == cpu_current() && trap_lock.lock == 1);
	if (!flag) kmt -> spin_lock(&trap_lock);
	sem -> count++;
	
	struct WaitList *tep;
	if (sem -> head != NULL) {
		assert(sem -> head -> task -> status == BLOCKED);
		sem -> head -> task -> status = SUITABLE;
	//	assert(current[cpu_current()] -> status != SUITABLE);
		tep = sem -> head;
		sem -> head = sem -> head -> next;
		pmm -> free(tep);
	}
//	assert(current[cpu_current()] -> status == RUNNING);
	
	if (!flag)kmt -> spin_unlock(&trap_lock);
	kmt -> spin_unlock(&sem -> lock);
}								

MODULE_DEF(kmt) = {
	.init        = kmt_init,
	.create      = kmt_create,
	.teardown    = kmt_teardown,
	.spin_init   = spin_init,
	.spin_lock   = spin_lock,
	.spin_unlock = spin_unlock,
	.sem_init    = sem_init,
	.sem_wait    = sem_wait,
	.sem_signal  = sem_signal,
};
