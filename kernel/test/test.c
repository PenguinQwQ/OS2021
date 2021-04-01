#include <common.h>
#define smp 4

int cpu_count() {
	return smp;	
}

int main() {
	pmm -> init();	
	return 0;
}
