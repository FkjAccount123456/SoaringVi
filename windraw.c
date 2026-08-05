#include "colorscheme.h"
#include "editor.h"
#include "wcwidth/wcwidth.h"

void _buffer_draw_modeline(buffer *buf) {
    for (int i = 0; i < buf->w; i++)
        bscreen_change(buf->h - 1, i, color_get(&color_quiet, L' ', CTE_MODELINE, 0));
    if (buf->fm->name.len) {
        int i = 0;
        for (i = 0; i < buf->fm->name.len; i++)
            bscreen_change(buf->h - 1, i + 1, color_get(&color_quiet, buf->fm->name.v[i], CTE_MODELINE, 0));
        if (buf->mgr->undo_cur != buf->fm->ver) {
            char s[] = " [+]";
            char j = 0;
            for (; s[j]; i++, j++)
                bscreen_change(buf->h - 1, i + 1, color_get(&color_quiet, s[j], CTE_MODELINE, 0));
        }
    } else {
        char s[] = "untitled [+]";
        int i;
        if (buf->mgr->undo_cur != buf->fm->ver)
            for (i = 0; s[i]; i++)
                bscreen_change(buf->h - 1, i + 1, color_get(&color_quiet, s[i], CTE_MODELINE, 0));
        else
            for (i = 0; s[i] != ' '; i++)
                bscreen_change(buf->h - 1, i + 1, color_get(&color_quiet, s[i], CTE_MODELINE, 0));
    }
}

void buffer_draw(buffer *buf) {
    if (buf->h <= 0 || buf->w <= 0)
        return;
    if (buf->y >= buf->mgr->text.len)
        buf->y = buf->mgr->text.len - 1;
    if (buf->x > buf_line(buf->y).len)
        buf->x = buf_line(buf->y).len;
    coord cursor = drawer_setcursor(&buf->dr, buf->y, buf->x);
    if (buf->mode == M_VISUAL)
        drawer_draw(&buf->dr, 1, buf->sel);
    else
        drawer_draw_nosel(&buf->dr);
    if (buf->e->cur == buf && !(buf->mode == M_COMMAND || buf->mode == M_GETCH && buf->e->getch_move_cursor))
        buf->e->cursor = cursor;
    _buffer_draw_modeline(buf);
}

void split_draw(split *sp) {
    window_draw(sp->chs.v[0].win);
    for (int i = 1; i < sp->chs.len; i++) {
        if (sp->is_vsp)
            for (int y = 0; y < sp->h; y++)
                screen_change(&sp->e->scr, sp->t + y, sp->chs.v[i].win->l - 1, color_get(&color_quiet, '|', CTE_BORDER, 0));
        window_draw(sp->chs.v[i].win);
    }
}

void _editor_draw_msg(editor *e) {
    if (e->cur->mode != M_NORMAL && e->cur->mode != M_COMMAND && e->cur->mode != M_GETCH) {
        if (e->gwin->h != e->h - 1)
            window_resize(e->gwin, e->h - 1, e->w);
        char *mode_str = e_mode_str.v[e->cur->mode];
        int w = 0;
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, ' ', CTE_CMDLINE, 0));
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, '-', CTE_CMDLINE, 0));
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, '-', CTE_CMDLINE, 0));
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, ' ', CTE_CMDLINE, 0));
        for (int i = 0; mode_str[i]; i++) {
            int cw = wcwidth(mode_str[i]);
            screen_change(&e->scr, e->gwin->h, w, color_get(&color_quiet, mode_str[i], CTE_CMDLINE, 0));
            w += cw;
        }
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, ' ', CTE_CMDLINE, 0));
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, '-', CTE_CMDLINE, 0));
        screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, '-', CTE_CMDLINE, 0));
        while (w < e->w)
            screen_change(&e->scr, e->gwin->h, w++, color_get(&color_quiet, ' ', CTE_CMDLINE, 0));
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
        screen_change(&e->scr, e->gwin->h + h, w, color_get(&color_quiet, e->msg.v[i], CTE_CMDLINE, 0));
        w += cw;
    }
    if (i == e->msg_x && e->cur->mode == M_COMMAND)
        e->cursor.y = e->gwin->h + h, e->cursor.x = w;
    while (w < e->w)
        screen_change(&e->scr, e->gwin->h + h, w++, color_get(&color_quiet, ' ', CTE_CMDLINE, 0));
}

void editor_draw(editor *e) {
    _editor_draw_msg(e);

    window_draw(e->gwin);

    screen_flush(&e->scr);
    gotoxy(e->cursor.y, e->cursor.x);
}
