#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <co.h>

void entry(void *arg) {
	while(1) {
		printf("%s", (const char *)arg);	
		co_yield();
	}	
}
int main() {
	struct co *co1 = co_start("co1", entry, "a");
	struct co *co2 = co_start("co2", entry, "b");
	co_wait(co1);
	co_wait(co2);
	return 0;	
}
