#include <game.h>

#define FPS 30

struct object obj;
struct baffle player1, player2;

int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();
  init_location();
  puts("Type 'ESC' to exit\n");

 // int next_frame = 0;
	
  int next_frame = 0, t = 0;
  while (1) {
	while(io_read(AM_TIMER_UPTIME).us / 1000 < next_frame);
    next_frame += 1000 / FPS;
	if (t++ % 15 == 0)update_obj();
	int key = 0;
	while ((key = read_key()) != AM_KEY_NONE){
		if (key == AM_KEY_ESCAPE) halt(0);
	}
	update_obj();
	
  }
  return 0;
}
