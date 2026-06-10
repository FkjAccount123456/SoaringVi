#ifndef EDITOR_H
#define EDITOR_H

#include "drawer.h"
#include <stddef.h>

#define BUFFER_MIN_H 2
#define BUFFER_MIN_W 5
#define BUFFER_MIN_SIZE coord_new(BUFFER_MIN_H, BUFFER_MIN_W)

#define bscreen_change(y, x, c)                                                \
    screen_change(buf->dr.scr, (y) + buf->t, (x) + buf->l, c)

enum {
    M_NORMAL,
    M_INSERT,
    M_VISUAL,

    M_USRADD,
};

extern size_t e_mode_useradd_cur;

size_t e_mode_add();

typedef struct editor editor;
typedef struct split split;
typedef struct buffer buffer;
typedef struct window_vtable window_vtable;

#define WINDOW_COMMON                                                          \
    int t, l, h, w;                                                            \
    editor *e;                                                                 \
    bool is_buf;                                                               \
    split *parent;                                                             \
    window_vtable *vtable

typedef struct window {
    WINDOW_COMMON;
} window;

#define wscreen_change(W, Y, X, ch)                                            \
    screen_change(&(W)->e->scr, (Y) + (W)->t, (X) + (W)->l, ch)

// 行吧，这就是多态了
typedef struct window_vtable {
    void (*draw)(window *);
    void (*free)(window *);
    void (*move)(window *, int, int);
    void (*resize)(window *, int, int);
    void (*moveresize)(window *, int, int, int, int);
} window_vtable;

#define window_draw(w) (w)->vtable->draw((window *)(w))
#define window_free(w) (w)->vtable->free((window *)(w))
#define window_move(w, t, l) (w)->vtable->move((window *)(w), t, l)
#define window_resize(w, h, x) (w)->vtable->resize((window *)(w), h, x)
#define window_moveresize(w, t, l, h, x)                                       \
    (w)->vtable->moveresize((window *)(w), t, l, h, x)

buffer *window_split(window *w, bool is_vsp, bool is_pos_lt);
buffer *window_find_right(window *w, int y);
buffer *window_find_left(window *w, int y);
buffer *window_find_up(window *w, int x);
buffer *window_find_down(window *w, int x);
buffer *window_find_front(window *w, int y); // 左
buffer *window_find_back(window *w, int y);  // 右
buffer *window_find_bottom(window *w, int x);
buffer *window_find_top(window *w, int x);
#define window_find(d, w, p) window_find_##d((window *)(w), (int)(p))

coord window_calc_minsize(window *w);
#define size_larger_eq(a, b) ((a).y >= (b).y && (a).x >= (b).x)
void window_resize_bottomup(window *s, int h, int w);

extern window_vtable winvt_buffer;
extern window_vtable winvt_split;

void init_window_vtable();

typedef struct buffer {
    WINDOW_COMMON;
    drawer dr;
    textmgr mgr;
    size_t y, x, ideal_x;
    char *file;

    int mode;
    coord sel;
} buffer;

#define buf_len() buf->mgr.text.len
#define buf_line(y) buf->mgr.text.v[y]
#define buf_at(y, x) buf->mgr.text.v[y].v[x]

void buffer_init(buffer *buf, split *parent, int t, int l, int h, int w);
void buffer_free(buffer *buf);

void buffer_move(buffer *buf, int t, int l);
void buffer_resize(buffer *buf, int h, int w);
void buffer_moveresize(buffer *buf, int t, int l, int h, int w);

#define buffer_calc_cursor(buf) drawer_setcursor(&(buf)->dr, (buf)->y, (buf)->x)

void buffer_draw(buffer *buf);

// 返回是否要fallback
bool buffer_prockey(buffer *buf, char_t key);

typedef struct window window;
typedef struct sp_child sp_child;

typedef struct seq(sp_child) sp_children;

typedef struct split {
    WINDOW_COMMON;
    bool is_vsp;
    sp_children chs;
} split;

void split_init(split *sp, split *parent, int t, int l, int h, int w);
void split_free(split *sp);
void split_draw(split *sp);

void split_move(split *sp, int t, int l);
void split_resize(split *sp, int h, int w);
void split_moveresize(split *sp, int t, int l, int h, int w);

void split_resize_child(split *sp, int id, int size);

typedef struct sp_child {
    window *win;
    double ratio; // 以整体为参照
} sp_child;

typedef struct seq(split *) splits;
typedef struct seq(buffer *) buffers;

typedef struct seq(int) intlist;

typedef struct editor {
    screen scr;
    window *gwin;
    buffer *cur;
    bool running;
    int h, w;
    splits sps;
    buffers bufs;
    coord cursor;

    intlist _split_sizes;
} editor;

void editor_init(editor *e);
void editor_free(editor *e);

void editor_quit(editor *e);
void editor_draw(editor *e);
bool editor_prockey(editor *e, char_t key);

void editor_mainloop(editor *e);

split *editor_add_split(editor *e);
buffer *editor_add_buffer(editor *e);

#endif // EDITOR_H
