#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#define N 65536

char *exec_argv[N] = {"strace", "-T", "-o"};
char *exec_envp[]  = { "PATH=/bin", NULL};

char buf[N];
int loc = 0;

void record() {
			
	
}


int main(int argc, char *argv[]) {
  int fd[2];
  if (pipe2(fd, O_NONBLOCK) != 0) assert(0);
  int pid = fork();
  if (pid == 0) {

	int file = open("/dev/null", 0);
	assert(file > 0);
	dup2(file, 1);
	dup2(file, 2);
	close(fd[0]);

	char tep_argv[100];
	int id = getpid();
    sprintf(tep_argv, "/proc/%d/fd/%d", id, fd[1]);
	exec_argv[3] = tep_argv;
	for (int i = 1; i < argc; i++) exec_argv[i + 3] = argv[i];
	int now = 0;
	while(exec_argv[now] != NULL)
		printf("%s\n". exec_argv[now]), now++;
	exec_argv[argc + 1] = NULL;
	execve("strace",          exec_argv, exec_envp);
	execve("/bin/strace",     exec_argv, exec_envp);
	execve("/usr/bin/strace", exec_argv, exec_envp);
	perror(argv[0]);
	exit(EXIT_FAILURE);
  }

  else {
	close(fd[1]);
	char s;
	int cnt = 0;
	while(waitpid(pid, NULL, WNOHANG) == 0) {
		int cnt = read(fd[0], &s, 1);
		if (cnt > 0) {
			buf[loc++] = s;
			if (s == '\n') {	
				buf[loc] = '\0';
				printf("%s", buf);
				record();
				loc = 0;
			}
		}
	}
	close(fd[0]);
	return 0;	  
  }
}
