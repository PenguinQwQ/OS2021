#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp 8

static int ttid[smp], sum = 0;

int cpu_current() {
	for (int i = 0; i < smp; i++) 
		if (syscall(SYS_gettid) == ttid[i]) return i;	
	assert(0);
}

int cpu_count() {
	return smp;	
}

void entry(int tid) {
	lock();
	ttid[sum++] = syscall(SYS_gettid);
	unlock();
	printf("%d\n", sum);	
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
