#include "screen.h"
#include "wcwidth/wcwidth.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

void screen_free(screen *scr) {
    for (size_t i = 0; i < scr->h; i++)
        free(scr->data[i]), free(scr->prev[i]);
    free(scr->data);
    free(scr->prev);
}

void screen_init(screen *scr, size_t h, size_t w) {
    scr->h = h, scr->w = w;
    scr->y = scr->x = 0;
    scr->data = malloc(h * sizeof(colortext *));
    scr->prev = malloc(h * sizeof(colortext *));
    for (size_t i = 0; i < h; i++) {
        scr->data[i] = calloc(w, sizeof(colortext));
        scr->prev[i] = calloc(w, sizeof(colortext));
    }
    scr->redraw = true;
}

void screen_resize(screen *scr, size_t h, size_t w) {
    if (h == scr->h && w == scr->w && scr->data && scr->prev)
        return;
    if (scr->data && scr->prev) {
        screen_free(scr);
    }
    screen_init(scr, h, w);
}

void screen_change(screen *scr, size_t y, size_t x, colortext c) {
#ifdef DEBUG
    assert(scr_chck(y, x));
#endif
    if (cotext_eq(c, scr->data[y][x])) {
        return;
    }
    if (!scr->data[y][x].ch) {
        size_t i;
        for (i = x; i != -1 && !scr->data[y][x].ch; i--)
            ;
        for (; i < x; i++)
            scr->data[y][i].ch = L' ';
    }
    size_t prevw = wcwidth(scr->data[y][x].ch), curw = wcwidth(c.ch);
    scr->data[y][x] = c;
    if (curw > 1) {
        c.ch = 0;
        for (size_t i = 1; i < curw; i++) {
            if (scr->data[y][x + i].ch) {
                prevw += wcwidth(scr->data[y][x + i].ch);
                scr->data[y][x + i] = c;
            }
        }
    }
    for (size_t i = curw; i < prevw; i++) {
        scr->data[y][x + i].ch = 0;
    }
}

void screen_flush(screen *scr) {
    u_procsnprintf(snprintf, "\x1b[0m\x1b[?25l");
    if (scr->redraw) {
        colortext last_co;
        memset(&last_co, -1, sizeof(last_co));
        for (size_t y = 0; y < scr->h; y++) {
            gotoxy(y, 0);
            for (size_t x = 0; x < scr->w; x++) {
                if (!scr->data[y][x].ch)
                    continue;
                if (!color_eq(last_co, scr->data[y][x])) {
                    cotext_print(scr->data[y][x]);
                    last_co = scr->data[y][x];
                } else {
                    putchar_c32(scr->data[y][x].ch);
                }
            }
        }
        scr->redraw = false;
    } else {
        gotoxy(0, 0);
        coord last_pos = coord_new(0, -1);
        colortext last_co;
        memset(&last_co, -1, sizeof(last_co));
        for (size_t y = 0; y < scr->h; y++) {
            for (size_t x = 0; x < scr->w; x++) {
                if (!scr->data[y][x].ch)
                    continue;
                if (cotext_eq(scr->data[y][x], scr->prev[y][x]))
                    continue;
                if (!(last_pos.y == y && last_pos.x + 1 == x))
                    gotoxy(y, x);
                if (!color_eq(last_co, scr->data[y][x])) {
                    cotext_print(scr->data[y][x]);
                    last_co = scr->data[y][x];
                } else {
                    putchar_c32(scr->data[y][x].ch);
                }
                last_pos = coord_new(y, x);
            }
            last_pos = coord_new(y, -1);
        }
    }
    if (scr->y != -1 && scr->x != -1) {
        gotoxy(scr->y, scr->x);
        u_procsnprintf(snprintf, "\x1b[0m\x1b[?25h");
    } else {
        u_procsnprintf(snprintf, "\x1b[0m");
    }
    for (size_t i = 0; i < scr->h; i++)
        memcpy(scr->prev[i], scr->data[i], scr->w * sizeof(colortext));
}
