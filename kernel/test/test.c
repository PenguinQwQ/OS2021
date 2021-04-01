#include <common.h>
#define smp 8

int cpu_count() {
	return smp;	
}

int main() {
	pmm -> init();	
	return 0;
}
