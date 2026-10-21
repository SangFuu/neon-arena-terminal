#include "render.h"
#include <stdio.h>
#include <string.h>
static void text(char grid[TERM_ROWS][TERM_COLS], int row, const char *s) {
    size_t n = strlen(s);
    if (n > TERM_COLS - 2)
        n = TERM_COLS - 2;
    memcpy(&grid[row][1], s, n);
}
static void entity(char grid[TERM_ROWS][TERM_COLS], Vec p, char ch) {
    int x = 2 + (int)((p.x - WORLD_X) * 56 / WORLD_W);
    int y = 5 + (int)((p.y - WORLD_Y) * 20 / WORLD_H);
    if (x >= 2 && x < 58 && y >= 5 && y < 25)
        grid[y][x] = ch;
}
void render_frame(const Game *g, char out[TERM_BUFFER]) {
    char grid[TERM_ROWS][TERM_COLS], line[150];
    memset(grid, ' ', sizeof grid);
    text(grid, 0, "NEON ARENA - JEU TERMINAL");
    snprintf(line, sizeof line, "Vie %3.0f/100 | Munitions %.1f/3 | Super %3.0f%% | Score %d",
             (double)g->player.hp, (double)g->player.ammo, (double)g->player.super, g->score);
    text(grid, 1, line);
    snprintf(line, sizeof line, "Vague %d/%d | Ennemis %d (+%d a venir) | Eliminations %d", g->wave,
             LAST_WAVE, game_bot_count(g), g->spawnLeft, g->kills);
    text(grid, 2, line);
    for (int y = 4; y <= 25; y++)
        for (int x = 1; x <= 58; x++) {
            if (y == 4 || y == 25 || x == 1 || x == 58)
                grid[y][x] = '#';
            else {
                Vec p = {WORLD_X + (x - 1.5f) * WORLD_W / 56, WORLD_Y + (y - 4.5f) * WORLD_H / 20};
                grid[y][x] = game_blocked(g, p, 0) ? '#' : '.';
            }
        }
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (g->pickups[i].active)
            entity(grid, g->pickups[i].pos, g->pickups[i].heal ? '+' : '$');
    for (int i = 0; i < MAX_SHOTS; i++)
        if (g->shots[i].active)
            entity(grid, g->shots[i].pos, g->shots[i].enemy ? 'o' : '*');
    for (int i = 0; i < MAX_BOTS; i++)
        if (g->bots[i].active)
            entity(grid, g->bots[i].pos, g->bots[i].type == 2 ? 'B' : g->bots[i].type == 1 ? 'T' : 'M');
    entity(grid, g->player.pos, '@');
    text(grid, 26, "@ vous  # mur  M contact  T tireur  B boss  $ bonus  + soin");
    text(grid, 28, "ZQSD/WASD : bouger | Fleches : viser/tirer | Espace : auto-tir");
    text(grid, 29, "F/Maj : esquive | E : super | P : pause | R : rejouer | X : quitter");
    switch (g->mode) {
    case MENU:
        text(grid, 31, "ENTREE : commencer les 4 vagues. Tout se joue au clavier.");
        break;
    case PAUSED:
        text(grid, 31, "PAUSE - P/Entree : reprendre | Echap : menu");
        break;
    case WON:
        text(grid, 31, "VICTOIRE ! R : nouvelle partie | Echap : menu");
        break;
    case LOST:
        text(grid, 31, "DEFAITE - R : retenter | Echap : menu");
        break;
    default:
        text(grid, 31, "Survivez aux quatre vagues. Vie et munitions regenerent.");
        break;
    }
    size_t n = 0;
    for (int y = 0; y < TERM_ROWS; y++) {
        memcpy(out + n, grid[y], TERM_COLS);
        n += TERM_COLS;
        out[n++] = '\n';
    }
    out[n] = '\0';
}
