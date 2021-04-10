#include <common.h>
#define PAGE_SIZE      4096
#define MAX_CPU        8
#define MAX_DATA_SIZE  3
#define MAX_SLAB_SUM   5
#define MAX_PAGE       100000
#define LUCK_NUMBER    10291223
#define MAX_BIG_SLAB   9012
#define MAX_SLOT       10

#ifdef TEST
#include <test.h>
struct Area{
	void *start;
	void *end;
}heap;
#define Heap_Size (150 << 20)
#define MAX_LIST       1000000
#else
#define MAX_LIST       200000
#endif

typedef struct{
	int flag;	
}spinlock_t;


static int DataSize[MAX_DATA_SIZE] = {64, 128, 1024};
static int power[MAX_DATA_SIZE]    = {256, 512, 256};
static int remain_cnt[MAX_CPU][MAX_DATA_SIZE];
static uintptr_t slot[MAX_SLOT];
static int _lSlab, _rSlab, slot_cnt = 0;;
uintptr_t st = 0;


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
	List -> val_l[List -> head1] = (uintptr_t)st;
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
	if (size == 4096) return MAX_DATA_SIZE;
	else return MAX_DATA_SIZE + 1;
}

void add_delete(uintptr_t l, uintptr_t r) {
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

int lst = 0;
spinlock_t lock_all;

void *Slow_path(size_t size) {

    int now = List -> head1;
	if (now == 0) assert(0);

	if (lst && List -> val_next[lst]) now = lst;


	int tep = 2;
    while (tep < size) tep = tep * 2;
	uintptr_t left ,right;	
	while(now) {
		left = ROUNDUP(List -> val_l[now], tep), right = List -> val_r[now];	
		if (right - left >= (uintptr_t)size && left <= right) {
			break;
		}
		now = List -> val_next[now];
	}
	lst = now;
	if (now == 0) return NULL;
	if (left == List -> val_l[now]) {
		List -> val_l[now] = left + size;
		add_delete(left, left + size);
	    return (void *)left;	
	}
	else {
		List -> val_r[now] = left;
		assert(List -> sum1);	
		int nxt = List -> val_valid[--List -> sum1];
		List -> val_l[nxt] = left + size, List -> val_r[nxt] = right;
		List -> val_next[nxt]  = List -> val_next[now];
		List -> val_next[now]  = nxt;
		add_delete(left, left + size);
		return (void *)left; 
	}
}

int pmax(int a, int b) {
	return a > b ? a : b;
}

uintptr_t BigSlab[MAX_CPU][MAX_BIG_SLAB];
static int BigSlab_Size[MAX_CPU] = {0};
uintptr_t lSlab[MAX_CPU], rSlab[MAX_CPU];
static int cnt = 0;

struct page_t* alloc_page(int cpu_id, int memory_size, int kd) {
	if (kd == 1 || kd == 3) {
		struct page_t *page;
		if (kd == 1) {
			_ptr[cnt] = (struct ptr_t *)st;
			st = ROUNDUP(st + PAGE_SIZE, PAGE_SIZE);	
	     	page = (struct page_t *)st;
			st = ROUNDUP(st + PAGE_SIZE, PAGE_SIZE);	
		}
		else {
			uintptr_t tep = (uintptr_t)Slow_path(PAGE_SIZE * 2);
			if (tep == 0) return NULL;
			spinlock(&BigLock_Slow);
			_ptr[cnt] = (struct ptr_t *)tep;
			assert(_ptr[cnt]);	
	     	page = (struct page_t *)(tep + PAGE_SIZE);
		}
		page -> lock        = &lock[cpu_id];
		page -> next        = NULL;
		page -> block_size  = memory_size;
		page -> belong      = cnt++;
		page -> magic       = LUCK_NUMBER; 
		page -> remain      = 0;
		page -> id          = cpu_id;
		if (kd == 3) spinunlock(&BigLock_Slow);
		for (uintptr_t k = ((uintptr_t)page) + pmax(128, DataSize[memory_size]); 
					   k != ((uintptr_t)page) + PAGE_SIZE;
					   k += DataSize[memory_size]) {
			_ptr[page -> belong] -> slot[page -> remain] = k;	
			page -> remain = page -> remain + 1;
			remain_cnt[cpu_id][memory_size]++;
	}
	assert(page->remain <= 512);
	assert(sizeof(_ptr[cnt - 1]) <= 4096);
	return page;
	}
	else if (kd == 2) {
		void *tep = (void *)st;	
		st = ROUNDUP(st + PAGE_SIZE, PAGE_SIZE);	
		return (struct page_t *)tep;
	}
	else if (kd == 4) {
		return NULL;
		void *tep = (void *)st;	
		st = ROUNDUP(st + 16 * PAGE_SIZE, PAGE_SIZE);	
		return (struct page_t *)tep;
	}
    else assert(0);
}

void *SlowSlab_path(int id, size_t sz) {
	if (sz <= 4096 && 
		BigSlab_Size[id] > 0) return (void *)BigSlab[id][--BigSlab_Size[id]];
	else {
		if (sz > 128) sz = pmax(sz, 128);
		else sz = pmax(sz, 128);
		spinlock(&BigLock_Slow);
		void *tep = Slow_path(sz);
		spinunlock(&BigLock_Slow);
		return tep;
	}
}

void* deal_slab(int id, int kd, size_t sz) {
	if (kd == MAX_DATA_SIZE) {
		return SlowSlab_path(id, sz);
		sz = pmax(sz, 128);
		spinlock(&BigLock_Slow);
		void *tep = Slow_path(sz);
		spinunlock(&BigLock_Slow);
		return tep;
	}

	struct page_t *now;
	if (remain_cnt[id][kd] == 0) {
		return deal_slab(id, kd + 1, sz);
/*		struct page_t* ptr = alloc_page(id, kd, 3);
		if (ptr == NULL);
		assert(ptr != NULL);
		now = page_table[id][kd];
		prev = NULL;
	    while (now != NULL && now -> remain == 0) prev = now, now = now ->next;
		assert(now == 0);
		assert(prev != NULL);
		prev -> next = ptr;
		now = ptr;*/
	}
	else {
		now = page_table[id][kd];
		while (now != NULL && now -> remain == 0) now = now ->next;
		assert(now != NULL);
		assert(now -> remain != 0);
	}
	assert(remain_cnt[id][kd]);
	remain_cnt[id][kd]--;
    return (void *)_ptr[now -> belong] -> slot[--now -> remain];	
}

void deal_slab_free(struct page_t *now, void *ptr) {
	assert(now -> magic == LUCK_NUMBER);
	remain_cnt[now -> id][now -> block_size]++;
	_ptr[now -> belong] -> slot[now -> remain ++] = (uintptr_t)ptr;
}


void deal_SlowSlab_free(int id, void *ptr) {
	BigSlab[id][BigSlab_Size[id]++] = (uintptr_t)ptr;	
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
//	printf("sum1:%d sum1: %d\n", sup, sub);
}

static void *special(size_t size) {
	if (slot_cnt > 0) return (void *)slot[--slot_cnt];
	else {
		spinlock(&BigLock_Slow);
		void *tep =  Slow_path(size);
		spinunlock(&BigLock_Slow);	
		return tep;
	}
} 

int tot = 0;
static void *kalloc(size_t size) {  

  assert(size);
  if ((size >> (size_t)20) >= (size_t)16) return NULL;
  void *space;
  int id = cpu_current();
  int kd = judge_size(size);

  if (kd < MAX_DATA_SIZE) {
	spinlock(&lock[id]);
	space = deal_slab(id, kd, size);
	spinunlock(&lock[id]);	  
	return space;
  }
  else if(kd == MAX_DATA_SIZE) {
	spinlock(&lock[id]);
	space = SlowSlab_path(id, size);
	spinunlock(&lock[id]);	  
	return space;
  }
  else if (kd == MAX_DATA_SIZE + 1) {
	spinlock(&BigLock_Slow);
	space = Slow_path(size);
	spinunlock(&BigLock_Slow);
	return space;  
  }
  else if (kd == MAX_DATA_SIZE + 2) {
	spinlock(&BigLock_Slab);
	space = special(size);
	spinunlock(&BigLock_Slab);
	return space;			  
  }
  else assert(0);
}

int judge_free(void *ptr) {
  assert(ptr != NULL);
  struct page_t *now = (struct page_t *) ((uintptr_t) ptr & (~(PAGE_SIZE - 1)));	
  if (now -> magic == LUCK_NUMBER) return 1;
  if ((uintptr_t)ptr >= _lSlab && (uintptr_t)ptr < _rSlab) return 3;
  int tot = cpu_count();
  for (int i = 0; i < tot; i++)
   if ((uintptr_t)ptr >= lSlab[i] && (uintptr_t)ptr < rSlab[i]) return 4 + i;
  
  return 2;
}

void special_free(void *ptr) {
	slot[slot_cnt++] = (uintptr_t)ptr;	
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
	  spinlock(&BigLock_Slow);
	  deal_Slow_free((uintptr_t)ptr);
	  spinunlock(&BigLock_Slow);
  }
  else if (kd == 3) {
	  spinlock(&BigLock_Slab);
	  special_free(ptr);
	  spinunlock(&BigLock_Slab);	  
  }
  else{	 
	 spinlock(&lock[kd - 4]);
	 deal_SlowSlab_free(kd - 4, ptr);
	 spinunlock(&lock[kd - 4]);
  }
}

static void pmm_init() {
  st = (uintptr_t)heap.start;
  BigLock_Slab.flag = 0;
  BigLock_Slow.flag = 0;
  assert(sizeof(DataSize) / sizeof(int) == MAX_DATA_SIZE);
  int tep = 0;
  for (int i = 0; i < MAX_DATA_SIZE; i++) tep += power[i] + 1;
  assert(tep * cpu_count() <= MAX_PAGE);
  
  #ifndef TEST
  st = ROUNDUP(st, PAGE_SIZE);
  #else
  char *ptr = malloc(Heap_Size);
  assert(ptr != NULL);
  st = (uintptr_t)ptr;
  st = ROUNDUP(st, PAGE_SIZE);
  heap.end   = (void *) st + Heap_Size;
  #endif
  
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
		for (int k = 0; k < power[j] - 1; k++) {
			now -> next = alloc_page(i, j, 1);
			now = now -> next;	
		}		
	}	    
  }

  for (int i = 0; i < tot; i++) {
	  lSlab[i] = st;
	  for (int j = 0; j < MAX_BIG_SLAB / tot; j++)
		BigSlab[i][BigSlab_Size[i]++] = (uintptr_t)alloc_page(0, 0, 2);
	  rSlab[i] = st;
  }


  _lSlab = st;
  for (int i = 0; i < MAX_SLOT; i++)
	slot[slot_cnt++] = (uintptr_t)alloc_page(0, 0, 4);
  _rSlab = st;

  List = (struct node *)st;
  st = ((uintptr_t)st + sizeof(struct node));
  st = ROUNDUP(st + PAGE_SIZE, PAGE_SIZE);
  printf("%d\n", (uintptr_t)(heap.end - st) >> 20);
  init_list();
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
