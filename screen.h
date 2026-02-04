#ifndef SCREEN_H
#define SCREEN_H

#include "utils.h"
#include <stdbool.h>

typedef struct screen {
    size_t h, w;
    size_t y, x;
    bool redraw;
    // 也许缓存友好一点
    colortext *data;
    colortext *prev;
} screen;

#define scr_data(y, x) scr->data[(y) * scr->w + (x)]
#define scr_prev(y, x) scr->prev[(y) * scr->w + (x)]
#define scr_chck(y, x) ((y) < scr->h && (x) < scr->w)

#define screen_init screen_resize
void screen_free(screen *scr);
#define screen_redraw(scr) (scr)->redraw = true
void screen_resize(screen *scr, size_t h, size_t w);
void screen_change(screen *scr, size_t y, size_t x, colortext c);
#define screen_setcursor(scr, y, x) (scr)->y = (y), (scr)->x = (x)
void screen_flush(screen *scr);

#endif // SCREEN_H
