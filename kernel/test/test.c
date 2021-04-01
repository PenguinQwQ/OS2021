#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp 8

int cpu_current() {
	return syscall(224);	
}

int cpu_count() {
	return smp;	
}

void entry(int tid) {
	cpu_current();	
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
