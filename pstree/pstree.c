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
bool flag[3] = {false, false, false};
char arg[6][30] = {"-p", "-n", "-V", 
						"--show-pids", "--numeric-sort","--version"};

struct process{
	char name[N];
	pid_t pid;
	pid_t ppid;	
}e[N];


void solve(int now, int dep) {
	for (int i = 1; i <= dep; i++) printf("\t");
	int len = strlen(e[now].name) - 1;
	for (int i = 1; i < len; i++) printf("%c", e[now].name[i]);
	if (flag[0] == true) printf("(%d)", e[now].pid);
	printf("\n");
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

void error_message(char *tep) {
	fprintf(stderr, "invalid option -- '%s'\n", tep);	
}

void version_message() {
	fprintf(stderr, "pstree (PSmisc) UNKNOWN\nCopyright (C) 1993-2017 Werner Almesberger and Craig Small\n\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under\nthe terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING.\n");	
}
int main(int argc, char *argv[]) {

  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
	if (i == 0) continue;
	bool bj = false;
	for (int j = 0; j < 6; j++) {
		if (strcmp(argv[i], arg[j]) == 0) {
			bj = true;
			flag[j % 3] = true;	
		}
	}

	if (bj == false) {
		error_message(argv[i]);
		return 0;
	}

	if (flag[2] == true) {
		version_message();
		return 0;	
	} 
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
	
	char tep;
	fscanf(fp, "%d %s %c %d", &e[sum].pid, e[sum].name, &tep, &e[sum].ppid);
	/*
	if(check_parentheses(e[sum].name) == false) {
		fscanf(fp, "%d %s%s %c %d", &e[sum].pid, e[sum].name
		                                ,buff, &tep, &e[sum].ppid);
		strcat(e[sum].name, buff);	
		printf("%d %s\n", e[sum].pid, buff);
	}

	printf("%d %s %d\n", e[sum].pid, e[sum].name, e[sum].ppid);
	*/

	fclose(fp);
  }
  solve(1, 0);
  return 0;
}

