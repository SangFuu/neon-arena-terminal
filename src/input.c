#include "input.h"
#include <string.h>
void controls_clear(Controls *c) {
    memset(c, 0, sizeof *c);
}
Action controls_key(Controls *c, int key, int down) {
    if (key >= 'a' && key <= 'z')
        key -= 'a' - 'A';
    if (key < 0 || key >= KEY_COUNT)
        return ACTION_NONE;
    int repeat = c->down[key];
    c->down[key] = down != 0;
    if (!down || repeat)
        return ACTION_NONE;
    switch (key) {
    case KEY_ENTER:
    case KEY_SPACE:
        return ACTION_CONFIRM;
    case KEY_ESCAPE:
        return ACTION_ESCAPE;
    case 'P':
        return ACTION_PAUSE;
    case 'R':
        return ACTION_RESTART;
    case 'X':
        return ACTION_QUIT;
    case KEY_SHIFT:
    case 'F':
        c->dash = 1;
        break;
    case 'E':
        c->super = 1;
        break;
    }
    return ACTION_NONE;
}
Input controls_read(Controls *c) {
    Input i = {0};
    i.mx = c->down['D'] - (c->down['A'] || c->down['Q']);
    i.my = c->down['S'] - (c->down['W'] || c->down['Z']);
    i.ax = c->down[KEY_RIGHT] - c->down[KEY_LEFT];
    i.ay = c->down[KEY_DOWN] - c->down[KEY_UP];
    i.fire = c->down[KEY_SPACE];
    i.dash = c->dash;
    i.super = c->super;
    c->dash = c->super = 0;
    return i;
}
