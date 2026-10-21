#include "input.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                             \
    do {                                                                                                     \
        if (!(x)) {                                                                                          \
            fprintf(stderr, "Echec ligne %d : %s\n", __LINE__, #x);                                          \
            return EXIT_FAILURE;                                                                             \
        }                                                                                                    \
    } while (0)
int main(void) {
    Controls c;
    controls_clear(&c);
    controls_key(&c, 'z', 1);
    controls_key(&c, 'q', 1);
    Input i = controls_read(&c);
    CHECK(i.mx == -1 && i.my == -1);
    controls_clear(&c);
    controls_key(&c, 'W', 1);
    controls_key(&c, 'A', 1);
    i = controls_read(&c);
    CHECK(i.mx == -1 && i.my == -1);
    controls_clear(&c);
    controls_key(&c, KEY_RIGHT, 1);
    controls_key(&c, KEY_SPACE, 1);
    i = controls_read(&c);
    CHECK(i.ax == 1 && i.fire);
    controls_key(&c, KEY_SPACE, 0);
    CHECK(!controls_read(&c).fire);
    controls_key(&c, 'F', 1);
    CHECK(controls_read(&c).dash);
    controls_key(&c, 'F', 1);
    CHECK(!controls_read(&c).dash);
    controls_key(&c, 'F', 0);
    controls_key(&c, KEY_SHIFT, 1);
    CHECK(controls_read(&c).dash);
    controls_key(&c, 'E', 1);
    CHECK(controls_read(&c).super);
    CHECK(!controls_read(&c).super);
    CHECK(controls_key(&c, 'P', 1) == ACTION_PAUSE);
    CHECK(controls_key(&c, 'P', 1) == ACTION_NONE);
    CHECK(controls_key(&c, KEY_ESCAPE, 1) == ACTION_ESCAPE);
    CHECK(controls_key(&c, 'R', 1) == ACTION_RESTART);
    CHECK(controls_key(&c, 'X', 1) == ACTION_QUIT);
    controls_clear(&c);
    CHECK(controls_read(&c).ax == 0);
    CHECK(controls_key(&c, -1, 1) == ACTION_NONE);
    Game g;
    char frame[TERM_BUFFER];
    game_start(&g, 1);
    g.mode = MENU;
    render_frame(&g, frame);
    CHECK(strlen(frame) == TERM_BUFFER - 1);
    CHECK(strstr(frame, "ENTREE") && strchr(frame, '@') && strchr(frame, '#'));
    g.mode = WON;
    render_frame(&g, frame);
    CHECK(strstr(frame, "VICTOIRE"));
    g.mode = LOST;
    render_frame(&g, frame);
    CHECK(strstr(frame, "DEFAITE"));
    g.mode = PAUSED;
    render_frame(&g, frame);
    CHECK(strstr(frame, "PAUSE"));
    puts("Clavier et affichage terminal OK.");
    return EXIT_SUCCESS;
}
