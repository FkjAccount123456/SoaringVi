/*
怎么说呢，别的靠着逻辑优化行数都可以大大减少，但这drawer是真不行
基本上2025版就是极限了，后面都是小的缝缝补补
这次不但要支持旧有的折行还要新增横向滚动，代码量估计得翻倍

2026-1-19
不过之前有大坨的答辩，借着重写的机会也许能清理一新
*/
#ifndef DRAWER_H
#define DRAWER_H

#include "screen.h"
#include "textmgr.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>

#define DRAWER_MODE_LINWRAP 0
#define DRAWER_MODE_HSCROLL 1

typedef struct drawer_config {
    bool mode;
    bool linum;
    size_t vscroff, hscroff;
} drawer_config;

typedef struct drawer {
    screen *scr;
    textmgr *mgr;
    size_t y, x;
    size_t full_w, h, w, linum_w;
    drawer_config cfg;
    coord vscroll;
    size_t hscroll;
} drawer;

#define dr_line(y) dr->mgr->text.v[y]
#define dr_at(y, x) dr->mgr->text.v[y].v[x]

#define vscreen_change(Y, X, ch)                                               \
    screen_change(dr->scr, (Y) + dr->y, (X) + dr->x, ch)

// 和Buffer一起分配
void drawer_init(drawer *dr, screen *scr, textmgr *mgr, size_t y, size_t x,
                 size_t h, size_t w, bool mode);

void drawer_setcfg(drawer *dr, drawer_config cfg);
void drawer_resize(drawer *dr, size_t h, size_t w);
// 根据光标位置调整滚动，并返回光标位置所在的视觉行
coord drawer_setcursor(drawer *dr, size_t y, size_t x);
void drawer_draw(drawer *dr);

// 行数从1开始，mode为HSCROLL时不应调用这三个函数
coord drawer_calcvhw(drawer *dr, size_t y, size_t x);
// 如果到头了就直接返回
coord drawer_scrollup(drawer *dr, coord pos, long n);
coord drawer_scrolldown(drawer *dr, coord pos, long n);

#endif // DRAWER_H
