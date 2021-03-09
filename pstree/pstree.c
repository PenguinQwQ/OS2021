#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define N 1024

char path[N] = {"/proc/"};
char finalpath[N];
char buff[N];

struct process{
	char name[N];
	pid_t pid;
	pid_t ppid;	
}e[N];

 
int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  DIR *dir = opendir(path);
  assert(dir != NULL);
  struct dirent *ptr;
  int sum = 0;
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
	fscanf(fp, "%d %s %c %d", &e[sum].pid, e[sum].name, &tep,&e[sum].ppid);
	printf("%d %s %d\n", e[sum].pid, e[sum].name, e[sum].ppid);
	fclose(fp);
  }
  printf("%d\n", sum);
  return 0;
}
