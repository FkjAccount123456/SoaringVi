#include "editor.h"

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

void _window_replaceself(window *w, window *new);
void _split_relay(split *sp);

void buffer_quit(buffer *buf, bool force) {
    if (buf->parent) {
        for (size_t i = 0; i < buf->e->bufs.len; i++)
            if (buf->e->bufs.v[i] == buf)
                seq_remove(buf->e->bufs, i);
        if (buf->parent->chs.len == 2) {
            window *other = buf->parent->chs.v[0].win == (window *)buf ? buf->parent->chs.v[1].win : buf->parent->chs.v[0].win;
            _window_replaceself((window *)buf->parent, other);
            other->parent = other->parent->parent;
            window_moveresize(other, buf->parent->t, buf->parent->l, buf->parent->h, buf->parent->w);
            for (size_t i = 0; i < buf->e->bufs.len; i++)
                if (buf->e->sps.v[i] == buf->parent)
                    seq_remove(buf->e->sps, i);
            split_free(buf->parent);
            if (buf == buf->e->cur)
                buf->e->cur = window_find_back(other, buf->t);
        } else {
            size_t id;
            double ratio;
            for (size_t i = 0; i < buf->parent->chs.len; i++)
                if (buf->parent->chs.v[i].win == (window *)buf) {
                    id = i;
                    ratio = 1.0f - buf->parent->chs.v[i].ratio;
                    seq_remove(buf->parent->chs, id);
                    break;
                }
            double sum = 0;
            for (size_t i = 0; i < buf->parent->chs.len - 1; i++)
                buf->parent->chs.v[i].ratio /= ratio, sum += buf->parent->chs.v[i].ratio;
            seq_end(buf->parent->chs).ratio = 1.0f - sum;
            _split_relay(buf->parent);
            if (id >= buf->parent->chs.len)
                id = buf->parent->chs.len - 1;
            if (buf == buf->e->cur)
                buf->e->cur = window_find_back(buf->parent->chs.v[id].win, buf->t);
            buffer_free(buf);
            free(buf);
        }
    } else {
        editor_quit(buf->e, force);
    }
}

void split_move(split *sp, int t, int l) {
    long long dt = t - sp->t, dl = l - sp->l;
    for (int i = 0; i < sp->chs.len; i++)
        window_move(sp->chs.v[i].win, sp->chs.v[i].win->t + dt, sp->chs.v[i].win->l + dl);
    sp->t = t, sp->l = l;
}

void split_resize(split *sp, int h, int w) {
    split_moveresize(sp, sp->t, sp->l, h, w);
}

// 怎么就没想到根据大小算比例呢
void _split_adjust_ratio(split *sp) {
    if (sp->is_vsp) {
        for (int i = 0; i < sp->chs.len; i++)
            sp->chs.v[i].ratio = (sp->chs.v[i].win->w + (i != sp->chs.len - 1)) / (double)sp->w;
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
            postsum[i - 1] = postsum[i] + window_calc_minsize(sp->chs.v[i - 1].win).y;
        sum_size = sp->h;
        for (int i = 0; i < sp->chs.len - 1; i++) {
            window *ch = sp->chs.v[i].win;
            int min_size = window_calc_minsize(ch).y;
            int max_size = sum_size - postsum[i + 1];
            max_size = max(0, max_size);
            int new_size = min(max_size, max(min_size, (int)(sp->h * sp->chs.v[i].ratio)));
            sum_size -= new_size;
            window_moveresize(ch, sp->t + pos, sp->l, new_size, sp->w);
            pos += new_size;
        }
        window_moveresize(seq_end(sp->chs).win, sp->t + pos, sp->l, sum_size, sp->w);
    } else {
        postsum[sp->chs.len] = -1;
        for (int i = sp->chs.len; i > 0; i--)
            postsum[i - 1] = postsum[i] + window_calc_minsize(sp->chs.v[i - 1].win).x + 1;
        sum_size = sp->w;
        for (int i = 0; i < sp->chs.len - 1; i++) {
            window *ch = sp->chs.v[i].win;
            int min_size = window_calc_minsize(ch).x;
            int max_size = sum_size - postsum[i + 1] - 1;
            max_size = max(0, max_size);
            int new_size = min(max_size, max(min_size, (int)(sp->w * sp->chs.v[i].ratio)));
            sum_size -= new_size + 1;
            window_moveresize(ch, sp->t, sp->l + pos, sp->h, new_size);
            pos += new_size + 1;
        }
        window_moveresize(seq_end(sp->chs).win, sp->t, sp->l + pos, sp->h, sum_size);
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
        int lt_minsize = e_sizesum(0, id, minsizes[i]), rb_minsize = e_sizesum(id + 1, sp->chs.len, minsizes[i]);
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
        int lt_minsize = e_sizesum(0, id, minsizes[i] + 1), rb_minsize = e_sizesum(id + 1, sp->chs.len, minsizes[i] + 1);
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
#define gen_window_find(d, drev, d_is_vsp, wfinal, d_op)             \
    buffer *window_find_##d(window *w, int y) {                      \
        if (!w->parent)                                              \
            return NULL;                                             \
        if (w->parent->is_vsp != d_is_vsp)                           \
            return window_find(d, w->parent, y);                     \
        int idx = _window_findselfindex(w);                          \
        if (idx == -1)                                               \
            return NULL;                                             \
        if (idx == (wfinal))                                         \
            return window_find(d, w->parent, y);                     \
        return window_find(drev, w->parent->chs.v[idx d_op].win, y); \
    }

gen_window_find(right, front, true, w->parent->chs.len - 1, +1);
gen_window_find(left, back, true, 0, -1);
gen_window_find(down, top, false, w->parent->chs.len - 1, +1);
gen_window_find(up, bottom, false, 0, -1);

#undef gen_window_find

#define gen_window_find_in(d, d_is_vsp, wfinal, attr)          \
    buffer *window_find_##d(window *_w, int p) {               \
        if (_w->is_buf)                                        \
            return (buffer *)_w;                               \
        split *w = (split *)_w;                                \
        if (w->is_vsp != d_is_vsp) {                           \
            for (int i = 0; i < w->chs.len - 1; i++)           \
                if (w->chs.v[i + 1].win->attr > p)             \
                    return window_find(d, w->chs.v[i].win, p); \
            return window_find(d, seq_end(w->chs).win, p);     \
        }                                                      \
        return window_find(d, w->chs.v[wfinal].win, p);        \
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
        minsize = e_sizesum(0, p->chs.len, window_calc_minsize(p->chs.v[i].win).y);
        minsize += h - s_minsize.y;
        if (p->h < minsize)
            window_resize_bottomup((window *)p, minsize, w);
        else if (p->w != w)
            window_resize_bottomup((window *)p, p->h, w);
        split_resize_child(p, _window_findselfindex(s), h);
    } else {
        minsize = e_sizesum(0, p->chs.len, window_calc_minsize(p->chs.v[i].win).x + 1);
        minsize += w - s_minsize.x - 1;
        if (p->w < minsize)
            window_resize_bottomup((window *)p, h, minsize);
        else if (p->h != h)
            window_resize_bottomup((window *)p, h, p->w);
        split_resize_child(p, _window_findselfindex(s), w);
    }
}
