#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <stdbool.h>

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

bool compile() {
	int p = 3;
	exec_argv[p] = cname;
	exec_argv[p + 1] = "-o";
	exec_argv[p + 2] = sname;
	int status;
	int pid = fork();
	if (pid == 0) {
		execvp("gcc", exec_argv);
		assert(0);
	}
	else {
		wait(&status);
		if (status) {
			printf("Illegal expression!\n");
			return false;
		}
		return true;
	}
	return true;
}

void dlink() {
	int (*func)();
	void *handle;
	handle = dlopen(sname, RTLD_GLOBAL);
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
	if(compile() == false) continue;
	dlink();
  }
  return 0;
}
