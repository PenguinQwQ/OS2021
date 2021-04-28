#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    char filename_template[] = "./temp_file.XXXXXX";
	int fd = mkstemp(filename_template);
	assert(fd > 0);
	write(fd, line, sizeof(line));
	close(fd);
  }
  return 0;
}
