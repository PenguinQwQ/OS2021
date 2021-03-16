#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
void fun() {
	printf("1\n");
}
int main() {
	co_start("s", fun, NULL);
	co_yield();
	return 0;	
}
