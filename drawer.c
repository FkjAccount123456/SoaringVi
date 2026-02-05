#include "drawer.h"
#include "wcwidth/wcwidth.h"

void drawer_init(drawer *dr, screen *scr, textmgr *mgr, size_t y, size_t x,
                 size_t h, size_t w, bool mode) {
    dr->scr = scr;
    dr->mgr = mgr;
    dr->y = y, dr->x = x;
    dr->h = h, dr->full_w = w;
    dr->cfg.mode = mode;
    dr->vscroll = coord_new(0, 0);
    dr->hscroll = 0;
}

void drawer_setcfg(drawer *dr, drawer_config cfg) {
    if (cfg.mode != dr->cfg.mode)
        dr->vscroll.x = 0;
    dr->cfg = cfg;
}

void drawer_resize(drawer *dr, size_t h, size_t w) {
    dr->full_w = w, dr->h = h;
}

char _get_num_vw(size_t num) {
    char res = 0;
    do {
        res++;
    } while (num /= 10);
    return res;
}

coord drawer_setcursor(drawer *dr, size_t y, size_t x) {
    if (dr->cfg.linum) {
        dr->linum_w = _get_num_vw(dr->mgr->text.len);
        if (dr->linum_w < 2)
            dr->linum_w = 2;
        dr->linum_w++;
        dr->w = dr->full_w - dr->linum_w;
    } else {
        dr->linum_w = 0;
        dr->w = dr->full_w;
    }

    if (dr->cfg.mode == DRAWER_MODE_HSCROLL) {
        if (y < dr->vscroll.y + dr->cfg.vscroff) {
            if (y > dr->cfg.vscroff)
                dr->vscroll.y = y - dr->cfg.vscroff;
            else
                dr->vscroll.y = 0;
        }
        if (y + dr->cfg.vscroff + 1 > dr->vscroll.y + dr->h &&
            y + dr->cfg.vscroff + 1 >= dr->h)
            dr->vscroll.y = y + dr->cfg.vscroff + 1 - dr->h;
        size_t w = 0;
        for (size_t i = 0; i < x; i++)
            w += wcwidth(dr->mgr->text.v[y].v[i]);
        if (w < dr->hscroll + dr->cfg.hscroff) {
            if (w > dr->cfg.hscroff)
                dr->hscroll = w - dr->cfg.hscroff;
            else
                dr->hscroll = 0;
        }
        size_t res_x = w;
        if (x < dr->mgr->text.v[y].len)
            w += wcwidth(dr->mgr->text.v[y].v[x]);
        else
            w++;
        if (w + dr->cfg.hscroff > dr->hscroll + dr->w &&
            w + dr->cfg.hscroff >= dr->w)
            dr->hscroll = w + dr->cfg.hscroff - dr->w;
        return coord_new(y - dr->vscroll.y, res_x + dr->linum_w - dr->hscroll);
    }

    coord cxy_vp = drawer_calcvhw(dr, y, x);
    coord crs_vp = coord_new(y, cxy_vp.y - 1);
    coord ubound = drawer_scrollup(dr, crs_vp, dr->cfg.vscroff);
    coord dbound = drawer_scrollup(dr, crs_vp, dr->h - dr->cfg.vscroff - 1);
    log("crs_vp: %zu %zu, scroll: %zu, %zu\n", crs_vp.y, crs_vp.x,
        dr->cfg.vscroff, dr->h - dr->cfg.vscroff - 1);
    log("ubound: %zu %zu\n", ubound.y, ubound.x);
    log("dbound: %zu %zu\n", dbound.y, dbound.x);
    if (coord_cmp(ubound, dr->vscroll) < 0)
        dr->vscroll = ubound;
    if (coord_cmp(dbound, dr->vscroll) > 0)
        dr->vscroll = dbound;

    if (y == dr->vscroll.y)
        return coord_new(cxy_vp.y + dr->y - dr->vscroll.x - 1,
                         cxy_vp.x + dr->x + dr->linum_w);
    size_t cur_vh =
        drawer_calcvhw(dr, dr->vscroll.y, dr_line(dr->vscroll.y).len).y -
        dr->vscroll.x - 1;
    cur_vh += cxy_vp.y;
    for (size_t i = dr->vscroll.y + 1; i < y; i++)
        cur_vh += drawer_calcvhw(dr, i, dr_line(i).len).y;
    if (cxy_vp.x >= dr->w && cur_vh < dr->h - 1) {
        cxy_vp.x = 0;
        cur_vh++;
    }
    cxy_vp.x += dr->x + dr->linum_w;
    cxy_vp.y = dr->y + cur_vh;
    return cxy_vp;
}

void _d_fill_lntext(char *t, size_t w, size_t num) {
    t[w - 1] = ' ';
    t[w] = 0;
    size_t i;
    for (i = w - 2; num; i--) {
        t[i] = num % 10 + '0';
        num /= 10;
    }
    for (; i != -1; i--)
        t[i] = ' ';
}

#define colortext_normal(c)                                                    \
    (colortext){.ch = c, .bg = {0, 0, 0}, .fg = {192, 192, 192}, .style = 0}

char d_linum_text[32];

// 很抱歉，但又是一坨
// 2026-1-23
// 但写的还不错，甚至可以称为逻辑清晰
// 2026-2-2
// 现在还在调试这一坨
void drawer_draw(drawer *dr) {
    if (dr->cfg.mode == DRAWER_MODE_HSCROLL) {
        for (size_t i = 0; i < dr->h; i++) {
            size_t y = dr->vscroll.y + i;
            if (y >= dr->mgr->text.len) {
                for (size_t vx = 0; vx < dr->full_w; vx++)
                    vscreen_change(i, vx, colortext_normal(L' '));
                continue;
            }
            size_t x = 0, w = 0, vx = 0;
            if (dr->linum_w) {
                _d_fill_lntext(d_linum_text, dr->linum_w, y + 1);
                for (; vx < dr->linum_w; vx++)
                    vscreen_change(i, vx, colortext_normal(d_linum_text[vx]));
            }
            char cur_w = 0;
            if (w < dr->hscroll)
                for (; x < dr_line(y).len; x++) {
                    cur_w = wcwidth(dr_line(y).v[x]);
                    if (w + cur_w >= dr->hscroll) {
                        x++;
                        break;
                    }
                    w += cur_w;
                }
            if (x >= dr_line(y).len) {
                for (; vx < dr->full_w; vx++)
                    vscreen_change(i, vx, colortext_normal(L' '));
                continue;
            }
            size_t tgt = w + cur_w;
            w = dr->hscroll;
            for (; w < tgt; vx++, w++)
                vscreen_change(i, vx, colortext_normal(L'<'));
            for (; x < dr_line(y).len; x++) {
                cur_w = wcwidth(dr_line(y).v[x]);
                if (w + cur_w > dr->hscroll + dr->w)
                    break;
                vscreen_change(i, vx, colortext_normal(dr_at(y, x)));
                w += cur_w, vx += cur_w;
            }
            if (x < dr_line(y).len)
                for (; vx < dr->full_w; vx++)
                    vscreen_change(i, vx, colortext_normal(L'>'));
            else
                for (; vx < dr->full_w; vx++)
                    vscreen_change(i, vx, colortext_normal(L' '));
        }
        return;
    }

    // 总算写完横向滚动了，然而折行更难写
    // 有Python版的逻辑可以参考，但不多
    size_t y = dr->vscroll.y, w = 0, h = 0, x = 0, vy = 0;
    if (dr->vscroll.x != 0) {
        for (; x < dr_line(y).len && h < dr->vscroll.x; x++) {
            char cur_w = wcwidth(dr_line(y).v[x]);
            if (w + cur_w > dr->w) {
                h++;
                w = 0;
            }
            w += cur_w;
        }
        x--;
        w = 0;
    }
    while (vy < dr->h) {
        if (y >= dr->mgr->text.len) {
            for (size_t i = 0; i < dr->full_w; i++) {
                vscreen_change(vy, i, colortext_normal(L' '));
            }
            vy++;
            continue;
        }
        if (h == 0 && dr->cfg.linum) {
            _d_fill_lntext(d_linum_text, dr->linum_w, y + 1);
            for (size_t i = 0; i < dr->linum_w; i++)
                vscreen_change(vy, i, colortext_normal(d_linum_text[i]));
        } else if (dr->cfg.linum) {
            for (size_t i = 0; i < dr->linum_w; i++)
                vscreen_change(vy, i, colortext_normal(L' '));
        }
        w = 0;
        for (; x < dr_line(y).len; x++) {
            char cur_w = wcwidth(dr_line(y).v[x]);
            if (w + cur_w > dr->w) {
                h++;
                break;
            }
            vscreen_change(vy, w + dr->linum_w, colortext_normal(dr_at(y, x)));
            w += cur_w;
        }
        while (w + dr->linum_w < dr->full_w) {
            vscreen_change(vy, w + dr->linum_w, colortext_normal(L' '));
            w++;
        }
        if (x >= dr_line(y).len) {
            y++, h = 0, x = 0;
        }
        vy++;
    }
    // 2026-1-23
    // 貌似写完了，竟然就这么几行吗
    // 看来再也没有什么能阻挡我了（
}

// 2026-1-25
// 666忘写这三个辅助函数了
coord drawer_calcvhw(drawer *dr, size_t y, size_t x) {
    size_t h = 1, w = 0;
    for (size_t i = 0; i < x; i++) {
        char cur_w = wcwidth(dr_at(y, i));
        if (w + cur_w > dr->w) {
            w = 0;
            h++;
        }
        w += cur_w;
    }
    return coord_new(h, w);
}

coord drawer_scrollup(drawer *dr, coord pos, long n) {
    if (n < 0)
        return drawer_scrolldown(dr, pos, -n);
    while (n) {
        if (n < pos.x) {
            pos.x -= n;
            return pos;
        } else if (pos.y) {
            n -= pos.x + 1;
            pos.y--;
            pos.x = drawer_calcvhw(dr, pos.y, dr_line(pos.y).len).y - 1;
        } else {
            return coord_new(0, 0);
        }
    }
    return pos;
}

coord drawer_scrolldown(drawer *dr, coord pos, long n) {
    if (n < 0)
        return drawer_scrollup(dr, pos, -n);
    while (n) {
        size_t cur_h = drawer_calcvhw(dr, pos.y, dr_line(pos.y).len).y;
        if (n + pos.x < cur_h) {
            pos.x += n;
            return pos;
        } else if (pos.y < dr->mgr->text.len - 1) {
            pos.y++;
            pos.x = 0;
            n -= cur_h - pos.x;
        } else {
            return coord_new(pos.y, cur_h - 1);
        }
    }
    return pos;
}
