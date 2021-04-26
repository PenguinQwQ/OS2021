#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#define N 65536

char *exec_argv[N] = {"strace", "-T"};
char *exec_envp[]  = { "PATH=/bin", NULL};

char buf[N];
int loc = 0;

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) exec_argv[i + 1] = argv[i];
  exec_argv[argc + 1] = NULL;
  int fd[2];
  if (pipe2(fd, O_NONBLOCK) != 0) assert(0);
  dup2(fd[1], 2);
  int pid = fork();

  if (pid == 0) {
	close(fd[0]);
	execve("strace",          exec_argv, exec_envp);
	execve("/bin/strace",     exec_argv, exec_envp);
	execve("/usr/bin/strace", exec_argv, exec_envp);
	perror(argv[0]);
	exit(EXIT_FAILURE);
  }

  else {
	close(fd[1]);
	char s;
	loc = 0;

	while(waitpid(pid, NULL, WNOHANG) == 0) {
		int cnt = read(fd[0], &s, 1);
		if (cnt > 0) {
			buf[loc++] = s;
			if (s == '\n') {	
				buf[loc] = '\0';
				printf("%s", buf);
			}
			else loc = 0;;
		}
	}
	close(fd[0]);
	return 0;	  
  }
}
