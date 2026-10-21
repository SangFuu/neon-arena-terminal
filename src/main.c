#include "game.h"
#include "input.h"
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
static void action(Game *g, Controls *c, Action a, int *running) {
    if (a == ACTION_QUIT)
        *running = 0;
    if (a == ACTION_RESTART || (a == ACTION_CONFIRM && g->mode == MENU)) {
        game_start(g, (unsigned)time(NULL));
        controls_clear(c);
    } else if (a == ACTION_PAUSE || (a == ACTION_ESCAPE && g->mode == PLAYING)) {
        if (g->mode == PLAYING)
            g->mode = PAUSED;
        else if (g->mode == PAUSED)
            g->mode = PLAYING;
        controls_clear(c);
    } else if (a == ACTION_CONFIRM && g->mode == PAUSED) {
        g->mode = PLAYING;
        controls_clear(c);
    } else if (a == ACTION_ESCAPE) {
        if (g->mode == MENU)
            *running = 0;
        else
            g->mode = MENU;
        controls_clear(c);
    }
}
static int map_key(WORD vk) {
    switch (vk) {
    case VK_UP:
        return KEY_UP;
    case VK_DOWN:
        return KEY_DOWN;
    case VK_LEFT:
        return KEY_LEFT;
    case VK_RIGHT:
        return KEY_RIGHT;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        return KEY_SHIFT;
    default:
        return vk < 256 ? (int)vk : -1;
    }
}
int main(int argc, char **argv) {
    Game g;
    Controls c;
    char frame[TERM_BUFFER];
    game_start(&g, (unsigned)time(NULL));
    controls_clear(&c);
    g.mode = MENU;
    if (argc == 3 && strcmp(argv[1], "--smoke-test") == 0) {
        int count = atoi(argv[2]), running = 1;
        if (count < 1 || count > 20000)
            return EXIT_FAILURE;
        action(&g, &c, controls_key(&c, KEY_ENTER, 1), &running);
        controls_key(&c, 'D', 1);
        controls_key(&c, KEY_SPACE, 1);
        for (int i = 0; i < count; i++) {
            if (i == 20)
                controls_key(&c, 'F', 1);
            if (i == 60)
                controls_key(&c, 'D', 0);
            game_update(&g, controls_read(&c), 1.0f / 60);
            render_frame(&g, frame);
        }
        action(&g, &c, ACTION_PAUSE, &running);
        if (g.mode != PAUSED)
            return EXIT_FAILURE;
        action(&g, &c, ACTION_CONFIRM, &running);
        if (g.mode != PLAYING)
            return EXIT_FAILURE;
        action(&g, &c, ACTION_QUIT, &running);
        fputs(frame, stdout);
        puts("Terminal : deplacement, tir, esquive, pause, reprise et sortie OK.");
        return running ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE), out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD oldMode, outMode;
    if (!GetConsoleMode(in, &oldMode) || !GetConsoleMode(out, &outMode)) {
        fputs("Ouvrez Jouer.cmd dans un terminal Windows. Dans CLion, utilisez une console externe.\n",
              stderr);
        return EXIT_FAILURE;
    }
    CONSOLE_CURSOR_INFO oldCursor;
    GetConsoleCursorInfo(out, &oldCursor);
    CONSOLE_CURSOR_INFO hidden = {oldCursor.dwSize, FALSE};
    SetConsoleCursorInfo(out, &hidden);
    SetConsoleMode(in, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleTitleA("Neon Arena - Terminal");
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(out, &info);
    COORD size = info.dwSize;
    if (size.X < TERM_COLS + 1)
        size.X = TERM_COLS + 1;
    if (size.Y < TERM_ROWS + 1)
        size.Y = TERM_ROWS + 1;
    SetConsoleScreenBufferSize(out, size);
    SMALL_RECT window = {0, 0, TERM_COLS, TERM_ROWS};
    SetConsoleWindowInfo(out, TRUE, &window);
    DWORD written;
    FillConsoleOutputCharacterA(out, ' ', (DWORD)size.X * size.Y, (COORD){0, 0}, &written);
    LARGE_INTEGER frequency, previous, now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&previous);
    int running = 1;
    while (running) {
        DWORD available;
        while (GetNumberOfConsoleInputEvents(in, &available) && available) {
            INPUT_RECORD event;
            DWORD read;
            if (!ReadConsoleInputA(in, &event, 1, &read) || read != 1) {
                running = 0;
                break;
            }
            if (event.EventType == KEY_EVENT) {
                KEY_EVENT_RECORD k = event.Event.KeyEvent;
                int key = map_key(k.wVirtualKeyCode);
                action(&g, &c, controls_key(&c, key, k.bKeyDown), &running);
                if (key >= 0 && key < KEY_COUNT)
                    c.down[key] = k.bKeyDown != 0;
            } else if (event.EventType == FOCUS_EVENT && !event.Event.FocusEvent.bSetFocus) {
                controls_clear(&c);
                if (g.mode == PLAYING)
                    g.mode = PAUSED;
            }
        }
        QueryPerformanceCounter(&now);
        float dt = (float)((double)(now.QuadPart - previous.QuadPart) / frequency.QuadPart);
        previous = now;
        if (dt > .05f)
            dt = .05f;
        game_update(&g, controls_read(&c), dt);
        render_frame(&g, frame);
        SetConsoleCursorPosition(out, (COORD){0, 0});
        WriteConsoleA(out, frame, (DWORD)strlen(frame), &written, NULL);
        Sleep(33);
    }
    SetConsoleMode(in, oldMode);
    SetConsoleCursorInfo(out, &oldCursor);
    SetConsoleCursorPosition(out, (COORD){0, TERM_ROWS});
    puts("A bientot !");
    return EXIT_SUCCESS;
}
