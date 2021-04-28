#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

static char cname[16];
static char sname[16];
static char line[4096];

void makedoc() {
    char filename_template[] = "XXXXXX";
	int fd = mkstemp(filename_template);
	assert(fd > 0);
	strcpy(cname, filename_template);
	cname[6] = '.', cname[7] = 'c', cname[8] = '\0';
	rename(filename_template, name);
	strcpy(sname, filename_template);
	sname[6] = '.', sname[7] = 's', sname[8] = 'o', sname[9] = '\0';
	write(fd, line, strlen(line));
	close(fd);
}

char *exec_argv[16] = {"gcc", "-fPIC", "-shared"};

void compile() {
	exec_argv[3] = cname;
	exec_argv[4] = "-o";
	exec_argv[5] = sname;
	int pid = fork();
	if (pid == 0) execvp("gcc", exec_argv);
	else wait(NULL);	
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
  }
  return 0;
}
