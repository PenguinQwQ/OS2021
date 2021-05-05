#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#define STACK_SIZE 4096 * 8

struct task{
	const char *name;	
	Context *ctx;
	void *stack;
};

struct spinlock{
	
	
};

struct semaphore{
	
};

struct Node{
	struct handle{
		handler_t func;
		int seq;
	}List[100];
	int sum;	
}event_handle[256];

