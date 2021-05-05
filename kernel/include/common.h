#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#define STACK_SIZE 4096 * 8
#define RUNNING 1
#define BLOCKED 0

extern struct task* task_head;
extern struct task* current;

struct task{
	const char *name;	
	Context *ctx;
	void *stack;
	int status;
	struct task* next;
};

struct spinlock{
	const char *name;	
	int  lock;
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

