#include <game.h>

// Operating system is a C program!
struct object{
	int x, y, v;
}obj;
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();
  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}
