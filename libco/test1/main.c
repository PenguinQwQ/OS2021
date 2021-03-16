#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <co.h>

int count = 1;

void entry(void *arg) {
	printf("1\n");
}
int main() {
	struct co *co1 = co_start("co1", entry, "a");
	co_yield();
	printf("Done\n");
	return 0;	
}
