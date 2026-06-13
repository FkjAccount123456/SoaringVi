#include "editor.h"
#include "drawer.h"
#include "screen.h"
#include "utils.h"
#include "wcwidth/wcwidth.h"
#include <stdbool.h>
#include <stddef.h>

charp_list e_mode_str;

size_t e_mod_useradd_cur = M_USRADD;

void e_mode_init() {
    e_mode_str = seq_init(charp_list);
    seq_append(e_mode_str, "NORMAL");
    seq_append(e_mode_str, "INSERT");
    seq_append(e_mode_str, "VISUAL");
    seq_append(e_mode_str, "COMMAND");
}

size_t e_mode_add(char *name) {
    seq_append(e_mode_str, name);
    return e_mod_useradd_cur++;
}

void buffer_init(buffer *buf, split *parent, int t, int l, int h, int w) {
    buf->parent = parent;
    buf->t = t, buf->l = l, buf->h = h, buf->w = w;
    text_init(&buf->mgr);
    drawer_init(&buf->dr, &buf->e->scr, &buf->mgr, t, l, h - 1, w,
                DRAWER_MODE_HSCROLL);
    drawer_setcfg(&buf->dr, (drawer_config){DRAWER_MODE_HSCROLL, true, 2, 2});
    buf->y = 0, buf->x = buf->ideal_x = 0;
    buf->sel.x = buf->sel.y = 0;
    buf->mode = M_NORMAL;
    buf->file = NULL;
}

void buffer_free(buffer *buf) {
    text_free(&buf->mgr);
}

void buffer_move(buffer *buf, int t, int l) {
    drawer_move(&buf->dr, t, l);
    buf->t = t, buf->l = l;
}

void buffer_resize(buffer *buf, int h, int w) {
    drawer_resize(&buf->dr, h - 1, w);
    buf->h = h, buf->w = w;
}

void buffer_moveresize(buffer *buf, int t, int l, int h, int w) {
    buffer_move(buf, t, l);
    buffer_resize(buf, h, w);
}

#define colortext_statusline(x)                                                \
    (colortext) {                                                              \
        .fg = {0, 0, 0}, .bg = {192, 192, 192}, .style = 0, .ch = x            \
    }

void _buffer_draw_modeline(buffer *buf) {
    for (int i = 0; i < buf->w; i++)
        bscreen_change(buf->h - 1, i, colortext_statusline(L' '));
    if (buf->file)
        for (int i = 0; buf->file[i]; i++)
            bscreen_change(buf->h - 1, i + 1,
                           colortext_statusline(buf->file[i]));
}

#define colortext_note(x)                                                      \
    (colortext) {                                                              \
        .fg = {0, 0, 0}, .bg = {255, 0, 0}, .style = 0, .ch = x                \
    }

void buffer_draw(buffer *buf) {
    if (buf->h <= 0 || buf->w <= 0)
        return;
    coord cursor = drawer_setcursor(&buf->dr, buf->y, buf->x);
    if (buf->mode == M_VISUAL)
        drawer_draw(&buf->dr, 1, buf->sel);
    else
        drawer_draw_nosel(&buf->dr);
    if (buf->e->cur == buf && buf->mode != M_COMMAND)
        buf->e->cursor = cursor;
    _buffer_draw_modeline(buf);
#ifdef DEBUG_MODE
    drawer *dr = &buf->dr;
    vscreen_change(0, 0, colortext_note(' '));
    vscreen_change(0, dr->full_w - 1, colortext_note(' '));
    vscreen_change(dr->h - 1, 0, colortext_note(' '));
    vscreen_change(dr->h - 1, dr->full_w - 1, colortext_note(' '));
#endif // DEBUG_MODE
}

void _buffer_cursor_up(buffer *buf) {
    if (buf->y == 0) {
        buf->x = buf->ideal_x = 0;
    } else {
        buf->y--;
        buf->x = min(buf->ideal_x, buf_line(buf->y).len);
    }
}

void _buffer_cursor_down(buffer *buf) {
    if (buf->y == buf_len() - 1) {
        buf->x = buf->ideal_x = buf_line(buf->y).len;
    } else {
        buf->y++;
        buf->x = min(buf->ideal_x, buf_line(buf->y).len);
    }
}

void _buffer_cursor_left(buffer *buf) {
    if (buf->x == 0) {
        if (buf->y == 0)
            return;
        buf->y--;
        buf->x = buf->ideal_x = buf_line(buf->y).len;
    } else {
        buf->x = buf->ideal_x = buf->x - 1;
    }
}

void _buffer_cursor_right(buffer *buf) {
    if (buf->x == buf_line(buf->y).len) {
        if (buf->y == buf_len() - 1)
            return;
        buf->y++;
        buf->x = buf->ideal_x = 0;
    } else {
        buf->x = buf->ideal_x = buf->x + 1;
    }
}

// 仍然是临时的
bool buffer_prockey(buffer *buf, char_t key) {
    if (buf->mode == M_NORMAL) {
        if (key == K_CTRL('c')) {
            editor_quit(buf->e);
        } else if (key == 'i') {
            buf->mode = M_INSERT;
        } else if (key == 'v') {
            buf->mode = M_VISUAL;
            buf->sel = coord_new(buf->y, buf->x);
        } else if (key == K_UP || key == 'k') {
            _buffer_cursor_up(buf);
        } else if (key == K_DOWN || key == 'j') {
            _buffer_cursor_down(buf);
        } else if (key == K_LEFT || key == 'h') {
            _buffer_cursor_left(buf);
        } else if (key == K_RIGHT || key == 'l') {
            _buffer_cursor_right(buf);
        } else if (key == 'u') {
            coord cursor = text_undo(&buf->mgr);
            buf->y = cursor.y, buf->x = buf->ideal_x = cursor.x;
        } else if (key == K_CTRL('r')) {
            coord cursor = text_redo(&buf->mgr, -1);
            if (cursor.x != -1 && cursor.y != -1)
                buf->y = cursor.y, buf->x = buf->ideal_x = cursor.x;
        } else {
            return true;
        }
        return false;
    } else if (buf->mode == M_INSERT) {
        if (key == K_CTRL('c') || key == K_ESC) {
            buf->mode = M_NORMAL;
        } else if (key == K_UP) {
            _buffer_cursor_up(buf);
        } else if (key == K_DOWN) {
            _buffer_cursor_down(buf);
        } else if (key == K_LEFT) {
            _buffer_cursor_left(buf);
        } else if (key == K_RIGHT) {
            _buffer_cursor_right(buf);
        } else if (key == K_BS) {
            coord l;
            if (buf->x) {
                l = coord_new(buf->y, buf->x - 1);
            } else if (buf->y) {
                l = coord_new(buf->y - 1, buf_line(buf->y - 1).len);
            } else {
                return false;
            }
            text_delete(&buf->mgr, l, coord_new(buf->y, buf->x));
            buf->y = l.y, buf->ideal_x = buf->x = l.x;
        } else if (isprintable(key) || key > 128 && wcwidth(key)) {
            rawstr ins = {&key, 1, 2, sizeof(char_t)};
            coord nxt = text_insert(&buf->mgr, coord_new(buf->y, buf->x), ins);
            buf->y = nxt.y, buf->x = buf->ideal_x = nxt.x;
        } else {
            return true;
        }
        return false;
    } else if (buf->mode == M_VISUAL) {
        if (key == K_CTRL('c') || key == K_ESC) {
            buf->mode = M_NORMAL;
        } else if (key == K_UP || key == 'k') {
            _buffer_cursor_up(buf);
        } else if (key == K_DOWN || key == 'j') {
            _buffer_cursor_down(buf);
        } else if (key == K_LEFT || key == 'h') {
            _buffer_cursor_left(buf);
        } else if (key == K_RIGHT || key == 'l') {
            _buffer_cursor_right(buf);
        } else if (key == 'd') {
            coord nxt =
                text_delete(&buf->mgr, coord_new(buf->y, buf->x), buf->sel);
            buf->y = nxt.y, buf->x = buf->ideal_x = nxt.x;
            buf->mode = M_NORMAL;
        } else {
            return true;
        }
        return false;
    }
    return true;
}

bool editor_prockey(editor *e, char_t key) {
    bool buffer_miss = buffer_prockey(e->cur, key);
    if (!buffer_miss)
        return false;
    if (key == K_CTRL('w')) {
        key = u_getch();
        if (key == 'h') {
            e->cur = window_split((window *)e->cur, 1, 1);
        } else if (key == 'l') {
            e->cur = window_split((window *)e->cur, 1, 0);
        } else if (key == 'k') {
            e->cur = window_split((window *)e->cur, 0, 1);
        } else if (key == 'j') {
            e->cur = window_split((window *)e->cur, 0, 0);
        } else if (key == K_CTRL('h') || key == K_BS) { // 可恶的Windows
            size_t y = buffer_calc_cursor(e->cur).y;
            buffer *new = window_find(left, (window *)e->cur, y);
            if (new)
                e->cur = new;
        } else if (key == K_CTRL('l')) {
            size_t y = buffer_calc_cursor(e->cur).y;
            buffer *new = window_find(right, (window *)e->cur, y);
            if (new)
                e->cur = new;
        } else if (key == K_CTRL('k')) {
            size_t x = buffer_calc_cursor(e->cur).x;
            buffer *new = window_find(up, (window *)e->cur, x);
            if (new)
                e->cur = new;
        } else if (key == K_CTRL('j')) {
            size_t x = buffer_calc_cursor(e->cur).x;
            buffer *new = window_find(down, (window *)e->cur, x);
            if (new)
                e->cur = new;
        } else {
            return true;
        }
        return false;
    } else if (e->cur->mode == M_NORMAL) {
        if (key == K_C_UP) {
            window_resize_bottomup((window *)e->cur, e->cur->h + 1, e->cur->w);
        } else if (key == K_C_DOWN) {
            window_resize_bottomup((window *)e->cur, e->cur->h - 1, e->cur->w);
        } else if (key == K_C_LEFT) {
            window_resize_bottomup((window *)e->cur, e->cur->h, e->cur->w - 1);
        } else if (key == K_C_RIGHT) {
            window_resize_bottomup((window *)e->cur, e->cur->h, e->cur->w + 1);
        } else if (key == ':') {
            editor_chmod_command(e);
        } else {
            return true;
        }
        return false;
    } else if (e->cur->mode == M_COMMAND) {
        if (key == K_ESC) {
            e->msg.len = 0;
            e->cur->mode = M_NORMAL;
        } else if (key == '\n') {
            e->cur->mode = M_NORMAL;
            editor_proccmd(e);
            e->msg.len = 0;
        } else if (key == K_LEFT) {
            if (e->msg_x > e->msg_start)
                e->msg_x--;
        } else if (key == K_RIGHT) {
            if (e->msg_x < e->msg.len)
                e->msg_x++;
        } else if (key == K_BS) {
            if (e->msg.len && e->msg_x) {
                // 忘记提供seq_delete导致的
                memmove(e->msg.v + e->msg_x - 1, e->msg.v + e->msg_x,
                        e->msg.len - e->msg_x);
                e->msg.len--;
                e->msg_x--;
            }
            if (e->msg_x == 0) {
                e->msg.len = 0;
                e->cur->mode = M_NORMAL;
            }
        } else if (isprintable(key) || key > 128 && wcwidth(key)) {
            seq_insert(e->msg, e->msg_x, &key, 1);
            e->msg_x++;
        } else {
            return true;
        }
        return false;
    }
    return true;
}

bool _streq_32_8(char_t *s, char *x, size_t len) {
    if (len == 0 && x && *x)
        return false;
    for (size_t i = 0; i < len; i++) {
        if (s[i] != x[i])
            return false;
    }
    return true;
}

void editor_proccmd(editor *e) {
    char_t *cmd = e->msg.v + 1;
    int cmd_len = e->msg.len - 1;
    int i, j, k, l;
    for (i = 0; i < cmd_len && isspace(cmd[i]); i++)
        ;
    for (j = i; j < cmd_len && !isspace(cmd[j]); j++)
        ;
    for (k = j; k < cmd_len && isspace(cmd[k]); k++)
        ;
    for (l = k; l < cmd_len && !isspace(cmd[l]); l++)
        ;
    char_t *head = cmd + i;
    int head_len = j - i;
#define match(x) _streq_32_8(head, x, head_len)
    if (match("q")) {
        editor_quit(e);
    }
#undef match
}

void split_init(split *sp, split *parent, int t, int l, int h, int w) {
    sp->parent = parent;
    sp->t = t, sp->l = l, sp->h = h, sp->w = w;
    sp->chs = seq_init_reserved(sp_children, 2);
}

void split_free(split *sp) {
    for (int i = 0; i < sp->chs.len; i++)
        window_free(sp->chs.v[i].win);
    seq_free(sp->chs);
}

#define colortext_normal(c)                                                    \
    (colortext){.ch = c, .bg = {0, 0, 0}, .fg = {192, 192, 192}, .style = 0}

void split_draw(split *sp) {
    window_draw(sp->chs.v[0].win);
    for (int i = 1; i < sp->chs.len; i++) {
        if (sp->is_vsp)
            for (int y = 0; y < sp->h; y++)
                screen_change(&sp->e->scr, sp->t + y, sp->chs.v[i].win->l - 1,
                              colortext_normal('|'));
        window_draw(sp->chs.v[i].win);
    }
}

void split_move(split *sp, int t, int l) {
    long long dt = t - sp->t, dl = l - sp->l;
    for (int i = 0; i < sp->chs.len; i++)
        window_move(sp->chs.v[i].win, sp->chs.v[i].win->t + dt,
                    sp->chs.v[i].win->l + dl);
    sp->t = t, sp->l = l;
}

void split_resize(split *sp, int h, int w) {
    split_moveresize(sp, sp->t, sp->l, h, w);
}

// 怎么就没想到根据大小算比例呢
void _split_adjust_ratio(split *sp) {
    if (sp->is_vsp) {
        for (int i = 0; i < sp->chs.len; i++)
            sp->chs.v[i].ratio =
                (sp->chs.v[i].win->w + (i != sp->chs.len - 1)) / (double)sp->w;
    } else {
        for (int i = 0; i < sp->chs.len; i++)
            sp->chs.v[i].ratio = sp->chs.v[i].win->h / (double)sp->h;
    }
}

void _split_relay_bysize(split *sp) {
    if (sp->is_vsp) {
        size_t pos = 0;
        for (int i = 0; i < sp->chs.len; i++) {
            window *win = sp->chs.v[i].win;
            window_moveresize(win, sp->t, sp->l + pos, sp->h, win->w);
            pos += win->w + 1;
        }
    } else {
        size_t pos = 0;
        for (int i = 0; i < sp->chs.len; i++) {
            window *win = sp->chs.v[i].win;
            window_moveresize(win, sp->t + pos, sp->l, win->h, sp->w);
            pos += win->h;
        }
    }
    _split_adjust_ratio(sp);
}

void _split_relay(split *sp) {
    seq_expand_to(sp->e->_split_sizes, sp->chs.len + 1);
    int *postsum = sp->e->_split_sizes.v, sum_min = 0;
    int pos = 0, sum_size;
    if (!sp->is_vsp) {
        postsum[sp->chs.len] = 0;
        for (int i = sp->chs.len; i > 0; i--)
            postsum[i - 1] =
                postsum[i] + window_calc_minsize(sp->chs.v[i - 1].win).y;
        sum_size = sp->h;
        for (int i = 0; i < sp->chs.len - 1; i++) {
            window *ch = sp->chs.v[i].win;
            int min_size = window_calc_minsize(ch).y;
            int max_size = sum_size - postsum[i + 1];
            max_size = max(0, max_size);
            int new_size =
                min(max_size, max(min_size, (int)(sp->h * sp->chs.v[i].ratio)));
            sum_size -= new_size;
            window_moveresize(ch, sp->t + pos, sp->l, new_size, sp->w);
            pos += new_size;
        }
        window_moveresize(seq_end(sp->chs).win, sp->t + pos, sp->l, sum_size,
                          sp->w);
    } else {
        postsum[sp->chs.len] = -1;
        for (int i = sp->chs.len; i > 0; i--)
            postsum[i - 1] =
                postsum[i] + window_calc_minsize(sp->chs.v[i - 1].win).x + 1;
        sum_size = sp->w;
        for (int i = 0; i < sp->chs.len - 1; i++) {
            window *ch = sp->chs.v[i].win;
            int min_size = window_calc_minsize(ch).x;
            int max_size = sum_size - postsum[i + 1] - 1;
            max_size = max(0, max_size);
            int new_size =
                min(max_size, max(min_size, (int)(sp->w * sp->chs.v[i].ratio)));
            sum_size -= new_size + 1;
            window_moveresize(ch, sp->t, sp->l + pos, sp->h, new_size);
            pos += new_size + 1;
        }
        window_moveresize(seq_end(sp->chs).win, sp->t, sp->l + pos, sp->h,
                          sum_size);
    }
}

void split_moveresize(split *sp, int t, int l, int h, int w) {
    sp->t = t, sp->l = l, sp->h = h, sp->w = w;
    _split_relay(sp);
}

// 先向上/左增长，不行的话再向右、下
void split_resize_child(split *sp, int id, int size) {
    if (id >= sp->chs.len)
        return;
    seq_expand_to(sp->e->_split_sizes, sp->chs.len + 1);
    int *minsizes = sp->e->_split_sizes.v;
    window *ch = sp->chs.v[id].win;
    // 颇有一种几个月前考完期末考试回来写绘制部分的感觉
    // 2026-5-24
    // 得，全部重写
    if (!sp->is_vsp) {
        map_res(minsizes, window_calc_minsize(sp->chs.v[i].win).y, sp->chs.len);
        size = max(size, minsizes[id]);
        int lt_minsize = e_sizesum(0, id, minsizes[i]),
            rb_minsize = e_sizesum(id + 1, sp->chs.len, minsizes[i]);
        size = min(size, sp->h - lt_minsize - rb_minsize);
        int size_diff = size - ch->h;
        if (size_diff > 0) {
            for (int i = id - 1; i >= 0 && size_diff; i--) {
                if (sp->chs.v[i].win->h - minsizes[i] >= size_diff) {
                    sp->chs.v[i].win->h -= size_diff;
                    size_diff = 0;
                } else {
                    size_diff -= sp->chs.v[i].win->h - minsizes[i];
                    sp->chs.v[i].win->h = minsizes[i];
                }
            }
            for (int i = id + 1; i < sp->chs.len && size_diff; i++) {
                if (sp->chs.v[i].win->h - minsizes[i] >= size_diff) {
                    sp->chs.v[i].win->h -= size_diff;
                    size_diff = 0;
                } else {
                    size_diff -= sp->chs.v[i].win->h - minsizes[i];
                    sp->chs.v[i].win->h = minsizes[i];
                }
            }
        } else if (size_diff < 0) {
            if (id < sp->chs.len - 1)
                sp->chs.v[id + 1].win->h -= size_diff;
            else
                sp->chs.v[id - 1].win->h -= size_diff;
        }
        ch->h = size;
    } else { // ？？？怎么还有
        map_res(minsizes, window_calc_minsize(sp->chs.v[i].win).x, sp->chs.len);
        size = max(size, minsizes[id]);
        int lt_minsize = e_sizesum(0, id, minsizes[i] + 1),
            rb_minsize = e_sizesum(id + 1, sp->chs.len, minsizes[i] + 1);
        size = min(size, sp->w - lt_minsize - rb_minsize);
        int size_diff = size - ch->w;
        if (size_diff > 0) {
            for (int i = id - 1; i >= 0 && size_diff; i--) {
                if (sp->chs.v[i].win->w - minsizes[i] >= size_diff) {
                    sp->chs.v[i].win->w -= size_diff;
                    size_diff = 0;
                } else {
                    size_diff -= sp->chs.v[i].win->w - minsizes[i];
                    sp->chs.v[i].win->w = minsizes[i];
                }
            }
            for (int i = id + 1; i < sp->chs.len && size_diff; i++) {
                if (sp->chs.v[i].win->w - minsizes[i] >= size_diff) {
                    sp->chs.v[i].win->w -= size_diff;
                    size_diff = 0;
                } else {
                    size_diff -= sp->chs.v[i].win->w - minsizes[i];
                    sp->chs.v[i].win->w = minsizes[i];
                }
            }
        } else if (size_diff < 0) {
            if (id < sp->chs.len - 1)
                sp->chs.v[id + 1].win->w -= size_diff;
            else
                sp->chs.v[id - 1].win->w -= size_diff;
        }
        ch->w = size;
    }
    _split_relay_bysize(sp);
}

void _window_replaceself(window *w, window *new) {
    if (w->parent)
        for (int i = 0; i < w->parent->chs.len; i++) {
            if (w->parent->chs.v[i].win == w) {
                w->parent->chs.v[i].win = new;
                return;
            }
        }
    else
        w->e->gwin = new;
}

buffer *window_split(window *w, bool is_vsp, bool is_pos_lt) {
    if (!w->is_buf && ((split *)w)->is_vsp == is_vsp) {
        split *sp = (split *)w;
        buffer *new = editor_add_buffer(sp->e);
        buffer_init(new, sp, sp->t, sp->l, sp->h, sp->w);
        double ratio = (double)1.0 / (sp->chs.len + 1);
        sp_child ch = {.win = (window *)new, .ratio = ratio};
        for (int i = 0; i < sp->chs.len; i++)
            sp->chs.v[i].ratio = ratio;
        if (is_pos_lt)
            seq_insert(sp->chs, 0, &ch, 1);
        else
            seq_append(sp->chs, ch);
        _split_relay(sp);
        return new;
    } else if (w->parent && w->parent->is_vsp == is_vsp) {
        split *sp = w->parent;
        buffer *new = editor_add_buffer(sp->e);
        buffer_init(new, sp, sp->t, sp->l, sp->h, sp->w);
        double ratio = (double)1.0 / (sp->chs.len + 1);
        sp_child ch = {.win = (window *)new, .ratio = ratio};
        int idx = 0;
        for (int i = 0; i < sp->chs.len; i++) {
            sp->chs.v[i].ratio = ratio;
            if (sp->chs.v[i].win == w)
                idx = i;
        }
        if (is_pos_lt)
            seq_insert(sp->chs, idx, &ch, 1);
        else
            seq_insert(sp->chs, idx + 1, &ch, 1);
        _split_relay(sp);
        return new;
    } else {
        split *sp = editor_add_split(w->e);
        split_init(sp, w->parent, w->t, w->l, w->h, w->w);
        buffer *new = editor_add_buffer(sp->e);
        buffer_init(new, sp, sp->t, sp->l, sp->h, sp->w);
        sp->is_vsp = is_vsp;
        if (is_pos_lt) {
            sp_child ch = {.win = (window *)new, .ratio = 0.5};
            seq_append(sp->chs, ch);
            ch.win = w;
            seq_append(sp->chs, ch);
        } else {
            sp_child ch = {.win = w, .ratio = 0.5};
            seq_append(sp->chs, ch);
            ch.win = (window *)new;
            seq_append(sp->chs, ch);
        }
        _split_relay(sp);
        _window_replaceself(w, (window *)sp);
        w->parent = sp;
        return new;
    }
}

int _window_findselfindex(window *w) {
    if (!w->parent)
        return -1;
    for (int i = 0; i < w->parent->chs.len; i++)
        if (w->parent->chs.v[i].win == w)
            return i;
    return -1;
}

// 没想到宏还有此等妙用，让我对Lisp宏更加期待了
// 2026-6-6
// 完了，怎么调试啊
#define gen_window_find(d, drev, d_is_vsp, wfinal, d_op)                       \
    buffer *window_find_##d(window *w, int y) {                                \
        if (!w->parent)                                                        \
            return NULL;                                                       \
        if (w->parent->is_vsp != d_is_vsp)                                     \
            return window_find(d, w->parent, y);                               \
        int idx = _window_findselfindex(w);                                    \
        if (idx == -1)                                                         \
            return NULL;                                                       \
        if (idx == (wfinal))                                                   \
            return window_find(d, w->parent, y);                               \
        return window_find(drev, w->parent->chs.v[idx d_op].win, y);           \
    }

gen_window_find(right, front, true, w->parent->chs.len - 1, +1);
gen_window_find(left, back, true, 0, -1);
gen_window_find(down, top, false, w->parent->chs.len - 1, +1);
gen_window_find(up, bottom, false, 0, -1);

#undef gen_window_find

#define gen_window_find_in(d, d_is_vsp, wfinal, attr)                          \
    buffer *window_find_##d(window *_w, int p) {                               \
        if (_w->is_buf)                                                        \
            return (buffer *)_w;                                               \
        split *w = (split *)_w;                                                \
        if (w->is_vsp != d_is_vsp) {                                           \
            for (int i = 0; i < w->chs.len - 1; i++)                           \
                if (w->chs.v[i + 1].win->attr > p)                             \
                    return window_find(d, w->chs.v[i].win, p);                 \
            return window_find(d, seq_end(w->chs).win, p);                     \
        }                                                                      \
        return window_find(d, w->chs.v[wfinal].win, p);                        \
    }

gen_window_find_in(front, true, 0, t);
gen_window_find_in(back, true, w->chs.len - 1, t);
gen_window_find_in(top, false, 0, l);
gen_window_find_in(bottom, false, w->chs.len - 1, l);

#undef gen_window_find_in

coord window_calc_minsize(window *w) {
    if (w->is_buf) {
        return BUFFER_MIN_SIZE;
    }
    split *sp = (split *)w;
    coord size = {0, 0};
    if (sp->is_vsp) {
        for (int i = 0; i < sp->chs.len; i++) {
            coord new = window_calc_minsize(sp->chs.v[i].win);
            size.x += new.x + 1;
            if (size.y < new.y)
                size.y = new.y;
        }
        size.x--;
    } else {
        for (int i = 0; i < sp->chs.len; i++) {
            coord new = window_calc_minsize(sp->chs.v[i].win);
            size.y += new.y;
            if (size.x < new.x)
                size.x = new.x;
        }
    }
    return size;
}

// 无法理解为什么TermEd只用20多行代码就能实现
// 好吧，归根结底还是SoaringVi的窗口设计太复杂了，不过确实是这样呈现效果最好
// 首先我需要实现一个change_pos
// 2026-5-25
// 貌似行了，也就二十多行的样子，然而change_pos对应物都快百行了
void window_resize_bottomup(window *s, int h, int w) {
    if (!s->parent)
        return;
    coord s_minsize = window_calc_minsize(s);
    h = max(h, s_minsize.y), w = max(w, s_minsize.x);
    split *p = s->parent;
    int minsize = 0;
    if (!p->is_vsp) {
        minsize =
            e_sizesum(0, p->chs.len, window_calc_minsize(p->chs.v[i].win).y);
        minsize += h - s_minsize.y;
        if (p->h < minsize)
            window_resize_bottomup((window *)p, minsize, w);
        else if (p->w != w)
            window_resize_bottomup((window *)p, p->h, w);
        split_resize_child(p, _window_findselfindex(s), h);
    } else {
        minsize = e_sizesum(0, p->chs.len,
                            window_calc_minsize(p->chs.v[i].win).x + 1);
        minsize += w - s_minsize.x - 1;
        if (p->w < minsize)
            window_resize_bottomup((window *)p, h, minsize);
        else if (p->h != h)
            window_resize_bottomup((window *)p, h, p->w);
        split_resize_child(p, _window_findselfindex(s), w);
    }
}

void editor_init(editor *e) {
    memset(e, 0, sizeof(editor));
    coord size = get_term_size();
    screen_init(&e->scr, size.y, size.x);
    e->h = size.y, e->w = size.x;
    e->sps = seq_init(splits);
    e->bufs = seq_init(buffers);
    e->cur = editor_add_buffer(e);
    buffer_init(e->cur, NULL, 0, 0, e->h - 1, e->w);
    e->gwin = (window *)e->cur;
    e->running = false;

    e->msg.v = NULL;

    e->_split_sizes = seq_init(intlist);
}

void editor_free(editor *e) {
    screen_free(&e->scr);
    window_free(e->gwin);

    free(e->_split_sizes.v);

    for (size_t i = 0; i < e->bufs.len; i++)
        free(e->bufs.v[i]);
    for (size_t i = 0; i < e->sps.len; i++)
        free(e->sps.v[i]);
    free(e->bufs.v);
    free(e->sps.v);
}

void editor_quit(editor *e) {
    e->running = false;
}

void _editor_draw_msg(editor *e) {
    if (e->cur->mode != M_NORMAL && e->cur->mode != M_COMMAND) {
        if (e->gwin->h != e->h - 1)
            window_resize(e->gwin, e->h - 1, e->w);
        char *mode_str = e_mode_str.v[e->cur->mode];
        int w = 0;
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal(' '));
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal('-'));
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal('-'));
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal(' '));
        for (int i = 0; mode_str[i]; i++) {
            int cw = wcwidth(mode_str[i]);
            screen_change(&e->scr, e->gwin->h, w,
                          colortext_normal(mode_str[i]));
            w += cw;
        }
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal(' '));
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal('-'));
        screen_change(&e->scr, e->gwin->h, w++, colortext_normal('-'));
        while (w < e->w)
            screen_change(&e->scr, e->gwin->h, w++, colortext_normal(' '));
        return;
    }

    int h = 1, w = 0, i;
    for (int i = 0; i < e->msg.len; i++) {
        int cw = wcwidth(e->msg.v[i]);
        if (w + cw > e->w)
            h++, w = 0;
        w += cw;
    }
    if (e->gwin->h != e->h - h)
        window_resize(e->gwin, e->h - h, e->w);
    h = 0, w = 0;
    for (i = 0; i < e->msg.len; i++) {
        int cw = wcwidth(e->msg.v[i]);
        if (i == e->msg_x && e->cur->mode == M_COMMAND) {
            if (w < e->w)
                e->cursor.y = e->gwin->h + h, e->cursor.x = w;
            else
                e->cursor.y = e->gwin->h + h + 1, e->cursor.x = 0;
        }
        if (w + cw > e->w)
            h++, w = 0;
        screen_change(&e->scr, e->gwin->h + h, w,
                      colortext_normal(e->msg.v[i]));
        w += cw;
    }
    if (i == e->msg_x && e->cur->mode == M_COMMAND)
        e->cursor.y = e->gwin->h + h, e->cursor.x = w;
    while (w < e->w)
        screen_change(&e->scr, e->gwin->h + h, w++, colortext_normal(' '));
}

void editor_draw(editor *e) {
    _editor_draw_msg(e);

    window_draw(e->gwin);

    screen_flush(&e->scr);
    gotoxy(e->cursor.y, e->cursor.x);
}

void editor_sendmsg(editor *e, rawstr msg) {
    if (e->msg.v)
        free(e->msg.v);
    e->msg = msg;
    e->msg_updtime = time(0);
}

void editor_chmod_command(editor *e) {
    if (e->msg.v)
        e->msg.len = 0;
    else
        e->msg = seq_init(rawstr);
    seq_append(e->msg, ':');
    e->cur->mode = M_COMMAND;
    e->msg_x = 1;
    e->msg_start = 1;
    e->msg_updtime = time(0);
}

void editor_mainloop(editor *e) {
    e->running = true;

    while (e->running) {
        editor_draw(e);
        flush();
        char_t ch = u_getch();
        editor_prockey(e, ch);
    }
}

split *editor_add_split(editor *e) {
    split *sp = malloc(sizeof(split));
    sp->e = e;
    sp->is_buf = 0;
    sp->vtable = &winvt_split;
    seq_append(e->sps, sp);
    return seq_end(e->sps);
}

buffer *editor_add_buffer(editor *e) {
    buffer *buf = malloc(sizeof(buffer));
    buf->e = e;
    buf->is_buf = 1;
    buf->vtable = &winvt_buffer;
    seq_append(e->bufs, buf);
    return seq_end(e->bufs);
}

window_vtable winvt_buffer, winvt_split;

void init_window_vtable() {
    winvt_buffer.draw = (void (*)(window *))buffer_draw;
    winvt_buffer.free = (void (*)(window *))buffer_free;
    winvt_buffer.move = (void (*)(window *, int, int))buffer_move;
    winvt_buffer.resize = (void (*)(window *, int, int))buffer_resize;
    winvt_buffer.moveresize =
        (void (*)(window *, int, int, int, int))buffer_moveresize;

    winvt_split.draw = (void (*)(window *))split_draw;
    winvt_split.free = (void (*)(window *))split_free;
    winvt_split.move = (void (*)(window *, int, int))split_move;
    winvt_split.resize = (void (*)(window *, int, int))split_resize;
    winvt_split.moveresize =
        (void (*)(window *, int, int, int, int))split_moveresize;
}
