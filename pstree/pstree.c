#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char path[256] = {"/proc/"};

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
	printf("d_name: %s\n", ptr->d_name);
  }
  return 0;
}
