#include <game.h>

#define SIDE 8
#define COL_PURPLE 0x2a0a29
#define COL_WHITE  0xeeeeee
#define length 2
static int w, h, block_size;
uint32_t texture[128][128];

extern struct object obj;

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
			event.x = i * SIDE, event.y = j * SIDE,
			event.w = SIDE, event.h = SIDE,
			event.sync = 1,
			event.pixels = pixels;
			ioe_write(AM_GPU_FBDRAW, &event); 	
		}
}

void splash() {
  init();
  for (int i = 0; i < w; i++)
	for (int j = 0; j < h; j++)
		texture[i][j] = COL_PURPLE;
  update_screen();
}

void init_location() {
	obj.x = w / 2, obj.y = h / 2;
	obj.v_x = 1, obj.v_y = 1;
	texture[obj.x][obj.y] = COL_WHITE;
	player1.start = w / 2 - (length / 2);
	player2.start = w / 2 - (length / 2);
	for (int i = player1.start; i < player1.start + length; i++) {
		texture[i][0] = COL_WHITE;	
	}
	for (int i = player2.start; i < player2.start + length; i++) {
		texture[i][h - 1] = COL_WHITE;	
	}
	update_screen();
}

void update_obj() {
	texture[obj.x][obj.y] = COL_PURPLE;
	obj.x += obj.v_x, obj.y += obj.v_y;
	texture[obj.x][obj.y] = COL_WHITE;	
}

void update_player1(int dir) {
	if (dir == 1 && player1.start + length - 1 < w - 1) {
		texture[player1.start][0] = COL_PURPLE;
		texture[player1.start + length][0] = COL_WHITE;
		player1.start += 1;
	}
	else if (dir == -1 && player1.start > 0){
		texture[player1.start + length - 1][0] = COL_PURPLE;
		texture[player1.start - 1][0] = COL_WHITE;
		player1.start -= 1;
	}
}

void update_player2(int dir) {
	if (dir == 1 && player2.start + length - 1 < w - 1) {
		texture[player2.start][h - 1] = COL_PURPLE;
		texture[player2.start + length][h - 1] = COL_WHITE;
		player2.start += 1;
	}
	else if (dir == -1 && player2.start > 0){
		texture[player2.start + length - 1][h - 1] = COL_PURPLE;
		texture[player2.start - 1][h - 1] = COL_WHITE;
		player2.start -= 1;
	}
}
