#include <common.h>
#include <threads.h>
#include <unistd.h>
#define smp 8

int cpu_current() {
	return getpid();	
}

int cpu_count() {
	return smp;	
}

void entry(int pid) {
	printf("%d\n", pid -1);	
} 

int main() {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
