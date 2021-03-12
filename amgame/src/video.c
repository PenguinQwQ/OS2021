#include <game.h>

#define SIDE 8
#define COL_PURPLE 0x2a0a29
#define COL_WHITE  0xeeeeee
#define COL_RED    0xff0033
#define COL_GREEN  0x00cc33
#define COL_BLUE   0x0000ff
#define length 6
static int w, h, block_size, crash = 0, num;
uint32_t texture[128][128];

int dx[4] = {1, 1, -1, -1};
int dy[4] = {1, -1, 1, -1};

extern struct object obj[10];

extern struct baffle player1, player2;

static void init() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width / SIDE;
  h = info.height / SIDE;
  block_size = SIDE * SIDE;
}

/*
static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}
*/

void update_screen() {
	AM_GPU_FBDRAW_T event;
	uint32_t pixels[SIDE * SIDE];
	for (int i = 0; i < w; i++)
		for (int j = 0; j < h; j++) {
			for (int k = 0; k < block_size; k++) {
				pixels[k] = texture[i][j];
			}
			int up = (crash) ? 500: 1;
			for (int k = 0; k < up; k++) {
				event.x = i * SIDE, event.y = j * SIDE,
				event.w = SIDE, event.h = SIDE,
				event.sync = 1,
				event.pixels = pixels;
				ioe_write(AM_GPU_FBDRAW, &event);
			}
		}
}

void splash() {
  num = 1;
  crash = 0;
  init();
  for (int i = 0; i < w; i++)
	for (int j = 0; j < h; j++)
		texture[i][j] = COL_PURPLE;
  update_screen();
}

void init_location() {
	for (int i = 0; i < num; i++) {
		obj[i].x = w / 2, obj[i].y = h / 2;
		obj[i].v_x = dx[rand() % 4], obj[i].v_y = dy[rand() % 4];
		texture[obj[i].x][obj[i].y] = COL_WHITE;
	}
	player1.start = w / 2 - (length / 2);
	player2.start = w / 2 - (length / 2);
	for (int i = player1.start; i < player1.start + length; i++) {
		texture[i][0] = COL_GREEN;	
	}
	for (int i = player2.start; i < player2.start + length; i++) {
		texture[i][h - 1] = COL_GREEN;	
	}
	for (int i = 0; i < h; i++) texture[w - 1][i] = COL_RED;
	for (int i = 0; i < h; i++) texture[0][i]     = COL_RED;
	update_screen();
}

void update_obj() {
	if (crash) return;
	for (int i = 0; i < num; i++) {
		texture[obj[i].x][obj[i].y] = COL_PURPLE;
		obj[i].x += obj[i].v_x, obj[i].y += obj[i].v_y;
		texture[obj[i].x][obj[i].y] = COL_WHITE;
		printf("%d %d \n", i, obj[i].y);
	}
	update_screen();
}

void update_player1(int dir) {
	if (crash) return;
	if (dir == 1 && player1.start + length - 1 < w - 2) {
		texture[player1.start][0] = COL_PURPLE;
		texture[player1.start + length][0] = COL_GREEN;
		player1.start += 1;
	}
	else if (dir == -1 && player1.start > 1){
		texture[player1.start + length - 1][0] = COL_PURPLE;
		texture[player1.start - 1][0] = COL_GREEN;
		player1.start -= 1;
	}
}

void update_player2(int dir) {
	if (crash) return;
	if (dir == 1 && player2.start + length - 1 < w - 2) {
		texture[player2.start][h - 1] = COL_PURPLE;
		texture[player2.start + length][h - 1] = COL_GREEN;
		player2.start += 1;
	}
	else if (dir == -1 && player2.start > 1){
		texture[player2.start + length - 1][h - 1] = COL_PURPLE;
		texture[player2.start - 1][h - 1] = COL_GREEN;
		player2.start -= 1;
	}
}

void test_hit() {
	if (crash) return;
	for (int i = 0; i < num; i++) {
		if (texture[obj[i].x + obj[i].v_x][obj[i].y + obj[i].v_y] != COL_PURPLE) {
			if (obj[i].x + obj[i].v_x == 0 || obj[i].x + obj[i].v_x == w - 1) 
				obj[i].v_x *= -1;
			if (obj[i].y + obj[i].v_y == 0 || obj[i].y + obj[i].v_y == h - 1) 
				obj[i].v_y *= -1;		
		}
	}
}

void test_bound() {
	if (crash) return;
	for (int i = 0; i < num; i++) 
		if (obj[i].y == 0 || obj[i].y == h - 1) {
			crash = 1;
			for (int i = 0; i < w; i++)
				for (int j = 0; j < h; j++)
					texture[i][j] = COL_BLUE;
			break;
		}
	update_screen();
}

void add_object() {
	if (num == 9) {
		printf("The objects can not be more!");
		return;
	}	
	num++;
	obj[num].x = w / 2, obj[num].y = h / 2;
	obj[num].v_x = dx[rand() % 4], obj[num].v_y = dy[rand() % 4];
	texture[obj[num].x][obj[num].y] = COL_WHITE;

}
