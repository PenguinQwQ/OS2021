#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char *exec_argv[] = { "strace -T", "echo", "hello", NULL};
  char *exec_envp[] = { "PATH=/bin", NULL, };
  execve("strace -T",          exec_argv, exec_envp);
  execve("/bin/strace -T",     exec_argv, exec_envp);
  execve("/usr/bin/strace -T", exec_argv, exec_envp);
  perror(argv[0]);
  exit(EXIT_FAILURE);
}
