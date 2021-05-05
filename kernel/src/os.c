#include <common.h>
#define MAX_CPU 128

spinlock_t trap_lock;

void func(void *args) {
	int ti = 0;
	while(1) {
		printf("Hello from CPU#%d for %d times with arg %s!\n", cpu_current(), ti++, args);	  
	}
}

static void os_init() {
  for (int i = 0; i < 256; i++)
		event_handle[i].sum = 0;
  pmm->init();
  kmt->init();
  kmt->spin_init(&trap_lock, "os_trap");
  kmt->create(pmm -> alloc(sizeof(task_t)), "hello", func, "aa");
  kmt->create(pmm -> alloc(sizeof(task_t)), "hello", func, "bb");
}

static void os_run() {
  iset(false);
  while(1);
}

extern task_t *task_head;
extern task_t *current[MAX_CPU];

static Context* os_trap(Event ev, Context *context) {
	assert(ienabled() == false);
	int id = cpu_current();
	if (current[id] != NULL) {
		current[id] -> ctx = context;
	}
	kmt -> spin_lock(&trap_lock);
	task_t *next = NULL, *now = task_head;
	while (now != NULL)	{
		if (now -> status == RUNNING) {
			next = now;
			next -> status = BLOCKED;
			break;	
		}
		now = now -> next;
	}
	if (next == NULL) next = current[id];
	if (next == NULL) {
	   	kmt -> spin_unlock(&trap_lock);
		return context;
	}
	next -> status = BLOCKED;
	current[id] -> status = RUNNING;
	kmt -> spin_unlock(&trap_lock);
	current[id] = next;
	return current[id] -> ctx;	
}

static void os_on_irq(int seq, int event, handler_t handler) {
	if (event == EVENT_NULL);
	int sum = event_handle[event].sum;
	assert(sum < 100);
	event_handle[event].List[sum].func = handler;
	event_handle[event].List[sum].seq  = seq;
	sum								   = sum + 1;
	event_handle[event].sum            = sum;

	// bubble sort
	for (int j = 0; j < sum - 1; j++)
			for (int i = 0; i < sum - 1 - j; i++)
				if (event_handle[event].List[i].seq < \
					event_handle[event].List[i + 1].seq) {
					int tep = event_handle[event].List[i].seq;
					event_handle[event].List[i].seq   = \
					event_handle[event].List[i + 1].seq;
					event_handle[event].List[i + 1].seq	= tep;
					
					handler_t _tep = event_handle[event].List[i].func;
					event_handle[event].List[i].func =  \
					event_handle[event].List[i + 1].func;
					event_handle[event].List[i + 1].func = _tep;
				}
}

MODULE_DEF(os) = {
  .init    = os_init,
  .run     = os_run,
  .trap    = os_trap,
  .on_irq  = os_on_irq,
};

