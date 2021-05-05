#include <common.h>
static void os_init() {
  for (int i = 0; i < 256; i++)
		event_handle[i].sum = 0;
  pmm->init();
  kmt->init();
}

static void os_run() {
  iset(true);
  while(1);
}

static Context* os_trap(Event ev, Context *context) {
		
	return NULL;	
}

int compare(const void *w1, const void *w2) {
	struct handle *t1 = (struct handle *)w1;
	struct handle *t2 = (struct handle *)w2;
	return t1 -> seq < t2 -> seq;	
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

