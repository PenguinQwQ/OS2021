#include <common.h>
#include <threads.h>
#define smp 8

int cpu_count() {
	return smp;	
}

void entry() {
	printf("%d\n", cpu_current());	
} 

int main() {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
