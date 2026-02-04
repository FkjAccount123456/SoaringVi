#include "textmgr.h"
#include <memory.h>
#include <stdlib.h>

int coord_cmp(coord a, coord b) {
    return a.y < b.y ? -1 : a.y > b.y ? 1 : a.x < b.x ? -1 : a.x > b.x ? 1 : 0;
}

void text_init(textmgr *mgr) {
    mgr->text = seq_init(str_list);
    seq_append(mgr->text, seq_init(rawstr));
}

void text_free(textmgr *mgr) {
    for (size_t i = 0; i < mgr->text.len; i++)
        free(mgr->text.v[i].v);
    free(mgr->text.v);
}

// 很好，感觉比Python版还要干脆利落
coord text_insert(textmgr *mgr, coord pos, rawstr str) {
    if (!str.len)
        return pos;
    coord old_pos = pos;
    str_list data = seq_init(str_list);
    rawstr tmp = seq_init(rawstr);
    for (size_t i = 0; i < str.len; i++) {
        if (str.v[i] == '\n') {
            seq_append(data, tmp);
            tmp = seq_init(rawstr);
            pos.y++;
            pos.x = 0;
        } else if (str.v[i] != '\r') {
            seq_append(tmp, str.v[i]);
        }
    }
    seq_append(data, tmp);
    return text_insert_processed(mgr, old_pos, data);
}

coord text_insert_processed(textmgr *mgr, coord pos, str_list data) {
    if (data.len == 1) {
        size_t len = data.v[0].len;
        seq_insert(text_line(pos.y), pos.x, data.v[0].v, len);
        free(data.v[0].v);
        free(data.v);
        return coord_new(pos.y, pos.x + len);
    }
    seq_insert(mgr->text, pos.y + 1, data.v + 1, data.len - 1);
    seq_insert(text_line(pos.y + data.len - 1), 0, &text_at(pos.y, pos.x),
               text_line(pos.y).len - pos.x);
    text_line(pos.y).len = pos.x;
    seq_extend(text_line(pos.y), data.v[0].v, data.v[0].len);
    free(data.v[0].v);
    free(data.v);
    return coord_new(pos.y + data.len - 1, seq_end(data).len);
}

// 2026-1-16
// 果然，简单了好多
// 竟然比Python版还短
coord text_delete(textmgr *mgr, coord l, coord r) {
    if (coord_cmp(l, r) == 0)
        return l;
    if (coord_cmp(l, r) > 0)
        swap(l, r);
    if (l.y == r.y) {
        memmove(&text_at(l.y, l.x), &text_at(l.y, r.x),
                (text_line(l.y).len - r.x) * sizeof(byte));
        text_line(l.y).len -= r.x - l.x;
        return l;
    }
    seq_expand_to(text_line(l.y), l.x + text_line(r.y).len - r.x);
    memcpy(&text_at(l.y, l.x), &text_at(r.y, r.x),
           (text_line(r.y).len - r.x) * sizeof(byte));
    text_line(l.y).len = l.x + text_line(r.y).len - r.x;
    for (size_t i = l.y + 1; i <= r.y; i++)
        free(text_line(i).v);
    memmove(&text_line(l.y + 1), &text_line(r.y + 1),
            sizeof(rawstr) * (mgr->text.len - r.y - 1));
    mgr->text.len -= r.y - l.y;
    return l;
}

rawstr text_get(textmgr *mgr, coord l, coord r) {
    if (coord_cmp(l, r) > 0)
        swap(l, r);
    rawstr res = seq_init(rawstr);
    if (l.y == r.y) {
        seq_extend(res, &text_at(l.y, l.x), r.x - l.x);
        return res;
    }
    seq_extend(res, &text_at(l.y, l.x), text_line(l.y).len - l.x);
    seq_append(res, '\n');
    for (size_t i = l.y + 1; i < r.y; i++) {
        seq_extend(res, text_line(i).v, text_line(i).len);
        seq_append(res, '\n');
    }
    seq_extend(res, text_line(r.y).v, r.x);
    return res;
}

#ifdef DEBUG_MODE

void text_log(textmgr *mgr) {
    log("text_log:\n");
    for (size_t i = 0; i < mgr->text.len; i++) {
        for (size_t j = 0; j < mgr->text.v[i].len; j++) {
            log("%lc", mgr->text.v[i].v[j]);
        }
        log(";%zu\n", mgr->text.v[i].len);
    }
    log("end;\n");
}

#endif // DEBUG_MODE
