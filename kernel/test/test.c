#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp  8
#define MAXN 50000

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
	size_t size;
	struct node* nxt;
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

int ti = 0;

void finish() {
	qsort(cpu, cnt, sizeof(struct node), compare);
	for (int i = 0; i < cnt - 1; i++)
		if ( !(cpu[i].l < cpu[i + 1].l  && \
		cpu[i].r <= cpu[i + 1].l        && \
		cpu[i].l <= cpu[i].r)           && \
		cpu[i].r - cpu[i].l >= cpu[i].size){
			printf("%d\n", i);
			printf("%p %p %d\n",cpu[i].l, cpu[i].r, cpu[i].size);
			printf("%p %p %d\n",cpu[i + 1].l, cpu[i + 1].r, cpu[i + 1].size);
			return;
		}
	printf("%d %d\n", cnt, ti);
	printf("Test01 Success!\n");	
}

void task1() { // smoke task
	for (int i = 0; i < MAXN; i++) {
		int p = rand() % 15, sz, bj = 0;;
		if (p <= 5)      sz = rand() % 128 + 1;
		else if (p <= 8) sz = 4096;
		else if (p <= 9) sz = (rand() & ((16 << 20) - 1)) + 1;
		else {
			if (cnt == 0) continue;
			bj = 1;
			int id = rand() % cnt;
			lock();
			if (cpu[id].l) ti++, \
				pmm -> free((void *)cpu[id].l),	cpu[id].l = cpu[id].r = 0;
			unlock();
		}
		if (bj == 0) {
			void *tep = pmm -> alloc(sz);
			record_alloc(sz, tep);
		}
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
	srand(time(0));
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
