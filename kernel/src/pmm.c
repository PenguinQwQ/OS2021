#include <common.h>
#define PAGE_SIZE      8192
#define MAX_CPU        8
#define MAX_DATA_SIZE  5
#define MAX_PAGE       100

typedef struct{
	int flag;	
}spinlock_t;

uintptr_t ptr[MAX_PAGE][512];
int		  sum[MAX_PAGE] = {0};


int DataSize[MAX_DATA_SIZE] = {16, 32, 64, 128};

struct page_t{
	spinlock_t *lock;
	struct page_t *next;
	int block_size;
	int belong;
	int magic;
};

struct page_t *page_table[MAX_CPU][MAX_DATA_SIZE];
spinlock_t lock[MAX_CPU];

static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}



static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  heap.start = (void *)ROUNDUP(heap.start, PAGE_SIZE);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
  int tot = cpu_count(), cnt = 0;
  for (int i = 0; i < tot; i++) {
	  for (int j = 0; j < MAX_DATA_SIZE; j++) {
		 struct page_t *page = (struct page_t *)heap.start;
		 page_table[i][j] = (struct page_t *)heap.start;
		 heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);
	     page -> lock = &lock[i];
		 page -> belong = cnt++;
		 page -> block_size = DataSize[j];
		 page -> next = NULL;
		 for (uintptr_t k = (uintptr_t)heap.start + 128; 
		     k != (uintptr_t)heap.start +PAGE_SIZE;
		     k += DataSize[j]) {
			 ptr[page -> belong][sum[page ->belong]++] = k;	
		 }
	  }
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
