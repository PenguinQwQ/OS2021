#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/wait.h>

static char cname[16];
static char sname[16];
static char line[4096];

void makedoc() {
    char filename_template[] = "/tmp/XXXXXX";
	int fd = mkstemp(filename_template);
	assert(fd > 0);
	strcpy(cname, filename_template);
	int p = 11;
	cname[p] = '.', cname[p + 1] = 'c', cname[p + 2] = '\0';
	rename(filename_template, cname);
	strcpy(sname, filename_template);
	sname[p] = '.', sname[p + 1] = 's', sname[p + 2] = 'o', sname[p + 3] = '\0';
	write(fd, line, strlen(line));
	close(fd);
}

char *exec_argv[16] = {"gcc", "-fPIC", "-shared"};

void compile() {
	exec_argv[3] = cname;
	exec_argv[4] = "-o";
	exec_argv[5] = sname;
	int status;
	int pid = fork();
	if (pid == 0) execvp("gcc", exec_argv);
	else {
		wait(&status);	
		if (WIFEXITED(status)) printf("Illegal expression!\n");
	}
}

void dlink() {
	int (*func)();
	void *handle;
	handle = dlopen(sname, RTLD_NOW);
	assert(handle != NULL);
	func = dlsym(handle, "lq");
	if (func) printf("%d\n", func());
}

int main(int argc, char *argv[]) {
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
	makedoc();
	compile();
	dlink();
  }
  return 0;
}
