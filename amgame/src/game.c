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

	
  int next_frame = 0, t = 0;
  while (1) {
	while(io_read(AM_TIMER_UPTIME).us / 1000 < next_frame);
    next_frame += 1000 / FPS;
	test_hit();
	if (t++ % 3 == 0)update_obj();
	int key = 0;
	while ((key = read_key()) != AM_KEY_NONE) {
		if (key == AM_KEY_ESCAPE) halt(0);
		if (key == AM_KEY_HOME)     update_player2(-1);
		else if (key == AM_KEY_END) update_player2(1);
		else if (key == AM_KEY_A)   update_player1(-1);
		else if (key == AM_KEY_D)   update_player1(1);
	}
	update_screen();
  }
  return 0;
}
