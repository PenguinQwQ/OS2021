#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <co.h>

int count = 1;

void entry(void *arg) {
	for (int i = 0; i < 5; i++) {
		printf("%s[%d]", (const char *)arg, count++);	
		co_yield();
	}	
}
int main() {
	struct co *co1 = co_start("co1", entry, "a");
	struct co *co2 = co_start("co2", entry, "b");
	co_yield();
	printf("Done\n");
	return 0;	
}
