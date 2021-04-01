#include <common.h>
#include <stdatomic.h>

int cpu_current() {
	return get_pid();	
}

int atomic_xchg(int *addr, int newval) {
	return atomic_exchange((int *)addr, newval);	
}
