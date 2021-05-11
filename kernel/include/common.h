#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <limits.h>
#define STACK_SIZE 4096 * 8
#define SUITABLE  2
#define RUNNING   1
#define BLOCKED   0

extern struct task* task_head;
extern struct task* current[128];
extern spinlock_t trap_lock;

struct spinlock{
	const char *name;	
	int  lock;
	int  cpu_id;
};
struct task{
	const char *name;	
	Context *ctx;
	void *stack;
	int status;
	bool on;
	bool sleep_flag;
	int times;
	struct spinlock lk;
	struct task* next;
};


struct WaitList{
	struct task *task;
	struct WaitList *next;
};

struct semaphore{
	struct spinlock lock;
	int count;
	const char* name;
	struct WaitList *head;	
};


struct Node{
	handler_t func;
	int seq;
	int event;
}Lists[65536];

