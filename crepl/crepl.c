#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <stdbool.h>
#define N 4096

static char cname[N];
static char sname[N];
static char line[N];
static char complete[N << 1];
static char expr[N];
static char jud[N];
static bool flag;

int T = 0;

void judge() {
	sscanf(line, "%s", jud);
	if (strcmp(jud, "int") == 0) {
		flag = false;
		strcpy(complete, line);
	}
	else {
		flag = true;	
		T++;
		if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';
		sprintf(complete, "int __expr_wrapper_%d(){ return %s;}", T, line);
		sprintf(expr, "__expr_wrapper_%d", T);
    }	
}

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
	judge();
	write(fd, complete, strlen(complete));
	close(fd);
	unlink(filename_template);
}

char *exec_argv[16] = {"gcc", "-fPIC", "-shared"};

bool compile() {
	if (sizeof(int *) == 4) exec_argv[3] = "-m32";
	else exec_argv[3] = "-m64";
	int p = 4;
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
			printf("compile error!\n");
			return false;
		}
		if(flag == false) printf("Add Sucessfully!\n");
		return true;
	}
	return true;
}

void dlink() {
	int (*func)();
	void *handle;
	handle = dlopen(sname, RTLD_LAZY | RTLD_GLOBAL);
	assert(handle != NULL);
	if (flag == false) return;
	func = dlsym(handle, expr);
	if (func) printf("(%s) == %d.\n", line, func());
}

int main(int argc, char *argv[]) {
  int file = open("/dev/null", 0);
  assert(file > 0);
  dup2(file, 2);
  while (1) {
    printf("crepl> ");
    fflush(stdout);
	flag = false;
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
	makedoc();
	if(compile() == false) continue;
	dlink();
  }
  return 0;
}
