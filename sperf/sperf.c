#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#define N 65536
#define M 128

char *exec_argv[N] = {"strace", "-T", "-o"};
char *exec_envp[]  = { "PATH=/:/usr/bin:/bin", NULL};

char buf[N];
int loc = 0, tot = 0;

struct node{
	char name[M];
	double time;	
}List[N];

struct node *head = NULL;

void record() {
	char now_name[M];
	double now_time = 0;	
	sscanf(buf, "%[a-zA-Z]", now_name);
	char *tep;
	tep = NULL;
	int len = strlen(buf), flag = 0;
	for (int i = len - 1; i >= 0; i--)
		if (buf[i] == '<' || buf[i] == '>' || buf[i] == ' ' || buf[i] == '.' || \
			buf[i] == '\n' || (buf[i] <= '9' && buf[i] >= '0')) {
				if (buf[i] == '<') {
					tep = &buf[i + 1];
					break;
				}
			}
		else return;

	sscanf(tep, "%lf", &now_time);
	
	for (int i = 0; i < tot; i++)
		if (strcmp(List[i].name, now_name) == 0) {
			flag = 1;
			List[i].time += now_time;
			break;	
		}
	if (flag == 0) {
		strcpy(List[tot].name, now_name);
		List[tot].time = now_time;
		tot++;	
	}
}

int compare(const void *w1, const void* w2) {
	struct node *t1 = (struct node *)w1;
	struct node *t2 = (struct node *)w2; 
	return t1 -> time < t2 -> time;
}

int ti = 0;

void show_result() {
	ti++;
	printf("Time #%d\n", ti);
	qsort(List, tot, sizeof(struct node), compare);
	double tot_time = 0;
	int ratio;	
	for (int i = 0; i < tot; i++) tot_time += List[i].time;
	for (int i = 0; i < (tot > 5 ? 5 : tot); i++) {
		ratio = List[i].time * 100.0 / tot_time;
		printf("%s (%d%%)\n", List[i].name, ratio);	
	}
	tot = 0;	
	for (int i = 0; i < 80; i++) printf("%c", '\0');
	printf("\n");
	for (int i = 0; i < 10; i++) printf("=");
	printf("\n");
	fflush(stdout);
}
char e[N];
int main(int argc, char *argv[], char *envp[]) {
  int fd[2];
  if (pipe2(fd, O_NONBLOCK) != 0) assert(0);
  int pid = fork();
  if (pid == 0) {
	int file = open("/dev/null", 0);
	assert(file > 0);
//	dup2(file, 1);
//	dup2(file, 2);
	close(fd[0]);
	char tep_argv[100];
	int id = getpid();
    sprintf(tep_argv, "/proc/%d/fd/%d", id, fd[1]);
	exec_argv[3] = tep_argv;
	for (int i = 1; i < argc; i++) exec_argv[i + 3] = argv[i];
	exec_argv[argc + 3] = NULL;
//	execve("strace",          exec_argv, exec_envp);
//	execve("/bin/strace",     exec_argv, exec_envp);
//	execve("/usr/bin/strace", exec_argv, exec_envp);
	char tmp[1024];
	char tmp2[] = {"/strace"};
	int i = 0;
	while (envp[i] != NULL) {
		strcpy(tmp, "PATH");
		int bj = 0;
		for (int j = 0; j < 4; j++) 
			if (tmp[j] != envp[i][j]) {
				bj = 1; break;	
			}
		if(bj == 0) continue;
		int current = 0;
		for (int j = 4; j < strlen(envp[i]); j++) 
			if (envp[i][j] == ':' || envp[i][j] == ' ' || envp[i][j] == '\n' \
			||  envp[i][j] == '\0') {
				strcat(tmp, tmp2);
				printf("%s\n", tmp);
				current = 0;
			}
			else tmp[current++] = envp[i][j];
		i++;
	}
	
	printf("666\n");
	perror(argv[0]);
	exit(EXIT_FAILURE);
  }

  else {
	  return 0;
	close(fd[1]);
	char s;
	int cnt = 0;
	int lst_time = 0;
	while((cnt = read(fd[0], &s, 1)) > 0 ||waitpid(pid, NULL, WNOHANG) == 0) {
		if (cnt > 0) {
			buf[loc++] = s;
			if (s == '\n') {
				buf[loc] = '\0';
				record();
				loc = 0;
			}
		}
		int now = clock() / CLOCKS_PER_SEC;
		if (now > lst_time) lst_time = now, show_result();
	}
	close(fd[0]);
	show_result();
	return 0;	  
  }
}
