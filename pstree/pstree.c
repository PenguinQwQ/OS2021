#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define N 1024
#define M 1000000

char path[N] = {"/proc/"};
char finalpath[N];
char buff[N];
int sum = 0;

struct process{
	char name[N];
	pid_t pid;
	pid_t ppid;	
}e[N];


void solve(int now, int dep) {
	for (int i = 1; i <= dep; i++) printf("\t");
	printf("%s(%d)\n", e[now].name, e[now].pid);
	for (int i = 1; i <= sum; i++) {
		if (e[i].ppid == e[now].pid) solve(i, dep + 1);	
	}
	return;
}
 
bool check_parentheses(char *tep) {
	int len = strlen(tep), st = 0;
	for (int i = 0; i < len; i++) 
		if (tep[i] == '(') st++;
		else if (tep[i] == ')') st--;
	if (st == 0) return true;
	else return false;
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  DIR *dir = opendir(path);
  assert(dir != NULL);
  struct dirent *ptr;
  while ((ptr = readdir(dir)) != NULL) {
	int len = strlen(ptr->d_name);
	bool judge = true;
	for (int i = 0; i < len; i++) {
		if (ptr->d_name[i] < '0' || ptr->d_name[i] > '9') {
			judge = false;
			break;
		}	
	}
	if (judge == false) continue;
	finalpath[0] = '\0';
	strcat(finalpath, path);
	strcat(finalpath, ptr->d_name);
	strcat(finalpath, "/stat");
	FILE *fp;
	fp = fopen(finalpath, "r");
	assert(fp != NULL);
	sum = sum + 1;
	fgets(buff, 20, fp);
	printf("%s\n", buff);
	/*
	char tep;
	fread(fp, "%d %s", &e[sum].p、id, e[sum].name);
	while(check_parentheses(e[sum].name) == false) {
		fscanf(fp, "%s", buff);
		strcat(e[sum].name, buff);	
	}
	fscanf(fp, "%c %d", &tep, &e[sum].ppid);
	*/

	//printf("%d %s %d\n", e[sum].pid, e[sum].name, e[sum].ppid);
	fclose(fp);
  }
  solve(1, 0);
  return 0;
}












