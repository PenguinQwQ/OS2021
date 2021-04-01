#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp 8

static int ttid[smp], sum = 0;

lock
int cpu_current() {
	lock();
	for (int i = 0; i < smp; i++) 
		if (syscall(SYS_gettid) == ttid[i]) {
			unlock();
			return i;	
		}
	asseet(0);
}

int cpu_count() {
	return smp;	
}

void entry(int tid) {
	lock();
	ttid[sum++] = cpu_current();
	unlock();
	printf("%d\n", sum);	
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
