#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#define N 65536

char *exec_argv[N] = {"strace", "-T"};
char *exec_envp[]  = { "PATH=/bin", NULL};
int fildes[2];

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) exec_argv[i + 1] = argv[i];
  exec_argv[argc + 1] = NULL;
  if (pipe(fildes) != 0) assert(0);
  int pid = fork();
  if (pid == 0) {
	execve("strace",          exec_argv, exec_envp);
	execve("/bin/strace",     exec_argv, exec_envp);
	execve("/usr/bin/strace", exec_argv, exec_envp);
	perror(argv[0]);
	exit(EXIT_FAILURE);
  }
  else {
	  
	printf("212\n");	  
	return 0;	  
  }
}
