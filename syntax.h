#ifndef SYNTAX_H
#define SYNTAX_H

#include "colorscheme.h"
#include "utils.h"

typedef struct textmgr textmgr;

typedef struct seq(colortype_t) ct_list;
typedef struct seq(ct_list) ct_list2;

typedef struct renderer {
    textmgr *text;
    char *file;
    ct_list2 data;
} renderer;

void renderer_init(renderer *z, textmgr *text, char *file);
void renderer_open(renderer *z, char *file, str_list data);
void renderer_setname(renderer *z, char *file);
void renderer_free(renderer *z);

void renderer_insert(renderer *z, coord pos, str_list data);
void renderer_delete(renderer *z, coord l, coord r);

colortype_t renderer_get(renderer *z, size_t y, size_t x);

#define rd_line(y) z->data.v[y]
#define rd_at(y, x) z->data.v[y].v[x]

#endif // SYNTAX_H
