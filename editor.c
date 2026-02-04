#include "editor.h"

void buffer_init(buffer *buf, screen *scr, textmgr *mgr, size_t y, size_t x, size_t h, size_t w) {
    drawer_init(&buf->dr, scr, mgr, y, x, h, w, DRAWER_MODE_HSCROLL);
    buf->mgr = mgr;
    buf->y = y, buf->x = buf->ideal_x = x;
    buffer_resize(buf, h, w);
}

void buffer_free(buffer *buf) {
    text_free(buf->mgr);
}

void buffer_resize(buffer *buf, size_t h, size_t w) {
    drawer_resize(&buf->dr, h, w);
    buf->h = h, buf->w = w;
}
