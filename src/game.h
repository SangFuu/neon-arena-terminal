#ifndef NEON_GAME_H
#define NEON_GAME_H
#define GAME_W 1120
#define GAME_H 720
#define MAX_BOTS 24
#define MAX_SHOTS 192
#define MAX_PARTICLES 256
#define MAX_PICKUPS 64
#define WALL_COUNT 8
#define GRID_COLS 28
#define GRID_ROWS 14
#define CELL 38
#define WORLD_X 28
#define WORLD_Y 110
#define WORLD_W (GRID_COLS * CELL)
#define WORLD_H (GRID_ROWS * CELL)
#define LAST_WAVE 4
typedef struct {
    float x, y;
} Vec;
typedef struct {
    float x, y, w, h;
} Rect;
typedef enum { MENU, PLAYING, PAUSED, WON, LOST } Mode;
typedef struct {
    float mx, my, ax, ay;
    int fire, dash, super;
} Input;
typedef struct {
    Vec pos, face, dashDir;
    float hp, ammo, shotTimer, dashTimer, dashLeft, invulnerable, quiet, super;
} Player;
typedef struct {
    int active, type;
    Vec pos;
    float hp, maxHp, speed, shotTimer, touchTimer;
} Bot;
typedef struct {
    int active, enemy;
    Vec pos, vel;
    float life, damage;
} Shot;
typedef struct {
    int active, kind;
    Vec pos, vel;
    float life, maxLife, size;
} Particle;
typedef struct {
    int active, heal;
    Vec pos;
    float life;
} Pickup;
typedef struct {
    Mode mode;
    Player player;
    Bot bots[MAX_BOTS];
    Shot shots[MAX_SHOTS];
    Particle particles[MAX_PARTICLES];
    Pickup pickups[MAX_PICKUPS];
    Rect walls[WALL_COUNT];
    int nav[GRID_COLS][GRID_ROWS];
    unsigned int rng;
    int wave, spawnLeft, score, kills;
    float spawnTimer, waveDelay, banner, time, hitFlash, superFlash;
} Game;
void game_start(Game *g, unsigned int seed);
void game_update(Game *g, Input input, float dt);
int game_blocked(const Game *g, Vec p, float radius);
int game_bot_count(const Game *g);
float game_distance(Vec a, Vec b);
#endif
