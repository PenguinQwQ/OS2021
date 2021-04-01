#include <common.h>
#include <threads.h>
#include <unistd.h>
#include <sys/syscall.h>
#define smp 8

static int ttid[smp], sum = 0;

int cpu_current() {
	return syscall(SYS_gettid);	
}

int cpu_count() {
	return smp;	
}

void entry(int tid) {
	ttid[sum++] = cpu_current();
	printf("%d\n", sum);	
} 

int main(int argc, char *argv[]) {
	pmm -> init();
	for (int i = 0; i < smp; i++)
		create(entry);	
	return 0;
}
