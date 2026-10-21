#ifndef NEON_RENDER_H
#define NEON_RENDER_H
#include "game.h"
#define TERM_COLS 74
#define TERM_ROWS 35
#define TERM_BUFFER (TERM_ROWS * (TERM_COLS + 1) + 1)
void render_frame(const Game *g, char out[TERM_BUFFER]);
#endif
