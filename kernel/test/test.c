#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp  8
#define MAXN 100

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
	int l;
	int r;
}cpu[MAXN * smp];

int cnt;

void record_alloc(int sz, void *space) {
	cpu[cnt].l = (uintptr_t)space;
	cpu[cnt].r = (uintptr_t)space + sz;
	cnt++;
}

void finish() {
	printf("%d\n", cnt);	
	
}

void task1() { // smoke task
	for (int i = 0; i < MAXN; i++) {
		int p = rand() % 10, sz;
		p = 5;
		if (p <= 5)      sz = rand() % 128;
		else if (p <= 8) sz = 4096;
		else             sz = rand() & ((16 << 20) - 1);
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
