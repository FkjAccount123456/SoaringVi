#include "syntax.h"

void renderer_init(renderer *z, textmgr *text, char *file) {
    z->text = text;
    z->file = file;
    z->data = seq_init(ct_list2);
    seq_append(z->data, seq_init(ct_list));
}

void renderer_free(renderer *z) {
    for (size_t i = 0; i < z->data.len; i++)
        free(z->data.v[i].v);
    free(z->data.v);
}

void renderer_insert(renderer *z, coord pos, str_list data) {
    if (data.len == 1) {
        size_t len = data.v[0].len;
        seq_insert_nofill(rd_line(pos.y), pos.x, len);
        memset(rd_line(pos.y).v + pos.x, CT_TEXT, len);
        return;
    }
    seq_insert_nofill(z->data, pos.y + 1, data.len - 1);
    for (size_t i = 1; i < data.len; i++) {
        rd_line(pos.y + i) = seq_init_reserved(ct_list, data.v[i].len);
        memset(rd_line(pos.y + i).v, CT_TEXT, data.v[i].len);
    }
    seq_extend(rd_line(pos.y + data.len - 1), &rd_at(pos.y, pos.x), rd_line(pos.y).len - pos.x);
    rd_line(pos.y).len = pos.x;
    seq_expand(rd_line(pos.y), data.v[0].len);
    memset(rd_line(pos.y).v + pos.x, CT_TEXT, data.v[0].len);
    rd_line(pos.y).len += data.v[0].len;
}

void renderer_delete(renderer *z, coord l, coord r) {
    if (coord_cmp(l, r) > 0)
        swap(l, r);
    if (l.y == r.y) {
        memmove(&rd_at(l.y, l.x), &rd_at(l.y, r.x), (rd_line(l.y).len - r.x));
        rd_line(l.y).len -= r.x - l.x;
        return;
    }
    seq_expand_to(rd_line(l.y), l.x + rd_line(r.y).len - r.x);
    memcpy(&rd_at(l.y, l.x), &rd_at(r.y, r.x), (rd_line(r.y).len - r.x));
    rd_line(l.y).len = l.x + rd_line(r.y).len - r.x;
    for (size_t i = l.y + 1; i <= r.y; i++)
        free(rd_line(i).v);
    memmove(&rd_line(l.y + 1), &rd_line(r.y + 1), sizeof(rawstr) * (z->data.len - r.y - 1));
    z->data.len -= r.y - l.y;
}

void renderer_open(renderer *z, char *file, str_list data) {
    renderer_setname(z, file);
    renderer_free(z);
    z->data = seq_init_reserved(ct_list2, data.len);
    z->data.len = data.len;
    for (size_t i = 0; i < data.len; i++) {
        z->data.v[i] = seq_init_reserved(ct_list, data.v[i].len);
        z->data.v[i].len = data.v[i].len;
        memset(z->data.v[i].v, CT_TEXT, data.v[i].len);
    }
}

void renderer_setname(renderer *z, char *file) {
    z->file = file;
}

colortype_t renderer_get(renderer *z, size_t y, size_t x) {
    return CT_TEXT;
}
