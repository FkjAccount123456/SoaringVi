#ifndef EDITOR_H
#define EDITOR_H

#include "drawer.h"
#include <stddef.h>

typedef struct buffer {
    drawer dr;
    textmgr *mgr;
    size_t y, x, ideal_x;
    size_t h, w;
} buffer;

void buffer_init(buffer *buf, screen *scr, textmgr *mgr, size_t y, size_t x, size_t h, size_t w);
void buffer_free(buffer *buf);

void buffer_resize(buffer *buf, size_t h, size_t w);

#endif // EDITOR_H
