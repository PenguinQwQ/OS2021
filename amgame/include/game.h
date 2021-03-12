#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

struct object{
	int x, y, v_x, v_y;
	};

struct baffle{
	int start;
	};

extern struct object obj;
extern struct baffle player1, player2;

void splash();
void init_location();
void update_obj();
int read_key();
void update_player1(int dir);
void update_player2(int dir);
void update_screen();
static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}
