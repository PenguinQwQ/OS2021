#include <stdatomic.h>

inline int atomic_xchg(int *addr, int newval) {
	return atomic_exchange((int *)addr, newval);
}
