#ifndef TEXTMGR_H
#define TEXTMGR_H

#include "utils.h"

#define OPERT_NOP 0
#define OPERT_INSERT 1
#define OPERT_DELETE 2

// 以l、r的顺序性来确定操作的类型，这样就统一了插入和删除的操作
// 不对不行，这样不好搞
typedef struct edit_history {
    unsigned char tp;
    coord l, r;
    rawstr v;
} edit_history;

#define history_nop()                                                          \
    (edit_history) {                                                           \
        .tp = OPERT_NOP                                                        \
    }

#define history_new(_tp, _l, _r, _v)                                           \
    (edit_history) {                                                           \
        .tp = _tp, .l = _l, .r = _r, .v = _v                                   \
    }

typedef struct undo_node undo_node;
typedef struct seq(undo_node) undo_list;

typedef struct undo_node {
    edit_history op;
    undo_node *prev;
    undo_list next;
    size_t time;
} undo_node;

#define undo_node_init(node, _op, _prev)                                       \
    (node)->op = _op, (node)->prev = _prev,                                    \
    (node)->next = seq_init_reserved(undo_list, 1)

// 存储时最后必须另外补充一个空行
// 2026-2-4
// 为什么要留这个空行来着？
// 好像没啥用，那就不要了吧
typedef struct textmgr {
    str_list text;
    undo_node undo_root;
    undo_node *undo_cur;
    bool is_doing;
} textmgr;

int coord_cmp(coord a, coord b);

void text_init(textmgr *mgr);
void text_free(textmgr *mgr);

void text_add_history(textmgr *mgr, edit_history op, bool pass_ownership);
coord text_undo(textmgr *mgr);
coord text_redo(textmgr *mgr, size_t branch);

coord text_insert(textmgr *mgr, coord pos, rawstr str);
// data在此处销毁
coord text_insert_processed(textmgr *mgr, coord pos, str_list data);
// 2026-1-15
// textmgr用左开右闭区间，相比Python版的全闭实现逻辑更简单一点
coord text_delete(textmgr *mgr, coord l, coord r);
rawstr text_get(textmgr *mgr, coord l, coord r);

#define text_line(y) mgr->text.v[y]
#define text_at(y, x) mgr->text.v[y].v[x]

#ifdef DEBUG_MODE
void text_log(textmgr *mgr);
#else // DEBUG_MODE
#define text_log(...)
#endif // DEBUG_MODE

#endif // TEXTMGR_H
