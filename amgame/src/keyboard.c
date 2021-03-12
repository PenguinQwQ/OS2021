#include <game.h>

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,

/*
static const char *key_names[] = {
  AM_KEYS(KEYNAME)
};
*/

int read_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (!event.keydown) return AM_KEY_NONE;
  return event.keycode;
}

