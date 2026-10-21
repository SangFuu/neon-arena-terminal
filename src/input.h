#ifndef NEON_INPUT_H
#define NEON_INPUT_H
#include "game.h"
enum {
    KEY_ENTER = 13,
    KEY_ESCAPE = 27,
    KEY_SPACE = 32,
    KEY_SHIFT = 256,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_COUNT
};
typedef struct {
    unsigned char down[KEY_COUNT];
    int dash, super;
} Controls;
typedef enum { ACTION_NONE, ACTION_CONFIRM, ACTION_PAUSE, ACTION_ESCAPE, ACTION_RESTART, ACTION_QUIT } Action;
void controls_clear(Controls *c);
Action controls_key(Controls *c, int key, int down);
Input controls_read(Controls *c);
#endif
