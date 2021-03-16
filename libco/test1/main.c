#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <co.h>
void fun() {
	for (int i = 1; i < 1; i++);
}
int main() {
	co_start("s", fun, NULL);
	co_yield();
	return 0;	
}
