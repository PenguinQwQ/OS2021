#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char path[256] = {"/proc/"};
char finalpath[256];

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
	strcat(finalpath, "/status");
	FILE *fp;
	fp = fopen(finalpath, "r");
	fread(path, 256, 1, fp);
	printf("%s\n", path);
	fclose(fp);
	break;	
  }
  return 0;
}
