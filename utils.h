#ifndef UTILS_H
#define UTILS_H

#include "debug.h"
#include "hashmap.h"
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define error(...)                                                             \
    do {                                                                       \
        fprintf(stderr, ##__VA_ARGS__);                                        \
        exit(1);                                                               \
    } while (0)

#define seq(T)                                                                 \
    {                                                                          \
        T *v;                                                                  \
        size_t len, max, Tsize;                                                \
    }

#define seq_init(T)                                                            \
    ({                                                                         \
        T name;                                                                \
        name.len = 0, name.max = 8, name.Tsize = sizeof(name.v[0]);            \
        name.v = malloc(name.Tsize * 8);                                       \
        name;                                                                  \
    })

#define seq_init_reserved(T, reserved)                                         \
    ({                                                                         \
        T name;                                                                \
        name.len = 0, name.max = reserved, name.Tsize = sizeof(name.v[0]);     \
        name.v = malloc(name.Tsize * (reserved));                              \
        name;                                                                  \
    })

#define seq_append(name, val)                                                  \
    do {                                                                       \
        if (name.len == name.max) {                                            \
            name.max <<= 1;                                                    \
            name.v = realloc(name.v, name.Tsize * name.max);                   \
        }                                                                      \
        name.v[name.len++] = val;                                              \
    } while (0)

#define seq_expand_to(name, _n)                                                \
    do {                                                                       \
        typeof(_n) n = _n;                                                     \
        if (name.max < n) {                                                    \
            while (name.max < n)                                               \
                name.max <<= 1;                                                \
            name.v = realloc(name.v, name.Tsize * name.max);                   \
        }                                                                      \
    } while (0)

#define seq_expand(name, _n)                                                   \
    do {                                                                       \
        typeof(_n) n = _n;                                                     \
        if (name.len + n > name.max) {                                         \
            while (name.len + n > name.max)                                    \
                name.max <<= 1;                                                \
            name.v = realloc(name.v, name.Tsize * name.max);                   \
        }                                                                      \
    } while (0)

#define seq_insert(name, _idx, src, _n)                                        \
    do {                                                                       \
        typeof(_idx) idx = _idx;                                               \
        typeof(_n) n = _n;                                                     \
        if (name.len + n > name.max) {                                         \
            while (name.len + n > name.max)                                    \
                name.max <<= 1;                                                \
            name.v = realloc(name.v, name.Tsize * name.max);                   \
        }                                                                      \
        memmove(name.v + idx + n, name.v + idx,                                \
                (name.len - idx) * name.Tsize);                                \
        memcpy(name.v + idx, src, n * name.Tsize);                             \
        name.len += n;                                                         \
    } while (0)

#define seq_extend(name, src, _n)                                              \
    do {                                                                       \
        typeof(_n) n = _n;                                                     \
        if (name.len + n > name.max) {                                         \
            while (name.len + n > name.max)                                    \
                name.max <<= 1;                                                \
            name.v = realloc(name.v, name.Tsize * name.max);                   \
        }                                                                      \
        memcpy(name.v + name.len, src, n * name.Tsize);                        \
        name.len += n;                                                         \
    } while (0)

#define seq_pop(name) (name.v[--name.len])

#define seq_free(name) (free(name.v))

#define seq_end(name) (name.v[name.len - 1])

#define seq_back(name, n) (name.v[name.len - n])

#define swap(a, b)                                                             \
    do {                                                                       \
        typeof(a) tmp = a;                                                     \
        a = b;                                                                 \
        b = tmp;                                                               \
    } while (0)

// 懒，直接用unicode吧，UTF-8太费劲
typedef wchar_t byte;

// SoaringVi的字符串不使用0结尾，而是指定长度
// 显然，我将在C语言中使用神似Python的代码风格
typedef struct seq(byte) rawstr;
typedef struct seq(rawstr) str_list;

#define U_OBUF_SIZE 65536

void u_init();
void u_fina();

// 从0开始
void gotoxy(size_t y, size_t x);

#define flush() fflush(stdout)

#define STYLE_BOLD 1
#define STYLE_ITALIC 2
#define STYLE_UNDERLINE 4

typedef struct colortext {
    unsigned char bg[3], fg[3];
    char style;
    byte ch;
} colortext;

bool cotext_eq(colortext a, colortext b);
bool color_eq(colortext a, colortext b);

void cotext_print(colortext c);

typedef struct coord {
    size_t y, x;
} coord;

#define coord_new(y, x) ((coord){y, x})

void wstrcpy(byte *dst, byte *src);

void u_init_ch2keymap();

unsigned char u_basic_getch();

bool u_kbhit();
// 是正常输入用返回值返回，否则用指针返回
byte u_getch(char **res);

#define wait_until(ev) while (!(ev))

char utf8_next(unsigned char ch);
char utf8_cvt(const unsigned char *code, byte *output);

#define get_time(expr)                                                         \
    ({                                                                         \
        size_t start = get_usec();                                             \
        typeof(expr) res = expr;                                               \
        size_t end = get_usec();                                               \
        log("time: %zu, ret: %lld\n", end - start, (long long)res);            \
        res;                                                                   \
    })

// 依稀记得2024年早期的betterlang object实现就是字典树
typedef struct trie {
    struct trie *child[256];
    bool is_leaf;
    void *data;
} trie;

#define trie_init(t) memset(t, 0, sizeof(trie))
void trie_insert(trie *t, unsigned char *key, void *data);
void **trie_get(trie *t, unsigned char *key);
void trie_free(trie *t);

#endif // UTILS_H
