#include <common.h>
#define PAGE_SIZE      4096
#define MAX_CPU        8
#define MAX_DATA_SIZE  6
#define MAX_PAGE       2000
#define LUCK_NUMBER    10291223
#define MAX_LIST       50000

typedef struct{
	int flag;	
}spinlock_t;


static int DataSize[MAX_DATA_SIZE] = {8, 16, 32, 64, 128, 256};
static int power[MAX_DATA_SIZE]    = {7, 15, 31, 63, 63, 5};

struct page_t{
	spinlock_t *lock;
	struct page_t *next;
	int block_size;
	int magic;
	int belong;
	int remain;
};

struct ptr_t{
  uintptr_t slot[256];
};

struct ptr_t *_ptr[MAX_PAGE]; 

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
		if (size <= DataSize[i]) return i;
	return MAX_DATA_SIZE;
}

void* deal_slab(int id, int kd) {
	struct page_t *now;
	now = page_table[id][kd];
	while (now != NULL && now -> remain == 0) now = now ->next;
	assert(now != NULL);
	assert(now -> remain != 0);
    return (void *)_ptr[now -> belong] -> slot[--now -> remain];	
}

void deal_slab_free(struct page_t *now, void *ptr) {
	assert(now -> magic == LUCK_NUMBER);
	_ptr[now -> belong] -> slot[now -> remain ++] = (uintptr_t)ptr;
}

struct node{
	int l[MAX_LIST], r[MAX_LIST];
	uintptr_t val[MAX_LIST];	
}*List;


void *slow_path() {
	assert(0);
	return NULL;	
	
	
}

spinlock_t BigLock;

static void *kalloc(size_t size) {
  if ((size >> 20) > 16) return NULL;
  int id = cpu_current();
  int kd = judge_size(size);
  void *space;
  if (kd < MAX_DATA_SIZE) {
	spinlock(&lock[id]);
	space = deal_slab(id, kd);
	unspinlock(&lock[id]);	  
	return space;
  }
  else if(kd == MAX_DATA_SIZE) {
	  spinlock(&BigLock);
	  space = slow_path();
	  unspinlock(&BigLock);
	  return space;
  }
  assert(0);
}

int judge_free(void *ptr) {
  struct page_t *now = (struct page_t *) ((uintptr_t) ptr & (~(PAGE_SIZE - 1)));	
  if (now -> magic == LUCK_NUMBER) return 1;
  assert(0);
}

static void kfree(void *ptr) {
  int kd = judge_free(ptr);
  if (kd == 1) {  
	struct page_t *now = (struct page_t *) ((uintptr_t)ptr & (~(PAGE_SIZE - 1)));
	spinlock(now->lock);
	deal_slab_free(now, ptr);
	unspinlock(now->lock);
  }
  else assert(0);
}


int pmax(int a, int b) {
	return a > b ? a : b;
}

static int cnt = 0;

struct page_t* alloc_page(int cpu_id, int memory_size, int kd) {
	if (kd == 1) {
		_ptr[cnt] = (struct ptr_t *)heap.start;
		heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
		struct page_t *page = (struct page_t *)heap.start;
		page -> lock        = &lock[cpu_id];
		page -> next        = NULL;
		page -> block_size  = DataSize[memory_size];
		page -> belong      = cnt++;
		page -> magic       = LUCK_NUMBER; 
		page -> remain      = 0;
		for (uintptr_t k = (uintptr_t)heap.start + pmax(128, DataSize[memory_size]); 
					   k != (uintptr_t)heap.start + PAGE_SIZE;
					   k += DataSize[memory_size]) {
			_ptr[page -> belong] -> slot[page -> remain] = k;	
			page -> remain = page -> remain + 1;
	}
	heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
	return page;
	}
    else assert(0);
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  heap.start = (void *)ROUNDUP(heap.start, PAGE_SIZE);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  BigLock.flag = 0;
  int tot = cpu_count();
  for (int i = 0; i < tot; i++) {
	  lock[i].flag = 0;
	  for (int j = 0; j < MAX_DATA_SIZE; j++) {
		  page_table[i][j] = alloc_page(i, j, 1);
	  }
  }
  for (int i  = 0; i < tot; i++) {
	for (int j = 0; j < MAX_DATA_SIZE; j++) {
		struct page_t *now = page_table[i][j];
		for (int k = 0; k < power[j]; k++) {
			now -> next = alloc_page(i, j, 1);
			now = now -> next;	
		}		
	}	    
  }
  List = (struct node *)heap.start;
  while ((uintptr_t)(&List -> l[MAX_LIST - 1])   >= (uintptr_t)heap.start ||                     (uintptr_t)&(List -> r[MAX_LIST - 1])   >= (uintptr_t)heap.start ||                     (uintptr_t)&(List -> val[MAX_LIST - 1]) >= (uintptr_t)heap.start                           )
		heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);

  printf("Got %d MiB heap: [%p, %p)\n", (heap.end-heap.start) >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
