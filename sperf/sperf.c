#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
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
	exit(0);
	execve("strace",          exec_argv, exec_envp);
	execve("/bin/strace",     exec_argv, exec_envp);
	execve("/usr/bin/strace", exec_argv, exec_envp);
	perror(argv[0]);
	exit(EXIT_FAILURE);
  }
  else {
	close(fd[1]);
	while(waitpid(pid, NULL, WNOHANG) == 0) {
		int cnt = read(fd[0], buf, sizeof(buf));
		printf("%d\n", cnt);
		if (cnt >= 0) buf[cnt] = 0;
		if (cnt > 0) printf("%s", buf);
	}
	return 0;	  
  }
}
