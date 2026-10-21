#include "game.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846f


static float clampf(float n, float a, float b) {
    return n < a ? a : (n > b ? b : n);
}

static Vec add(Vec a, Vec b) {
    return (Vec){a.x + b.x, a.y + b.y};
}

static Vec scale(Vec v, float n) {
    return (Vec){v.x * n, v.y * n};
}

static Vec direction(Vec a, Vec b) {
    float x = b.x - a.x, y = b.y - a.y, d = sqrtf(x * x + y * y);
    return d > .0001f ? (Vec){x / d, y / d} : (Vec){0, 0};
}

float game_distance(Vec a, Vec b) {
    float x = a.x - b.x, y = a.y - b.y;
    return sqrtf(x * x + y * y);
}

static unsigned int random_u(Game *g) {
    unsigned int x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g->rng = x;
}

static float random_f(Game *g) {
    return (float)(random_u(g) % 10000) / 10000.0f;
}

static void reduce(float *v, float dt) {
    *v = fmaxf(0, *v - dt);
}

int game_blocked(const Game *g, Vec p, float r) {
    if (p.x - r < WORLD_X || p.x + r > WORLD_X + WORLD_W || p.y - r < WORLD_Y || p.y + r > WORLD_Y + WORLD_H)
        return 1;
    for (int i = 0; i < WALL_COUNT; i++) {
        Rect w = g->walls[i];
        float dx = p.x - clampf(p.x, w.x, w.x + w.w), dy = p.y - clampf(p.y, w.y, w.y + w.h);
        if (dx * dx + dy * dy < r * r)
            return 1;
    }
    return 0;
}

static void move(const Game *g, Vec *p, Vec delta, float r) {
    int steps = (int)(fmaxf(fabsf(delta.x), fabsf(delta.y)) / 6) + 1;
    delta = scale(delta, 1.0f / (float)steps);
    for (int i = 0; i < steps; i++) {
        Vec n = {p->x + delta.x, p->y};
        if (!game_blocked(g, n, r))
            *p = n;
        n = (Vec){p->x, p->y + delta.y};
        if (!game_blocked(g, n, r))
            *p = n;
    }
}

static int sight_radius(const Game *g, Vec a, Vec b, float radius) {
    float d = game_distance(a, b);
    int steps = (int)(d / 7) + 1;
    for (int i = 1; i < steps; i++) {
        float t = (float)i / (float)steps;
        Vec p = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
        if (game_blocked(g, p, radius))
            return 0;
    }
    return 1;
}

static int sight(const Game *g, Vec a, Vec b) {
    return sight_radius(g, a, b, 6);
}

int game_bot_count(const Game *g) {
    int n = 0;
    for (int i = 0; i < MAX_BOTS; i++)
        n += g->bots[i].active != 0;
    return n;
}

static void burst(Game *g, Vec p, int kind, int n) {
    for (int i = 0; i < MAX_PARTICLES && n > 0; i++)
        if (!g->particles[i].active) {
            float a = random_f(g) * 2 * PI, s = 40 + random_f(g) * 100, life = .25f + random_f(g) * .35f;
            g->particles[i] =
                (Particle){1, kind, p, {cosf(a) * s, sinf(a) * s}, life, life, 2 + random_f(g) * 4};
            n--;
        }
}

static void projectile(Game *g, Vec p, Vec aim, int enemy, float damage) {
    for (int i = 0; i < MAX_SHOTS; i++)
        if (!g->shots[i].active) {
            g->shots[i] = (Shot){1, enemy, p, scale(aim, enemy ? 255 : 570), enemy ? 2.8f : 1.25f, damage};
            return;
        }
}

void game_start(Game *g, unsigned int seed) {
    memset(g, 0, sizeof *g);
    g->rng = seed ? seed : 1;
    g->mode = PLAYING;
    g->player = (Player){.pos = {560, 376}, .face = {0, -1}, .hp = 100, .ammo = 3};
    const int cells[WALL_COUNT][4] = {{4, 3, 4, 1},  {20, 3, 4, 1},  {12, 2, 4, 1}, {12, 10, 4, 1},
                                      {4, 10, 4, 1}, {20, 10, 4, 1}, {8, 6, 1, 2},  {19, 6, 1, 2}};
    for (int i = 0; i < WALL_COUNT; i++)
        g->walls[i] = (Rect){WORLD_X + cells[i][0] * CELL, WORLD_Y + cells[i][1] * CELL, cells[i][2] * CELL,
                             cells[i][3] * CELL};
    g->wave = 1;
    g->spawnLeft = 6;
    g->spawnTimer = 1.4f;
    g->banner = 2;
    g->waveDelay = 1.8f;
}

/* Champ de distances partage : les ennemis contournent les obstacles. */
static void navigation(Game *g) {
    int bestX = 0, bestY = 0;
    float best = 1e20f;
    for (int x = 0; x < GRID_COLS; x++)
        for (int y = 0; y < GRID_ROWS; y++) {
            Vec p = {WORLD_X + (x + .5f) * CELL, WORLD_Y + (y + .5f) * CELL};
            g->nav[x][y] = game_blocked(g, p, 15) ? -2 : -1;
            float d = game_distance(p, g->player.pos);
            if (g->nav[x][y] == -1 && d < best) {
                best = d;
                bestX = x;
                bestY = y;
            }
        }

    int qx[GRID_COLS * GRID_ROWS], qy[GRID_COLS * GRID_ROWS], head = 0, tail = 0;
    g->nav[bestX][bestY] = 0;
    qx[tail] = bestX;
    qy[tail++] = bestY;
    const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        for (int i = 0; i < 4; i++) {
            int nx = x + dx[i], ny = y + dy[i];
            if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS && g->nav[nx][ny] == -1) {
                g->nav[nx][ny] = g->nav[x][y] + 1;
                qx[tail] = nx;
                qy[tail++] = ny;
            }
        }
    }
}

static Vec chase(Game *g, Vec p) {
    if (sight_radius(g, p, g->player.pos, 18))
        return direction(p, g->player.pos);
    int x = (int)clampf((p.x - WORLD_X) / CELL, 0, GRID_COLS - 1),
        y = (int)clampf((p.y - WORLD_Y) / CELL, 0, GRID_ROWS - 1), bx = x, by = y, best = g->nav[x][y];
    const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i], ny = y + dy[i];
        if (nx >= 0 && nx < GRID_COLS && ny >= 0 && ny < GRID_ROWS && g->nav[nx][ny] >= 0 &&
            (best < 0 || g->nav[nx][ny] < best)) {
            best = g->nav[nx][ny];
            bx = nx;
            by = ny;
        }
    }
    return direction(p, (Vec){WORLD_X + (bx + .5f) * CELL, WORLD_Y + (by + .5f) * CELL});
}

static void spawn(Game *g) {
    for (int i = 0; i < MAX_BOTS; i++)
        if (!g->bots[i].active) {
            Vec p = {0, 0};
            int found = 0;
            for (int k = 0; k < 40; k++) {
                int side = (int)(random_u(g) % 4);
                float t = .08f + random_f(g) * .84f;
                p = (Vec){WORLD_X + 20 + t * (WORLD_W - 40), WORLD_Y + 20};
                if (side == 1)
                    p.y = WORLD_Y + WORLD_H - 20;
                if (side >= 2)
                    p = (Vec){side == 2 ? WORLD_X + 20 : WORLD_X + WORLD_W - 20,
                              WORLD_Y + 20 + t * (WORLD_H - 40)};
                found = !game_blocked(g, p, 18) && game_distance(p, g->player.pos) > 210;
                if (found)
                    break;
            }
            if (!found) {
                return;
            }
            int type = (g->wave == LAST_WAVE && g->spawnLeft == 1) ? 2 : (int)(random_u(g) % 2);
            float hp = (type == 2 ? 150 : (type == 1 ? 64 : 48)) + g->wave * 5;
            g->bots[i] = (Bot){
                1, type, p, hp, hp, type == 2 ? 80 : (type == 1 ? 85 : 112 + g->wave * 5), .8f + random_f(g),
                0};
            g->spawnLeft--;
            return;
        }
}

static void hitPlayer(Game *g, float damage) {
    if (g->player.invulnerable > 0)
        return;
    g->player.hp = fmaxf(0, g->player.hp - damage);
    g->player.invulnerable = .45f;
    g->player.quiet = 0;
    g->hitFlash = .2f;
    burst(g, g->player.pos, 2, 12);
    if (g->player.hp <= 0)
        g->mode = LOST;
}

static void killBot(Game *g, Bot *b) {
    b->active = 0;
    g->kills++;
    g->score += b->type == 2 ? 250 : 100;
    g->player.super = fminf(100, g->player.super + 22);
    burst(g, b->pos, b->type == 1 ? 1 : 3, 16);
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (!g->pickups[i].active) {
            g->pickups[i] = (Pickup){1, g->kills % 4 == 0, b->pos, 15};
            break;
        }
}

/* Distance au segment : un tir reste fiable meme si une image prend plus de temps. */
static float segment_distance(Vec p, Vec a, Vec b) {
    float dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
    float t = length > .0001f ? clampf(((p.x - a.x) * dx + (p.y - a.y) * dy) / length, 0, 1) : 0;
    return game_distance(p, (Vec){a.x + t * dx, a.y + t * dy});
}

static Vec aimAtBot(const Game *g) {
    float best = 1e20f;
    Vec aim = g->player.face;
    for (int i = 0; i < MAX_BOTS; i++)
        if (g->bots[i].active && sight(g, g->player.pos, g->bots[i].pos)) {
            float d = game_distance(g->player.pos, g->bots[i].pos);
            if (d < best) {
                best = d;
                aim = direction(g->player.pos, g->bots[i].pos);
            }
        }
    return aim;
}

void game_update(Game *g, Input in, float dt) {
    if (g->mode != PLAYING || !isfinite(dt) || dt <= 0)
        return;
    dt = fminf(dt, .05f);
    g->time += dt;
    reduce(&g->banner, dt);
    reduce(&g->hitFlash, dt);
    reduce(&g->superFlash, dt);
    Player *p = &g->player;
    reduce(&p->shotTimer, dt);
    reduce(&p->dashTimer, dt);
    reduce(&p->dashLeft, dt);
    reduce(&p->invulnerable, dt);
    p->ammo = fminf(3, p->ammo + dt * 1.35f);
    p->quiet += dt;
    if (p->quiet > 4)
        p->hp = fminf(100, p->hp + dt * 4);
    Vec movement = direction((Vec){0, 0}, (Vec){in.mx, in.my});
    if (in.dash && p->dashTimer <= 0) {
        p->dashDir = (fabsf(movement.x) + fabsf(movement.y) > .01f) ? movement : p->face;
        p->dashLeft = .16f;
        p->dashTimer = 2.5f;
        p->invulnerable = fmaxf(p->invulnerable, .2f);
        burst(g, p->pos, 0, 10);
    }
    if (p->dashLeft > 0)
        move(g, &p->pos, scale(p->dashDir, dt * 740), 16);
    else
        move(g, &p->pos, scale(movement, dt * 230), 16);
    int aimed = fabsf(in.ax) + fabsf(in.ay) > .01f;
    if (aimed)
        p->face = direction((Vec){0, 0}, (Vec){in.ax, in.ay});
    else if (in.fire)
        p->face = aimAtBot(g);
    else if (fabsf(movement.x) + fabsf(movement.y) > .01f)
        p->face = movement;
    if ((in.fire || aimed) && p->shotTimer <= 0 && p->ammo >= 1) {
        projectile(g, add(p->pos, scale(p->face, 8)), p->face, 0, 28);
        p->ammo -= 1;
        p->shotTimer = .18f;
    }
    if (in.super && p->super >= 100) {
        p->super = 0;
        p->invulnerable = .65f;
        g->superFlash = .5f;
        burst(g, p->pos, 0, 35);
        for (int i = 0; i < 16; i++) {
            float a = i * 2 * PI / 16;
            Vec v = {cosf(a), sinf(a)};
            projectile(g, add(p->pos, scale(v, 22)), v, 0, 65);
        }
    }
    navigation(g);
    g->spawnTimer -= dt;
    if (g->spawnLeft > 0 && g->spawnTimer <= 0) {
        spawn(g);
        g->spawnTimer = .85f;
    }
    for (int i = 0; i < MAX_BOTS; i++)
        if (g->bots[i].active) {
            Bot *b = &g->bots[i];
            reduce(&b->shotTimer, dt);
            reduce(&b->touchTimer, dt);
            float d = game_distance(b->pos, p->pos);
            Vec v = chase(g, b->pos);
            if (b->type == 1 && d < 285 && sight(g, b->pos, p->pos)) {
                v = d < 135 ? scale(direction(b->pos, p->pos), -1) : (Vec){0, 0};
                if (b->shotTimer <= 0) {
                    Vec a = direction(b->pos, p->pos);
                    projectile(g, add(b->pos, scale(a, 22)), a, 1, 13);
                    b->shotTimer = 1.45f;
                }
            }
            for (int j = 0; j < MAX_BOTS; j++)
                if (j != i && g->bots[j].active && game_distance(b->pos, g->bots[j].pos) < 29)
                    v = add(v, scale(direction(g->bots[j].pos, b->pos), .75f));
            if (fabsf(v.x) + fabsf(v.y) > 0) {
                v = direction((Vec){0, 0}, v);
            }
            move(g, &b->pos, scale(v, dt * b->speed), b->type == 2 ? 18 : 14);
            if (game_distance(b->pos, p->pos) < (b->type == 2 ? 36 : 31) && b->touchTimer <= 0) {
                hitPlayer(g, b->type == 2 ? 24 : 15);
                b->touchTimer = .8f;
            }
        }
    for (int i = 0; i < MAX_SHOTS; i++)
        if (g->shots[i].active) {
            Shot *s = &g->shots[i];
            Vec previous = s->pos;
            s->life -= dt;
            s->pos = add(s->pos, scale(s->vel, dt));
            if (s->life <= 0 || game_blocked(g, s->pos, 4)) {
                s->active = 0;
                continue;
            }
            if (s->enemy) {
                if (game_distance(s->pos, p->pos) < 20) {
                    hitPlayer(g, s->damage);
                    s->active = 0;
                    burst(g, s->pos, 2, 5);
                }
            } else
                for (int j = 0; j < MAX_BOTS; j++)
                    if (g->bots[j].active && segment_distance(g->bots[j].pos, previous, s->pos) <
                                                 (g->bots[j].type == 2 ? 23 : 19)) {
                        g->bots[j].hp -= s->damage;
                        s->active = 0;
                        burst(g, s->pos, 1, 5);
                        if (g->bots[j].hp <= 0)
                            killBot(g, &g->bots[j]);
                        break;
                    }
        }
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (g->pickups[i].active) {
            Pickup *item = &g->pickups[i];
            item->life -= dt;
            float d = game_distance(item->pos, p->pos);
            if (d < 95)
                item->pos = add(item->pos, scale(direction(item->pos, p->pos), dt * 210));
            if (d < 25 && g->mode == PLAYING) {
                if (item->heal)
                    p->hp = fminf(100, p->hp + 22);
                else {
                    g->score += 25;
                    p->super = fminf(100, p->super + 5);
                }
                item->active = 0;
                burst(g, p->pos, 0, 8);
            }
            if (item->life <= 0)
                item->active = 0;
        }
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (g->particles[i].active) {
            Particle *a = &g->particles[i];
            a->life -= dt;
            a->pos = add(a->pos, scale(a->vel, dt));
            a->vel = scale(a->vel, fmaxf(0, 1 - dt * 3));
            if (a->life <= 0)
                a->active = 0;
        }
    if (p->hp <= 0) {
        p->hp = 0;
        g->mode = LOST;
    }
    if (g->mode == PLAYING && g->spawnLeft == 0 && game_bot_count(g) == 0) {
        g->waveDelay -= dt;
        if (g->waveDelay <= 0) {
            if (g->wave >= LAST_WAVE) {
                g->mode = WON;
                g->score += 500;
            } else {
                g->wave++;
                g->spawnLeft = 4 + g->wave * 2;
                g->spawnTimer = 1.2f;
                g->waveDelay = 1.8f;
                g->banner = 2;
                p->hp = fminf(100, p->hp + 25);
                g->score += 150;
            }
        }
    }
}
