#include <common.h>
#define PAGE_SIZE      4096
#define MAX_CPU        8
#define MAX_DATA_SIZE  8
#define MAX_PAGE       3000
#define LUCK_NUMBER    10291223
#define MAX_BIG_SLAB   2048

#ifdef TEST
#include <test.h>
struct Area{
	void *start;
	void *end;
}heap;
#define Heap_Size (4ll << 30)
#define MAX_LIST       1000000
#else
#define MAX_LIST       100000
#endif

typedef struct{
	int flag;	
}spinlock_t;


static int DataSize[MAX_DATA_SIZE] = {8, 16, 32, 64, 128, 512, 1024, 2048};
static int power[MAX_DATA_SIZE]    = {7, 15, 31, 63, 127, 15, 7, 7};
static int remain_cnt[MAX_CPU][MAX_DATA_SIZE];

struct page_t{
	spinlock_t *lock;
	struct page_t *next;
	int block_size;
	int magic;
	int belong;
	int remain;
	int id;
};

struct ptr_t{
  uintptr_t slot[512];
};

struct ptr_t *_ptr[MAX_PAGE]; 

struct page_t *page_table[MAX_CPU][MAX_DATA_SIZE];

spinlock_t lock[MAX_CPU];
spinlock_t BigLock_Slow;
spinlock_t BigLock_Slab;

struct node{
	int head1;
	int val_next[MAX_LIST];
	uintptr_t val_l[MAX_LIST], val_r[MAX_LIST];
	int val_valid[MAX_LIST], sum1;
    
	int head2;
	int delete_next[MAX_LIST];
	uintptr_t delete_l[MAX_LIST], delete_r[MAX_LIST];
	int delete_valid[MAX_LIST], sum2;
}*List;

void init_list() {
	List -> sum1 = List -> sum2 = 0;
	List -> delete_valid[List -> sum2++] = 1;
	for (int i = 0; i < MAX_LIST; i++) {
		List -> val_next[i] = List -> delete_next[i] = 0;
		List -> val_l[i] = List -> val_r[i] = 0;
		List -> delete_l[i] = List -> delete_r[i] = 0;
		if (i < 2) continue;
		List -> val_valid[List -> sum1++] = i, List -> delete_valid[List -> sum2++] = i;
	}	
	List -> head1 = 1, List -> head2 = 0;
	List -> val_l[List -> head1] = (uintptr_t)heap.start;
	List -> val_r[List -> head1] = (uintptr_t)heap.end;
}


void spinlock(spinlock_t *lk) {
	while(atomic_xchg(&lk -> flag, 1));	
}

void spinunlock(spinlock_t *lk) {
	atomic_xchg(&lk -> flag, 0);
}

int judge_size(size_t size) {
	for (int i = 0; i < MAX_DATA_SIZE; i++)
		if (size <= DataSize[i]) return i;
	if (size <= 4096) return MAX_DATA_SIZE;
	else return MAX_DATA_SIZE + 1;
}

void add_delete(int l, int r) {
	assert(List -> sum2);
	int id = List -> delete_valid[--List -> sum2];
	if (List -> head2 == 0) List -> head2 = id;
	else {
		List -> delete_next[id] = List -> head2;
	    List -> head2 = id;
	}	
	List -> delete_l[id] = l;
	List -> delete_r[id] = r;
}

void *Slow_path(size_t size) {
	int now = List -> head1;
	if (now == 0) assert(0);//return NULL;
	int tep = 2;
    while (tep < size) tep = tep * 2;
	uintptr_t left ,right;	
	while(now) {
		left = ROUNDUP(List -> val_l[now], tep), right = List -> val_r[now];	
		if (right - left >= size) break;
		now = List -> val_next[now];
	}
	if (now == 0) {
		return NULL;
	}
	if (left == List -> val_l[now]) {
		List -> val_l[now] = left + size;
		add_delete(left, left + size);
	    return (void *)left;	
	}
	else {
		List -> val_r[now] = left;
		printf("%p\n", left);
		assert(List -> sum1);	
		int nxt = List -> val_valid[--List -> sum1];
		List -> val_l[nxt] = left + size, List -> val_r[nxt] = right;
		List -> val_next[nxt]  = List -> val_next[now];
		List -> val_next[now]  = nxt;
		add_delete(left, left + size);
		return (void *)left; 
	}
}

void* deal_slab(int id, int kd, int sz) {
	if (kd == MAX_DATA_SIZE) {
		spinlock(&BigLock_Slow);
		void *tep = Slow_path(sz);
		spinunlock(&BigLock_Slow);
		return tep;
	}
	if (remain_cnt[id][kd] == 0) return deal_slab(id, kd + 1, sz);
	struct page_t *now;
	now = page_table[id][kd];
	while (now != NULL && now -> remain == 0) now = now ->next;
	assert(now != NULL);
	assert(now -> remain != 0);
	remain_cnt[id][kd]--;
    return (void *)_ptr[now -> belong] -> slot[--now -> remain];	
}

void deal_slab_free(struct page_t *now, void *ptr) {
	assert(now -> magic == LUCK_NUMBER);
	remain_cnt[now -> id][now -> block_size]++;
	_ptr[now -> belong] -> slot[now -> remain ++] = (uintptr_t)ptr;
}

uintptr_t BigSlab[MAX_BIG_SLAB];
static int BigSlab_Size = 0;
uintptr_t lSlab, rSlab;

void deal_SlowSlab_free(void *ptr) {
	BigSlab[BigSlab_Size++] = (uintptr_t)ptr;	
}

void *SlowSlab_path() {
	if (BigSlab_Size > 0) return (void *)BigSlab[--BigSlab_Size];
	else {
		spinlock(&BigLock_Slow);
		void *tep = Slow_path(PAGE_SIZE);
		spinunlock(&BigLock_Slow);
		return tep;
	}
}



uintptr_t lookup_right(uintptr_t left) {
	int now = List -> head2, prev = 0;
	uintptr_t right = 0;
	assert(now);
	while (now) {
		if (List -> delete_l[now] == left) {
			right = List -> delete_r[now];
			break;	
		}
		prev = now;
		now = List -> delete_next[now];
	}
	assert(now);
	List -> delete_valid[List -> sum2++] = now;
	List -> delete_l[now] = List -> delete_r[now] = 0;
	if (List -> head2 == now) List -> head2 = List -> delete_next[now];
	else if(prev) List -> delete_next[prev] = List -> delete_next[now];
	List -> delete_next[now] = 0;
	return right;	
}

void deal_Slow_free(uintptr_t left) {
	uintptr_t right = lookup_right(left);
	int now = List -> head1;
	assert(now);
	if (right <= List -> val_l[now]) {
		if (right == List -> val_l[now]) List -> val_l[now] = left;
		else {
			assert(List -> sum1);
			int id = List -> val_valid[--List -> sum1];
			List -> val_l[id] = left, List -> val_r[id] = right;
			List -> val_next[id] = now;
			List -> head1 = id;	
		}
	}
	else {
		int nxt = 0;
		while(now) {
			if (List -> val_r[now] <= left) {
				nxt = List -> val_next[now];
				assert(nxt);
				if(List -> val_l[nxt] >= right) break;	
			}
			now = List -> val_next[now];
		}
		assert(List -> val_l[nxt] >= right);
		assert(now);

		int bj = 0;
		if (List -> val_r[now] == left) bj = 1, List->val_r[now] = right;
		else if (List -> val_l[nxt] == right) bj = 1, List->val_l[nxt] = left;
		if (bj) {
			if (List -> val_r[now] == List -> val_l[nxt]) {
				List -> val_next[now] = List -> val_next[nxt];
				List -> val_r[now] = List -> val_r[nxt];
				List -> val_l[nxt] = List -> val_r[nxt] = 0;
				List -> val_next[nxt] = 0;
				List -> val_valid[List -> sum1++] = nxt;
			}	
		}
		else {
			assert(List -> sum1);
			int id = List -> val_valid[--List -> sum1];
			List -> val_l[id] = left, List -> val_r[id] = right;
			List -> val_next[id] = nxt, List -> val_next[now] = id;	
		}
	}	
}


void debug_count() {
	int now = List -> head1, sub = 0, sup = 0;
	while(now) {
		sup++;
		now = List -> val_next[now];
	}	
	now = List -> head2, sub = 0;
	while(now) {
		sub++;
		now = List -> delete_next[now];
	}	
	printf("sum1:%d sum1: %d\n", sup, sub);
}


static void *kalloc(size_t size) {
  if ((size >> 20) > 16) return NULL;
  int id = cpu_current();
  int kd = judge_size(size);
  void *space;
  if (kd < MAX_DATA_SIZE) {
	spinlock(&lock[id]);
	space = deal_slab(id, kd, kd);
	spinunlock(&lock[id]);	  
	return space;
  }
  else if(kd == MAX_DATA_SIZE) {
	spinlock(&BigLock_Slab);
	space = SlowSlab_path();
	spinunlock(&BigLock_Slab);
	return space;
  }
  else if (kd == MAX_DATA_SIZE + 1) {
	spinlock(&BigLock_Slow);
	space = Slow_path(size);
	spinunlock(&BigLock_Slow);
	return space;  
  }
  else assert(0);
}

int judge_free(void *ptr) {
  struct page_t *now = (struct page_t *) ((uintptr_t) ptr & (~(PAGE_SIZE - 1)));	
  if (now -> magic == LUCK_NUMBER) return 1;
  else if ((uintptr_t)ptr >= lSlab && (uintptr_t)ptr < rSlab) return 2;
  else return 3;
}

static void kfree(void *ptr) {
  int kd = judge_free(ptr);
  if (kd == 1) {  
	struct page_t *now = (struct page_t *) ((uintptr_t)ptr & (~(PAGE_SIZE - 1)));
	spinlock(now->lock);
	deal_slab_free(now, ptr);
	spinunlock(now->lock);
  }
  else if (kd == 2) {
	  spinlock(&BigLock_Slab);
	  deal_SlowSlab_free(ptr);
	  spinunlock(&BigLock_Slab);
  }
  else if (kd == 3) {
	  spinlock(&BigLock_Slow);
	  deal_Slow_free((uintptr_t)ptr);
	  spinunlock(&BigLock_Slow);
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
		page -> block_size  = memory_size;
		page -> belong      = cnt++;
		page -> magic       = LUCK_NUMBER; 
		page -> remain      = 0;
		page -> id          = cpu_id;
		for (uintptr_t k = (uintptr_t)heap.start + pmax(128, DataSize[memory_size]); 
					   k != (uintptr_t)heap.start + PAGE_SIZE;
					   k += DataSize[memory_size]) {
			_ptr[page -> belong] -> slot[page -> remain] = k;	
			page -> remain = page -> remain + 1;
			remain_cnt[cpu_id][memory_size]++;
	}
	assert(page->remain <= 512);
	assert(sizeof(_ptr[cnt - 1]) <= 4096);
	heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
	return page;
	}
	else if (kd == 2) {
		void * tep = heap.start;	
		heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);	
		return (struct page_t *)tep;
	}
    else assert(0);
}

static void pmm_init() {
  assert(sizeof(DataSize) / sizeof(int) == MAX_DATA_SIZE);
  int tep = 0;
  for (int i = 0; i < MAX_DATA_SIZE; i++) tep += power[i];
  assert(tep <= MAX_PAGE);
  
  #ifndef TEST
  heap.start = (void *)ROUNDUP(heap.start, PAGE_SIZE);
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  #else
  char *ptr = malloc(Heap_Size);
  assert(ptr != NULL);
  heap.start = ptr;
  heap.start = (void *)ROUNDUP(heap.start, PAGE_SIZE);
  heap.end   = heap.start + Heap_Size;
  printf("Got %d MiB heap: [%p, %p)\n", Heap_Size >> 20, heap.start, heap.end);
  #endif
  BigLock_Slab.flag = 0;
  BigLock_Slow.flag = 0;
  
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

  lSlab = (uintptr_t)heap.start;
  for (int i = 0; i < MAX_BIG_SLAB; i++)
	BigSlab[BigSlab_Size++] = (uintptr_t)alloc_page(0, 0, 2);
  rSlab = (uintptr_t)heap.start;
  List = (struct node *)heap.start;
  heap.start = (void *)((uintptr_t)heap.start + sizeof(struct node));
  heap.start = (void *)ROUNDUP(heap.start + PAGE_SIZE, PAGE_SIZE);
  init_list();
  printf("Got %d MiB heap: [%p, %p)\n", (heap.end-heap.start) >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
