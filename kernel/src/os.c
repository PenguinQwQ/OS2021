#include <common.h>

static void os_init() {
  pmm->init();
}

void test1() {	
  #define MAXN 1001
  void *st[MAXN];
  int st2[MAXN];
  int top = 0;
  for (int i = 0; i < 6; i++) {
		int	op = rand() % 2;
		if (top == 0) op = 0;
		if (top == MAXN) op = 1;
		if (op == 0) {
			st2[top] = 4096 + (rand() & ((1 << 10) - 1));
			st[top]    = pmm->alloc(st2[top]);
			assert(st[top] != 0); 
			top++;
		}
	    else {
			pmm->free(st[top - 1]);
			top--;
		}
   }
	printf("%d\n", top);
	assert(0);	
}


static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  test1();
  while(1);
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
};
