#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int checks = 0;
#define CHECK(c)                                                                                             \
    do {                                                                                                     \
        checks++;                                                                                            \
        if (!(c)) {                                                                                          \
            fprintf(stderr, "ECHEC ligne %d : %s\n", __LINE__, #c);                                          \
            exit(EXIT_FAILURE);                                                                              \
        }                                                                                                    \
    } while (0)
static Game fresh(void) {
    Game g;
    game_start(&g, 42);
    g.spawnLeft = 0;
    g.waveDelay = 999;
    return g;
}
static int shots(const Game *g) {
    int n = 0;
    for (int i = 0; i < MAX_SHOTS; i++)
        n += g->shots[i].active != 0;
    return n;
}
int main(void) {
    Game a = fresh(), b = fresh();
    Vec start = a.player.pos;
    for (int i = 0; i < 10; i++) {
        game_update(&a, (Input){.mx = 1}, 1.0f / 60);
        game_update(&b, (Input){.mx = 1, .my = 1}, 1.0f / 60);
    }
    CHECK(fabsf(game_distance(a.player.pos, start) - game_distance(b.player.pos, start)) < .01f);
    CHECK(!game_blocked(&a, a.player.pos, 16));
    a = fresh();
    Rect w = a.walls[0];
    a.player.pos = (Vec){w.x - 17, w.y + w.h / 2};
    for (int i = 0; i < 120; i++)
        game_update(&a, (Input){.mx = 1}, 1.0f / 60);
    CHECK(a.player.pos.x <= w.x - 16 + .01f);
    CHECK(!game_blocked(&a, a.player.pos, 16));
    a = fresh();
    a.player.pos.x = WORLD_X + 16;
    for (int i = 0; i < 30; i++)
        game_update(&a, (Input){.mx = -1}, 1.0f / 60);
    CHECK(a.player.pos.x >= WORLD_X + 16);
    a = fresh();
    game_update(&a, (Input){.mx = 1, .dash = 1}, 1.0f / 60);
    CHECK(a.player.dashTimer > 2.4f);
    CHECK(a.player.invulnerable > 0);
    CHECK(game_distance(a.player.pos, start) > 10);
    float timer = a.player.dashTimer;
    game_update(&a, (Input){.dash = 1}, 1.0f / 60);
    CHECK(a.player.dashTimer < timer);
    a = fresh();
    game_update(&a, (Input){.fire = 1}, 1.0f / 60);
    CHECK(shots(&a) == 1);
    CHECK(a.player.ammo < 3);
    a = fresh();
    a.player.super = 100;
    game_update(&a, (Input){.super = 1}, 1.0f / 60);
    CHECK(shots(&a) == 16);
    CHECK(a.player.super == 0);
    CHECK(a.player.invulnerable > 0);
    a = fresh();
    a.bots[0] = (Bot){.active = 1,
                      .type = 0,
                      .pos = {660, 376},
                      .hp = 28,
                      .maxHp = 28,
                      .speed = 0,
                      .shotTimer = 99,
                      .touchTimer = 99};
    for (int i = 0; i < 30; i++)
        game_update(&a, (Input){.fire = 1}, 1.0f / 60);
    CHECK(!a.bots[0].active);
    CHECK(a.kills == 1);
    CHECK(a.score >= 100);
    CHECK(a.player.super >= 22);
    a = fresh();
    a.shots[0] = (Shot){.active = 1, .enemy = 1, .pos = a.player.pos, .life = 1, .damage = 20};
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.player.hp == 80);
    a.shots[0] = (Shot){.active = 1, .enemy = 1, .pos = a.player.pos, .life = 1, .damage = 20};
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.player.hp == 80);
    a = fresh();
    a.bots[0] = (Bot){.active = 1, .type = 0, .pos = {562, 376}, .hp = 28, .maxHp = 28, .touchTimer = 99};
    game_update(&a, (Input){.fire = 1}, .05f);
    CHECK(a.kills == 1);
    a = fresh();
    a.player.hp = 5;
    a.shots[0] = (Shot){.active = 1, .enemy = 1, .pos = a.player.pos, .life = 1, .damage = 20};
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.mode == LOST);
    CHECK(a.player.hp == 0);
    a = fresh();
    a.mode = PAUSED;
    b = a;
    game_update(&a, (Input){.mx = 1, .fire = 1, .dash = 1, .super = 1}, .05f);
    CHECK(memcmp(&a, &b, sizeof a) == 0);
    a = fresh();
    a.waveDelay = 0;
    a.player.hp = 50;
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.wave == 2 && a.spawnLeft == 8);
    CHECK(a.player.hp == 75);
    a = fresh();
    a.wave = LAST_WAVE;
    a.waveDelay = 0;
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.mode == WON && a.score == 500);
    b = a;
    game_update(&a, (Input){.fire = 1, .mx = 1}, .05f);
    CHECK(memcmp(&a, &b, sizeof a) == 0);
    a = fresh();
    a.player.hp = 40;
    a.pickups[0] = (Pickup){1, 1, a.player.pos, 5};
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.player.hp == 62);
    CHECK(!a.pickups[0].active);
    a = fresh();
    a.player.hp = 40;
    a.player.quiet = 5;
    game_update(&a, (Input){0}, 1.0f / 60);
    CHECK(a.player.hp > 40);
    a = fresh();
    b = a;
    game_update(&a, (Input){0}, NAN);
    CHECK(memcmp(&a, &b, sizeof a) == 0);
    for (unsigned int seed = 1; seed <= 16; seed++) {
        game_start(&a, seed);
        for (int frame = 0; frame < 1200; frame++) {
            if (a.mode != PLAYING)
                break;
            Input input = {.mx = sinf(frame * .023f),
                           .my = cosf(frame * .017f),
                           .fire = 1,
                           .dash = frame % 180 == 0,
                           .super = frame % 20 == 0};
            game_update(&a, input, 1.0f / 60);
            CHECK(isfinite(a.player.pos.x) && isfinite(a.player.pos.y));
            CHECK(!game_blocked(&a, a.player.pos, 15.99f));
            CHECK(a.player.ammo >= 0 && a.player.ammo <= 3);
            CHECK(a.player.hp >= 0 && a.player.hp <= 100);
            for (int i = 0; i < MAX_BOTS; i++)
                if (a.bots[i].active) {
                    CHECK(isfinite(a.bots[i].pos.x));
                    CHECK(!game_blocked(&a, a.bots[i].pos, a.bots[i].type == 2 ? 17.99f : 13.99f));
                }
        }
    }
    /* Parcours complet des quatre vagues avec protection de test, absente du jeu. */
    game_start(&a, 7);
    int frame;
    for (frame = 0; frame < 20000 && a.mode == PLAYING; frame++) {
        a.player.hp = 100;
        a.player.invulnerable = 1;
        game_update(&a, (Input){.fire = 1, .super = 1}, 1.0f / 60);
    }
    if (a.mode != WON)
        fprintf(stderr, "Progression : vague %d, restants %d, bots %d, kills %d\n", a.wave, a.spawnLeft,
                game_bot_count(&a), a.kills);
    if (a.mode != WON) {
        for (int i = 0; i < MAX_BOTS; i++)
            if (a.bots[i].active)
                fprintf(stderr, "Bot %d type %d pos %.2f %.2f hp %.1f\n", i, a.bots[i].type,
                        (double)a.bots[i].pos.x, (double)a.bots[i].pos.y, (double)a.bots[i].hp);
    }
    CHECK(a.mode == WON);
    CHECK(a.kills == 36);
    printf("Gameplay OK : %d controles, 16 simulations et une partie complete en %d images.\n", checks,
           frame);
    return EXIT_SUCCESS;
}
