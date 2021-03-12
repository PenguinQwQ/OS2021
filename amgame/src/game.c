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
	t++;
	if (t % 30 == 0) printf("%d\n", 1);

//	while(current < frames) current++;

//	update_obj();
	
	print_key();
  }
  return 0;
}
