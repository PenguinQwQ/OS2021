#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp 8

int cpu_current() {
	return syscall(SYS_gettid);	
}

int cpu_count() {
	return smp;	
}

void entry(int tid) {
	printf("%d\n", cpu_current());	
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
