#include "textmgr.h"
#include <memory.h>
#include <stdlib.h>
#include <time.h>

int coord_cmp(coord a, coord b) {
    return a.y < b.y ? -1 : a.y > b.y ? 1 : a.x < b.x ? -1 : a.x > b.x ? 1 : 0;
}

void text_init(textmgr *mgr) {
    mgr->text = seq_init(str_list);
    seq_append(mgr->text, seq_init(rawstr));

    undo_node_init(&mgr->undo_root, history_nop(), NULL);
    mgr->undo_cur = &mgr->undo_root;
}

void text_free(textmgr *mgr) {
    for (size_t i = 0; i < mgr->text.len; i++)
        free(mgr->text.v[i].v);
    free(mgr->text.v);
}

void text_add_history(textmgr *mgr, edit_history op, bool pass_ownership) {
    log("text_add_history pass_ownership: %d\n", pass_ownership);
    time_t t;
    time(&t);
    if (mgr->undo_cur->next.len == 0 && t - mgr->undo_cur->time <= 1) {
        if (mgr->undo_cur->op.tp == OPERT_INSERT && op.tp == OPERT_INSERT &&
            op.v.len == 1 && !coord_cmp(op.l, mgr->undo_cur->op.r) &&
            op.v.v[0] != '\n') {
            seq_append(mgr->undo_cur->op.v, op.v.v[0]);
            mgr->undo_cur->op.r.x += 1;
            mgr->undo_cur->time = t;
            if (pass_ownership)
                free(op.v.v);
            return;
        }
        if (mgr->undo_cur->op.tp == OPERT_INSERT && op.tp == OPERT_DELETE &&
            op.v.len == 1 && !coord_cmp(op.r, mgr->undo_cur->op.r) &&
            op.v.v[0] != '\n') {
            mgr->undo_cur->op.r.x -= 1;
            mgr->undo_cur->op.v.len--;
            mgr->undo_cur->time = t;
            if (pass_ownership)
                free(op.v.v);
            return;
        }
        if (mgr->undo_cur->op.tp == OPERT_DELETE && op.tp == OPERT_DELETE &&
            op.v.len == 1 && !coord_cmp(op.r, mgr->undo_cur->op.l) &&
            op.v.v[0] != '\n') {
            mgr->undo_cur->op.l.x -= 1;
            seq_insert(mgr->undo_cur->op.v, 0, op.v.v, 1);
            mgr->undo_cur->time = t;
            if (pass_ownership)
                free(op.v.v);
            return;
        }
    }
    if (!pass_ownership) {
        byte *src = op.v.v;
        op.v.v = malloc(op.v.max * sizeof(byte));
        memcpy(op.v.v, src, op.v.len * sizeof(byte));
    }
    seq_expand(mgr->undo_cur->next, 1);
    mgr->undo_cur->next.len++;
    undo_node_init(&seq_end(mgr->undo_cur->next), op, mgr->undo_cur);
    mgr->undo_cur = &seq_end(mgr->undo_cur->next);
    mgr->undo_cur->time = t;
}

coord text_undo(textmgr *mgr) {
    if (mgr->undo_cur == &mgr->undo_root)
        return coord_new(0, 0);
    mgr->is_doing = true;
    edit_history op = mgr->undo_cur->op;
    mgr->undo_cur = mgr->undo_cur->prev;
    coord res;
    if (op.tp == OPERT_INSERT) {
        text_delete(mgr, op.l, op.r);
        res = op.l;
    } else if (op.tp == OPERT_DELETE) {
        text_insert(mgr, op.l, op.v);
        res = op.r;
    }
    mgr->is_doing = false;
    return res;
}

coord text_redo(textmgr *mgr, size_t branch) {
    if (!mgr->undo_cur->next.len)
        return coord_new(-1, -1);
    if (branch == -1)
        branch = mgr->undo_cur->next.len - 1;
    mgr->is_doing = true;
    mgr->undo_cur = &mgr->undo_cur->next.v[branch];
    edit_history op = mgr->undo_cur->op;
    coord res;
    if (op.tp == OPERT_INSERT) {
        text_insert(mgr, op.l, op.v);
        res = op.r;
    } else if (op.tp == OPERT_DELETE) {
        text_delete(mgr, op.l, op.r);
        res = op.l;
    }
    mgr->is_doing = false;
    return res;
}

// 很好，感觉比Python版还要干脆利落
coord text_insert(textmgr *mgr, coord pos, rawstr str) {
    if (!str.len)
        return pos;
    coord l = pos;
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
    coord r = text_insert_processed(mgr, l, data);
    if (!mgr->is_doing)
        text_add_history(mgr, history_new(OPERT_INSERT, l, r, str), false);
    return r;
}

// 不负责添加history
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
    rawstr str = text_get(mgr, l, r);
    if (l.y == r.y) {
        if (!mgr->is_doing) {
            rawstr tmp = {.len = r.x - l.x, .v = &text_at(l.y, l.x)};
            text_add_history(mgr, history_new(OPERT_DELETE, l, r, tmp), false);
        }
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
    if (!mgr->is_doing)
        text_add_history(mgr, history_new(OPERT_DELETE, l, r, str), true);
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
