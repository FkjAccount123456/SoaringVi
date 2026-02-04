#include "screen.h"
#include "wcwidth/wcwidth.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>

void screen_free(screen *scr) {
    free(scr->data);
    free(scr->prev);
}

void screen_resize(screen *scr, size_t h, size_t w) {
    if (h == scr->h && w == scr->w && scr->data && scr->prev)
        return;
    scr->h = h, scr->w = w;
    scr->y = scr->x = 0;
    if (scr->data && scr->prev) {
        free(scr->data);
        free(scr->prev);
    }
    scr->data = calloc(h * w, sizeof(colortext));
    scr->prev = calloc(h * w, sizeof(colortext));
    scr->redraw = true;
}

void screen_change(screen *scr, size_t y, size_t x, colortext c) {
    assert(scr_chck(y, x));
    if (cotext_eq(c, scr_data(y, x))) {
        return;
    }
    if (!scr_data(y, x).ch) {
        size_t i;
        for (i = x; i != -1 && !scr_data(y, x).ch; i--)
            ;
        for (; i < x; i++)
            scr_data(y, i).ch = L' ';
    }
    size_t prevw = wcwidth(scr_data(y, x).ch), curw = wcwidth(c.ch);
    scr_data(y, x) = c;
    c.ch = 0;
    for (size_t i = 1; i < curw; i++) {
        if (scr_data(y, x + i).ch) {
            prevw += wcwidth(scr_data(y, x + i).ch);
            scr_data(y, x + i) = c;
        }
    }
    for (size_t i = curw; i < prevw; i++) {
        scr_data(y, x + i).ch = 0;
    }
}

#ifdef DEBUG_MODE

void screen_log(screen *scr) {
    log("screen_log:\n");
    for (size_t y = 0; y < scr->h; y++) {
        for (size_t x = 0; x < scr->w; x++) {
            if (!scr_data(y, x).ch)
                continue;
            log("%lc", scr_data(y, x).ch);
        }
        log("\n");
    }
    log("end;\n");
}

#endif // DEBUG_MODE

void screen_flush(screen *scr) {
    printf("\x1b[0m\x1b[?25l");
    if (scr->redraw) {
        colortext last_co;
        memset(&last_co, -1, sizeof(last_co));
        for (size_t y = 0; y < scr->h; y++) {
            gotoxy(y, 0);
            for (size_t x = 0; x < scr->w; x++) {
                if (!scr_data(y, x).ch)
                    continue;
                if (!color_eq(last_co, scr_data(y, x))) {
                    cotext_print(scr_data(y, x));
                    last_co = scr_data(y, x);
                } else {
                    printf("%lc", scr_data(y, x).ch);
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
                if (!scr_data(y, x).ch)
                    continue;
                if (cotext_eq(scr_data(y, x), scr_prev(y, x)))
                    continue;
                if (!(last_pos.y == y && last_pos.x + 1 == x))
                    gotoxy(y, x);
                if (!color_eq(last_co, scr_data(y, x))) {
                    cotext_print(scr_data(y, x));
                    last_co = scr_data(y, x);
                } else {
                    printf("%lc", scr_data(y, x).ch);
                }
                last_pos = coord_new(y, x);
            }
            last_pos = coord_new(y, -1);
        }
    }
    if (scr->y != -1 && scr->x != -1) {
        gotoxy(scr->y, scr->x);
        printf("\x1b[0m\x1b[?25h");
    } else {
        printf("\x1b[0m");
    }
    memcpy(scr->prev, scr->data, sizeof(colortext) * scr->h * scr->w);
}
