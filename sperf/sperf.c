#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#define N 65536

char *exec_argv[N] = {"strace", "-T"};
char *exec_envp[]  = { "PATH=/bin", NULL};
char buf[N];

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) exec_argv[i + 1] = argv[i];
  exec_argv[argc + 1] = NULL;
  int fd[2];
  if (pipe(fd) != 0) assert(0);
  dup2(fd[1], 2);
  int pid = fork();
  if (pid == 0) {
	close(fd[0]);
	fprintf(stderr, "1\n");return 0;
	execve("strace",          exec_argv, exec_envp);
	execve("/bin/strace",     exec_argv, exec_envp);
	execve("/usr/bin/strace", exec_argv, exec_envp);
	perror(argv[0]);
	exit(EXIT_FAILURE);
  }
  else {
	close(fd[1]);
	while(kill(pid, 0) == 0) {
		int cnt = read(fd[0], buf, sizeof(buf));
		if (cnt > 0) printf("%s", buf);
		printf("%d\n", pid);			
	}
	return 0;	  
  }
}
