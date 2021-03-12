#include <game.h>

#define SIDE 16
#define COL_PURPLE 0x2a0a29
#define COL_WHITE  0xeeeeee
static int w, h, block_size;
uint32_t texture[128][128];

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

static void update_screen() {
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
