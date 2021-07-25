#ifndef AAAAAAAAAAAAAA
#define AAAAAAAAAAAAAA
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <limits.h>
#include <vfs.h>
#define STACK_SIZE 4096 * 8
#define SUITABLE  2
#define RUNNING   1
#define BLOCKED   0

extern struct task* task_head;
extern struct task* current[128];
extern spinlock_t trap_lock;
extern uint32_t current_dir[8];
extern uint32_t ProcLoc;
struct file* create_file(uint32_t now, char *name, int type);
uint32_t GetClusLoc(uint32_t now);

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
	uint32_t inode;
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

#endif
