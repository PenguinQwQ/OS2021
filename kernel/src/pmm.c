#include <common.h>
#define PAGE_SIZE     4096
#define MAX_SLAB      32 
#define MAX_CPU       8
#define MAX_DATA_SIZE 10
#define MAX_CNT       50000
#ifdef TEST
#include <test.h>
struct Area{
	void *start;
	void *end;
}heap;
#define Heap_Size (512 << 20)
#define MAX_LIST       1000000
#else
#define MAX_LIST       200000
#endif

typedef struct{
	int flag;	
}spinlock_t;


spinlock_t lock_all;

void spinlock(spinlock_t *lk) {
	while(atomic_xchg(&lk -> flag, 1));
}

void spinunlock(spinlock_t *lk) {
	atomic_xchg(&lk -> flag, 0);
}

uintptr_t st = 0;

typedef struct page {
	spinlock_t lock;
	int id;
	int type;
	int head;
	int cnt;
	int sz;
	int special;
	uintptr_t end;
	struct page *next;
	uintptr_t stack[MAX_SLAB];
}page_t;

page_t* page_table[MAX_CPU][MAX_DATA_SIZE];

static int slot[MAX_DATA_SIZE] = {16, 32, 64, 128, 256, 512, 1024, 2048,  4096, 0};
typedef struct node{
	int head;
	int next[MAX_LIST];
	uintptr_t l[MAX_LIST], r[MAX_LIST];
	int valid[MAX_LIST];
	int sum;	
}List_t;

List_t *List;

void init_list() {
	List -> sum = 0;
	for (int i = 0; i < MAX_LIST; i++) {
		List -> next[i] = 0;
		List -> l[i] = List -> r[i] = 0;
		if (i < 2) continue;
		List -> valid[List -> sum++] = i;
	}
	List -> head = 1; 
	List -> l[1] = st, List -> r[1] = (uintptr_t)heap.end;
}

static int lst = 0;

void *slow_alloc(size_t size) {
	size_t tep = 4096;
	while (tep < size) tep = tep * 2;
	size = tep;
	int now = List -> head, prev = 0;
	lst = 0;
	if (lst) now = lst;
	uintptr_t left, right;
	spinlock(&lock_all);
	while (now != 0) {
		left = List -> l[now], right = List -> r[now];
		left = ROUNDUP(left, size);
		if (left - List -> l[now] < PAGE_SIZE) left = ROUNDUP(left + size, size);
		if (left < right &&  left + (uintptr_t)size < right) break;
		prev = now;
		now = List -> next[now];
	}
	if (now == 0) {
		lst = 0;
		spinunlock(&lock_all);
		return NULL;
	}
	lst = now;
	page_t *page = (page_t *)(left - PAGE_SIZE);
	page -> type = 2, page -> head = 0, page -> end = left + size;
    
	if (List -> l[now] == left - PAGE_SIZE && List -> r[now] == left + size) {
		assert(prev);
		List -> next[prev] = List -> next[now];
		List -> next[now] = List -> l[now] = List -> r[now] = 0;
	}
	else if (List -> l[now] == left - PAGE_SIZE) List -> l[now] = left + size;
	else if (List -> r[now] == left + size)      List -> r[now] = left - PAGE_SIZE;
	else {
		assert(List -> sum);
		int id = List -> valid[--List -> sum];
		List -> r[id]    = List -> r[now];
		List -> r[now]   = left - PAGE_SIZE,  List -> l[id]     = left + size;
		List -> next[id] = List -> next[now], List -> next[now] = id;
	}
	spinunlock(&lock_all);
	return (void *)left;
}

void init_page(page_t *page, int id, int kd, int sz, int special) {
	uintptr_t now = (uintptr_t)page;
	page -> id   = id;
	page -> type = 1;
	page -> head = kd;
	page -> cnt  = 0;
	page -> end  = now + PAGE_SIZE * 2;
	page -> next = NULL;
	page -> sz   = sz;
	page -> special = special;
	page -> lock.flag = 0;
}

page_t* init_slab(int id, int kd, int sz) {
	uintptr_t now = (uintptr_t)slow_alloc(PAGE_SIZE);
	if (now == 0) return NULL;
	now = now - PAGE_SIZE;
	page_t *page  = (page_t *)now; 
	init_page(page, id, kd, sz, 0);
	now = now + PAGE_SIZE;
	for (uintptr_t i = now; i < now + PAGE_SIZE; i += slot[sz])
		page -> stack[page->cnt++] = i;
	return page;
}

static int tot = 0;
void *fast_alloc(int id, int kd) {
	page_t* page = NULL;
	page = page_table[id][kd];
	spinlock(&page -> lock);
	while (page -> cnt == 0) {
		if (page -> next == NULL) break;	
		spinunlock(&page -> lock);
		page = page -> next;
		spinlock(&page -> lock);
	}
	assert(page);
	if (page -> cnt == 0) {
		assert(page -> next == NULL);
		page -> next = init_slab(id, 0, kd);
		spinunlock(&page -> lock);
		page = page -> next;
		if (page != NULL) spinlock(&page -> lock);
	}
	if (page == NULL) return NULL;
	uintptr_t t = page -> stack[--page -> cnt];
	spinunlock(&page -> lock);
	return (void *)t;
}

static void *kalloc(size_t size) {
  if ((size >> 20) >= 16) return NULL;
  int id = cpu_current(), bj = 0;
  void *space;
  for (int i = 0; i < MAX_DATA_SIZE; i++) {
	  if (size <= slot[i]) {
		space = fast_alloc(id, i);
		bj = 1;
		break;  
	  }
  }
  if (!bj) {
	  space = slow_alloc(size);
  }
  return space;
}

void fast_free(page_t *page, void *ptr) {
	assert(page);
	if (page -> special == 0) {
		spinlock(&page -> lock);
		page -> stack[page -> cnt ++] = (uintptr_t) ptr;
		spinunlock(&page -> lock);
	}
	else assert(0);
}

void slow_free(uintptr_t left, uintptr_t right) {
	int now = List -> head;
	assert(now);
	if (right <= List -> l[now]) {
		if (right == List -> l[now]) List -> l[now] = left;
		else {
			assert(List -> sum);
			int id = List -> valid[-- List -> sum];
			List -> l[id] = left, List -> r[id] = right;	
			List -> next[id] = now;
			List -> head = id;
		}	
	}
	else {
		int nxt = 0;
		while (now) {
			if (List -> r[now] <= left) {
				nxt = List -> next[now];
				assert(nxt);
				if (List -> l[nxt] >= right) break;	
			}	
			now = List -> next[now];
		}	
		assert(now);
		int bj = 0;
		if (List -> r[now] == left) bj = 1, List -> r[now] = right;
	    else if (List -> l[nxt] == right) bj = 1, List ->l[nxt] = left;
		if (bj) {
			if (List -> r[now] == List -> l[nxt]) {
				List -> next[now] = List -> next[nxt];
				List -> r[now]    = List -> r[nxt];
				List -> l[nxt] = List -> r[nxt] = List -> next[nxt] = 0;
				List -> valid[List -> sum++] = nxt;	
			}	
		}
		else {
			assert(List -> sum);
			int id = List -> valid[--List -> sum];
			List -> l[id] = left, List -> r[id] = right;
			List -> next[id] = nxt, List -> next[now] = id;	
		}
	}	
}

static void kfree(void *ptr) {
	uintptr_t now = ((uintptr_t)ptr & (~(PAGE_SIZE - 1)));
	page_t *page = (page_t *)(now - PAGE_SIZE);
	assert(page);
	if (page -> type == 1) {
		fast_free(page, ptr);
	}
	else if (page -> type == 2) {
		spinlock(&lock_all);	
		slow_free(now - PAGE_SIZE, page -> end);
		spinunlock(&lock_all);
	}
	else assert(0);
}

static void pmm_init() {
  lock_all.flag = 0;

  assert(sizeof(page_t) <= 4096);
  assert(sizeof(slot) / sizeof(int) == MAX_DATA_SIZE);
  #ifndef TEST
  st = ROUNDUP(heap.start, PAGE_SIZE);
  #else
  char *ptr = malloc(Heap_Size);
  assert(ptr != NULL);
  st = (uintptr_t)ptr;
  st = ROUNDUP(st, PAGE_SIZE);
  heap.end   = (void *) st + Heap_Size;
  #endif	
 
  
  List = (List_t *)st;
  st = ROUNDUP(st + sizeof(List_t), PAGE_SIZE);

  tot = cpu_count();
  
  init_list();  
  for (int i = 0; i < tot; i++) 
	  for (int j = 0; j < MAX_DATA_SIZE - 1; j++) {
	  page_table[i][j] = init_slab(i, 1, j);
	  assert(page_table[i][j]);
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
