#include <common.h>
#define MAX_CPU 128

spinlock_t trap_lock;

void func(void *args) {
	int ti = 0;
	while(1) {
		int i = ienabled();
		iset(false);
		printf("Hello from CPU#%d for %d times with arg %s!\n", cpu_current(), ti++, args);	  
		if (i)iset(true);
	}
}


int Lists_sum = 0;

sem_t empty, fill;

void producer() {
	while(1){kmt->sem_wait(&empty); putch('('); kmt->sem_signal(&fill);}
}

void comsumer() {
	while(1){kmt->sem_wait(&fill); putch(')'); kmt->sem_signal(&empty);}
}
int T = 0;
static void os_init() {
  T++;
  Lists_sum = 0;
  pmm->init();
  kmt->init();
  kmt->spin_init(&trap_lock, "os_trap");
/*
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "aa");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "bb");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "cc");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "dd");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "ee");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "ff");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "gg");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "hh");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "ii");
  kmt -> create(pmm -> alloc(sizeof(task_t)), "hello", func, "jj");
*/

  kmt -> sem_init(&empty, "empty", 5);
  kmt -> sem_init(&fill,  "fill" , 0);
//  for (int i = 0; i < 4; i++) 
	  kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);
	
//  for (int i = 0; i < 5; i++) 
	  kmt->create(pmm->alloc(sizeof(task_t)), "consumer", comsumer, NULL);
	  
}

static void os_run() {
  iset(true);
  while(1);
}

extern task_t *task_head;
extern task_t *current[MAX_CPU];
task_t origin[MAX_CPU];

static Context* os_trap(Event ev, Context *context) {
	assert(ienabled() == false);
	int id = cpu_current();
	if (current[id] != NULL) current[id] -> ctx = context;
	else {
		current[id] = &origin[id];
		origin[id].ctx = context;
		current[id] -> status = RUNNING;
	}

	for (int i = 0; i < Lists_sum; i++)
		if (ev.event == Lists[i].event || Lists[i].event == EVENT_NULL)
			Lists[i].func(ev, context);
			
	kmt -> spin_lock(&trap_lock);
	task_t *next = NULL, *now = task_head;
	assert(ienabled() == false);
	while (now != NULL)	{
		if (now -> status == SUITABLE) {
			next = now;
			next -> status = RUNNING;
			break;	
		}
		now = now -> next;
	}

	assert(current[id] != NULL);

	if (next == NULL) {
		assert(ienabled() == false);
		if (current[id] -> status != BLOCKED) current[id] -> status = SUITABLE;

		assert(origin[cpu_current()].ctx != NULL);
		current[id] = &origin[cpu_current()];
		current[id] -> status = RUNNING;
		kmt -> spin_unlock(&trap_lock);
		assert(ienabled() == false);
		return current[id] -> ctx;
	}
	assert(next != NULL);
	assert(next -> status == RUNNING);
	if (current[id] -> status != BLOCKED) current[id] -> status = SUITABLE;
	assert(current[id] != next && next -> status == RUNNING);

	assert(cpu_current() == id);
	current[id] = next;
	kmt -> spin_unlock(&trap_lock);
	assert(ienabled() == false);
	assert(current[id] -> status == RUNNING);
	return next -> ctx;	
}

static void os_on_irq(int seq, int event, handler_t handler) {
	assert(Lists_sum < 65536);
	Lists[Lists_sum].func  = handler;
	Lists[Lists_sum].seq   = seq;
	Lists[Lists_sum].event = event;
	Lists_sum              = Lists_sum + 1;

	// bubble sort
	for (int j = 0; j < Lists_sum - 1; j++)
			for (int i = 0; i < Lists_sum - 1 - j; i++)
				if (Lists[i].seq > Lists[i + 1].seq) {
					int tep = Lists[i].seq;
					Lists[i].seq = Lists[i + 1].seq;
					Lists[i + 1].seq	= tep;

					tep = Lists[i].event;
					Lists[i].event = Lists[i + 1].event;
					Lists[i + 1].event = tep;

					handler_t _tep = Lists[i].func;
					Lists[i].func = Lists[i + 1].func;
					Lists[i + 1].func = _tep;
				}
}

MODULE_DEF(os) = {
  .init    = os_init,
  .run     = os_run,
  .trap    = os_trap,
  .on_irq  = os_on_irq,
};

