#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp 4

static int ttid[smp], sum = 0;

int cpu_current() {
	for (int i = 0; i < smp; i++) 
		if (syscall(SYS_gettid) == ttid[i]) return i;	
	assert(0);
}

int cpu_count() {
	return smp;	
}

void task1() { // smoke task
	for (int i = 0; i < 100000; i++) {
		int p = rand() % 10;
		if (p <= 4) pmm->alloc(rand() % 1024);	
		else if (p <= 7) pmm -> alloc(4096);
		else pmm->alloc(rand());
	}	
}

void entry(int tid) {
	lock();
	ttid[sum++] = syscall(SYS_gettid);
	unlock();
	task1();
	printf("task_1 success on thread#%d!\n", cpu_current());
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
