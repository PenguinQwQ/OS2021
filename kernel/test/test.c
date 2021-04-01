#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp  1 
#define MAXN 100000

static int ttid[smp], sum = 0;

int cpu_current() {
	for (int i = 0; i < smp; i++) 
		if (syscall(SYS_gettid) == ttid[i]) return i;	
	assert(0);
}

int cpu_count() {
	return smp;	
}

struct node{
	uintptr_t l;
	uintptr_t r;
	int size;
}cpu[MAXN * smp];

int compare(const void* w1, const void* w2) {
	struct node* t1 = (struct node *)w1;
	struct node* t2 = (struct node *)w2;
	return t1->l > t2->l;
}

int cnt;

void record_alloc(int sz, void *space) {
	if (space == NULL) return;
	lock();
	cpu[cnt].l = (uintptr_t)space;
	cpu[cnt].size = sz;
	cpu[cnt].r = (uintptr_t)space + sz;
	cnt++;
	unlock();
}

void finish() {
	qsort(cpu, cnt, sizeof(struct node), compare);
	for (int i = 0; i < cnt - 1; i++)
		if ( !(cpu[i].l < cpu[i + 1].l  && \
		cpu[i].r <= cpu[i + 1].l        && \
		cpu[i].l <= cpu[i].r)){
			printf("%d\n", i);
			printf("%p %p %d\n",cpu[i].l, cpu[i].r, cpu[i].size);
			printf("%p %p %d\n",cpu[i + 1].l, cpu[i + 1].r, cpu[i + 1].size);
			return;
		}
	printf("Test01 Success!\n");	
}

void task1() { // smoke task
	for (int i = 0; i < MAXN; i++) {
		int p = rand() % 10, sz;
		if (p <= 5)      sz = rand() % 128 + 1;
		else if (p <= 8) sz = 4096;
		else             sz = (rand() & ((16 << 20) - 1)) + 1;
		void *tep = pmm -> alloc(sz);
		record_alloc(sz, tep);
	}	
}

void entry(int tid) {
	lock();
	ttid[sum++] = syscall(SYS_gettid);
	unlock();
	task1();
	join(finish);
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
