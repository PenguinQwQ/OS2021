#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
  static char line[4096];
  static char name[16];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    char filename_template[] = "XXXXXX";
	int fd = mkstemp(filename_template);
	assert(fd > 0);
	strcpy(name, filename_template);
	name[6] = '.', name[7] = 'c', name[8] = '\0';
	rename(filename_template, name);
	write(fd, line, strlen(line));
	close(fd);
  }
  return 0;
}
