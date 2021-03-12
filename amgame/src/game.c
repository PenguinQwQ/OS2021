#include <game.h>

#define FPS 30
#define init_speed 5
#define BLUE \\033[1;34;40m

struct object obj[10];
struct baffle player1, player2;

int main(const char *args) {
  ioe_init();
  puts("\033[1;34;40m Welcome to Pinball Game!\033[0m\n");
  puts("\033[1;34;40m Player1 operator the baffle with 'A' and 'D'.\033[0m\n");
  puts("\033[1;34;40m Player2 operator the baffle with 'Left' and 'Right'.\033[0m\n");
  puts("\033[1;34;40m Type 'P' to speed up, and 'L' to speed down.\033[0m\n");
  puts("\033[1;34;40m Type 'Enter' to increase the number of Pinballs\033[0m\n");
  puts("\033[1;34;40m Type 'M' to restart the game\n");
  puts("\033[1;34;40m Type 'ESC' to exit\n");
  splash();
  init_location();

	
  int next_frame = 0, t = 0, speed = init_speed;
  while (1) {
	while(io_read(AM_TIMER_UPTIME).us / 1000 < next_frame);
    next_frame += 1000 / FPS;
	test_hit();
	t++;
	if (t >= 30 && t % speed == 0)update_obj();
	int key = 0;
	while ((key = read_key()) != AM_KEY_NONE) {
		if (key == AM_KEY_ESCAPE) halt(0);
		if (key == AM_KEY_HOME)     update_player2(-1);
		else if (key == AM_KEY_END) update_player2(1);
		else if (key == AM_KEY_A)   update_player1(-1);
		else if (key == AM_KEY_D)   update_player1(1);
		else if (key == AM_KEY_P && speed > 1) speed -= 1;
		else if (key == AM_KEY_L) speed += 1;
		else if (key == AM_KEY_M) {
			speed = init_speed, t = 0;
		    splash(), init_location();
			next_frame = io_read(AM_TIMER_UPTIME).us / 1000; 
		}
		else if (key == AM_KEY_RETURN) add_object();
	}
	update_screen();
	test_bound();
  }
  return 0;
}
