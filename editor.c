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
    seq_append(e_mode_str, "GETCH");
}

size_t e_mode_add(char *name) {
    seq_append(e_mode_str, name);
    return e_mod_useradd_cur++;
}

void buffer_init(buffer *buf, split *parent, int t, int l, int h, int w) {
    buf->parent = parent;
    buf->t = t, buf->l = l, buf->h = h, buf->w = w;
    buf->fm = editor_add_file(buf->e);
    buf->mgr = &buf->fm->mgr;
    drawer_init(&buf->dr, &buf->e->scr, buf->mgr, t, l, h - 1, w, DRAWER_MODE_HSCROLL);
    drawer_setcfg(&buf->dr, (drawer_config){DRAWER_MODE_HSCROLL, true, 2, 2});
    buf->y = 0, buf->x = buf->ideal_x = 0;
    buf->sel.x = buf->sel.y = 0;
    buf->mode = M_NORMAL;
}

void buffer_free(buffer *buf) {
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
        if (key == 'i') {
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
            coord cursor = text_undo(buf->mgr);
            buf->y = cursor.y, buf->x = buf->ideal_x = cursor.x;
        } else if (key == K_CTRL('r')) {
            coord cursor = text_redo(buf->mgr, -1);
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
            text_delete(buf->mgr, l, coord_new(buf->y, buf->x));
            buf->y = l.y, buf->ideal_x = buf->x = l.x;
        } else if (isprintable(key) || key > 128 && key <= 0x10FFFF && wcwidth(key)) {
            rawstr ins = {&key, 1, 2, sizeof(char_t)};
            coord nxt = text_insert(buf->mgr, coord_new(buf->y, buf->x), ins);
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
            coord nxt = text_delete(buf->mgr, coord_new(buf->y, buf->x), buf->sel);
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
            editor_chmod_command(e, editor_proccmd, NULL);
            editor_sendmsg_charp(e, ":");
        } else {
            return true;
        }
        return false;
    } else if (e->cur->mode == M_COMMAND) {
        if (key == K_ESC || key == K_CTRL('c')) {
            e->msg.len = 0;
            e->cur->mode = M_NORMAL;
        } else if (key == '\n') {
            e->cur->mode = M_NORMAL;
            e_callback_t cb = e->cb;
            size_t len = e->msg.len;
            e->msg.len = 0;
            e->cb(e, e->cb_clos, len);
        } else if (key == K_LEFT) {
            if (e->msg_x > e->msg_start)
                e->msg_x--;
        } else if (key == K_RIGHT) {
            if (e->msg_x < e->msg.len)
                e->msg_x++;
        } else if (key == K_BS) {
            if (e->msg.len && e->msg_x) {
                // 忘记提供seq_delete导致的
                // 2026-8-2
                // 现在提供了
                seq_remove(e->msg, e->msg_x - 1);
                e->msg_x--;
            }
            if (e->msg_x == 0) {
                e->msg.len = 0;
                e->cur->mode = M_NORMAL;
            }
        } else if (isprintable(key) && key != '\t' || key > 128 && key <= 0x10FFFF && wcwidth(key)) {
            seq_insert(e->msg, e->msg_x, &key, 1);
            e->msg_x++;
        } else {
            return true;
        }
        return false;
    } else if (e->cur->mode == M_GETCH) {
        e->input_getch = key;
        e->cur->mode = M_NORMAL;
        e->cb(e, e->cb_clos, 1);
    }
    return true;
}

bool _streq_32_8(char_t *s, char *x, size_t len) {
    if (len == 0 && x && *x)
        return false;
    if (x[len])
        return false;
    for (size_t i = 0; i < len; i++) {
        if (s[i] != x[i])
            return false;
    }
    return true;
}

void editor_proccmd(editor *e, void *clos, size_t len) {
    char_t *cmd = e->msg.v + 1;
    int cmd_len = len - 1;
    int i, j, k, l;
    for (i = 0; i < cmd_len && isspace(cmd[i]); i++)
        ;
    for (j = i; j < cmd_len && !isspace(cmd[j]); j++)
        ;
    for (k = j; k < cmd_len && isspace(cmd[k]); k++)
        ;
    for (l = k; l < cmd_len && !isspace(cmd[l]); l++)
        ;
    char_t *head = cmd + i, *arg = cmd + k;
    int head_len = j - i, arg_len = l - k;
#define match(x) _streq_32_8(head, x, head_len)
    if (match("qa")) {
        editor_quit(e, 0);
    } else if (match("qa!")) {
        editor_quit(e, 1);
    } else if (match("q")) {
        buffer_quit(e->cur, 0);
    } else if (match("q!")) {
        buffer_quit(e->cur, 1);
    } else if (match("e") || match("o")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        buffer_openfile(e->cur, name, false);
    } else if (match("e!") || match("o!")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        buffer_openfile(e->cur, name, true);
    } else if (match("w")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        buffer_writefile(e->cur, name, false);
    } else if (match("w!")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        buffer_writefile(e->cur, name, true);
    } else if (match("vsp")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        e->cur = window_split((window *)e->cur, 1, 1);
        buffer_openfile(e->cur, name, 0);
    } else if (match("vsp!")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        e->cur = window_split((window *)e->cur, 1, 1);
        buffer_openfile(e->cur, name, 1);
    } else if (match("sp")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        e->cur = window_split((window *)e->cur, 0, 1);
        buffer_openfile(e->cur, name, 0);
    } else if (match("sp!")) {
        rawstr name = seq_from_slice(rawstr, arg, arg_len);
        e->cur = window_split((window *)e->cur, 0, 1);
        buffer_openfile(e->cur, name, 1);
    } else {
        editor_sendmsg_charp(e, "Unknown command");
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

void editor_init(editor *e) {
    memset(e, 0, sizeof(editor));
    coord size = get_term_size();
    screen_init(&e->scr, size.y, size.x);
    e->h = size.y, e->w = size.x;
    e->sps = seq_init(splits);
    e->bufs = seq_init(buffers);
    e->files = seq_init(filemgrs);
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
    for (size_t i = 0; i < e->files.len; i++)
        filemgr_free(e->files.v[i]);
    free(e->bufs.v);
    free(e->sps.v);
    free(e->files.v);
}

void editor_quit(editor *e, bool force) {
    if (!force) {
        if (e->cur->fm->ver != e->cur->mgr->undo_cur) {
            editor_sendmsg_charp(e, "File unsaved, use q! to quit or :w to save");
            return;
        }
        for (size_t i = 0; i < e->files.len; i++) {
            filemgr *fm = e->files.v[i];
            if (fm->ver != fm->mgr.undo_cur) {
                buffer_openfile_byfm(e->cur, fm);
                editor_sendmsg_charp(e, "File unsaved, use q! to quit or :w to save");
                return;
            }
        }
    }
    e->running = false;
}

void editor_sendmsg(editor *e, rawstr msg) {
    if (e->msg.v)
        free(e->msg.v);
    e->msg = msg;
    e->msg_updtime = time(0);
    e->msg_x = msg.len;
    e->msg_start = msg.len;
}

void editor_sendmsg_charp(editor *e, char *msg) {
    size_t len = strlen(msg);
    if (!e->msg.v) {
        e->msg = seq_init_reserved(rawstr, len);
    } else {
        seq_expand_to(e->msg, len);
    }
    for (size_t i = 0; i < len; i++)
        e->msg.v[i] = msg[i];
    e->msg.len = len;
    e->msg_updtime = time(0);
    e->msg_x = len;
    e->msg_start = len;
}

void editor_chmod_command(editor *e, e_callback_t cb, void *clos) {
    e->cur->mode = M_COMMAND;
    e->cb = cb;
    e->cb_clos = clos;
}

void editor_chmod_getch(editor *e, e_callback_t cb, void *clos, bool move_cursor) {
    e->cur->mode = M_GETCH;
    e->cb = cb;
    e->cb_clos = clos;
    e->getch_move_cursor = move_cursor;
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

filemgr *editor_add_file(editor *e) {
    filemgr *file = malloc(sizeof(filemgr));
    file->e = e;
    file->is_sync = false;
    file->sync_time = time(0);
    file->name = seq_init_null(rawstr);
    file->path_mbs = file->name_mbs = NULL;
    text_init(&file->mgr, NULL);
    file->ver = file->mgr.undo_cur;
    seq_append(e->files, file);
    return file;
}

filemgr *editor_find_fm(editor *e, char *path_mbs) {
    if (!path_mbs)
        return NULL;
    for (size_t i = 0; i < e->files.len; i++) {
        if (e->files.v[i]->path_mbs && !strcmp(e->files.v[i]->path_mbs, path_mbs))
            return e->files.v[i];
    }
    return NULL;
}

window_vtable winvt_buffer, winvt_split;

void init_window_vtable() {
    winvt_buffer.draw = (void (*)(window *))buffer_draw;
    winvt_buffer.free = (void (*)(window *))buffer_free;
    winvt_buffer.move = (void (*)(window *, int, int))buffer_move;
    winvt_buffer.resize = (void (*)(window *, int, int))buffer_resize;
    winvt_buffer.moveresize = (void (*)(window *, int, int, int, int))buffer_moveresize;

    winvt_split.draw = (void (*)(window *))split_draw;
    winvt_split.free = (void (*)(window *))split_free;
    winvt_split.move = (void (*)(window *, int, int))split_move;
    winvt_split.resize = (void (*)(window *, int, int))split_resize;
    winvt_split.moveresize = (void (*)(window *, int, int, int, int))split_moveresize;
}
