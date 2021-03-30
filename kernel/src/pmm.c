#include <common.h>
#define PAGE_SIZE      4096
#define MAX_CPU        8
#define MAX_DATA_SIZE  5
#define MAX_PAGE       100
#define LUCK_NUMBER    10291223

typedef struct{
	int flag;	
}spinlock_t;

uintptr_t ptr[MAX_PAGE][512];

int DataSize[MAX_DATA_SIZE] = {8, 16, 32, 64, 128};

struct page_t{
	spinlock_t *lock;
	struct page_t *next;
	int block_size;
	int magic;
	int belong;
	int remain;
};

struct page_t *page_table[MAX_CPU][MAX_DATA_SIZE];
spinlock_t lock[MAX_CPU];

void spinlock(spinlock_t *lk) {
	while(atomic_xchg(&lk -> flag, 1));	
}

void unspinlock(spinlock_t *lk) {
	atomic_xchg(&lk -> flag, 0);
}

int judge_size(size_t size) {
	for (int i = 0; i < MAX_DATA_SIZE; i++)
		if (size < DataSize[i]) return i;
	assert(0);
}

void* deal_slab(int id, int kd) {
	struct page_t *now;
	now = page_table[id][kd];
	while (now != NULL && now -> remain == 0) now = now ->next;
	assert(now != NULL);
    return (void *)ptr[now -> belong][-- now -> remain];	
}

static void *kalloc(size_t size) {
  int id = cpu_current();
  int kd = judge_size(size);
  void *space;
  if (kd < MAX_DATA_SIZE) {
	spinlock(&lock[id]);
	space = deal_slab(id, kd);
	unspinlock(&lock[id]);	  
	return space;
  }
  else assert(0);
}

static void kfree(void *ptr) {
}



static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  heap.start = (void *)ROUNDUP(heap.start, PAGE_SIZE);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
  int tot = cpu_count(), cnt = 0;
  for (int i = 0; i < tot; i++) {
	  lock[i].flag = 0;
	  for (int j = 0; j < MAX_DATA_SIZE; j++) {
		 struct page_t *page = (struct page_t *)heap.start;
		 page_table[i][j] = (struct page_t *)heap.start;
		 heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);
	     page -> lock       = &lock[i];
		 page -> next       = NULL;
		 page -> block_size = DataSize[j];
		 page -> belong     = cnt++;
		 page -> magic      = LUCK_NUMBER; 
		 page -> remain     = 0;
		 for (uintptr_t k = (uintptr_t)heap.start + 128; 
		                k != (uintptr_t)heap.start +PAGE_SIZE;
		                k += DataSize[j]) {
			 ptr[page -> belong][page -> remain] = k;	
			 page -> remain = page -> remain + 1;
		 }
	  }
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
