#ifndef UTILS_H
#define UTILS_H

#include "debug.h"
#include "hashmap.h"
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
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
byte u_getch();

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

enum {
    K_ESC = 27,
    K_UP = -1,
    K_DOWN = -2,
    K_LEFT = -3,
    K_RIGHT = -4,
    K_HOME = -5,
    K_END = -6,
    K_C_UP = -7,
    K_C_DOWN = -8,
    K_C_LEFT = -9,
    K_C_RIGHT = -10,
    K_C_HOME = -11,
    K_C_END = -12,
    K_M_UP = -13,
    K_M_DOWN = -14,
    K_M_LEFT = -15,
    K_M_RIGHT = -16,
    K_M_HOME = -17,
    K_M_END = -18,
    K_S_UP = -19,
    K_S_DOWN = -20,
    K_S_LEFT = -21,
    K_S_RIGHT = -22,
    K_S_HOME = -23,
    K_S_END = -24,
    K_F1 = -25,
    K_F2 = -26,
    K_F3 = -27,
    K_F4 = -28,
    K_F5 = -29,
    K_F6 = -30,
    K_F7 = -31,
    K_F8 = -32,
    K_F9 = -33,
    K_F10 = -34,
    K_F11 = -35,
    K_F12 = -36,
    K_C_F1 = -37,
    K_C_F2 = -38,
    K_C_F3 = -39,
    K_C_F4 = -40,
    K_C_F5 = -41,
    K_C_F6 = -42,
    K_C_F7 = -43,
    K_C_F8 = -44,
    K_C_F9 = -45,
    K_C_F10 = -46,
    K_C_F11 = -47,
    K_C_F12 = -48,
    K_M_F1 = -49,
    K_M_F2 = -50,
    K_M_F3 = -51,
    K_M_F4 = -52,
    K_M_F5 = -53,
    K_M_F6 = -54,
    K_M_F7 = -55,
    K_M_F8 = -56,
    K_M_F9 = -57,
    K_M_F10 = -58,
    K_M_F11 = -59,
    K_M_F12 = -60,
    K_INS = -61,
    K_DEL = -62,
    K_C_INS = -63,
    K_C_DEL = -64,
    K_M_INS = -65,
    K_M_DEL = -66,
    K_PAGEUP = -67,
    K_PAGEDOWN = -68,
    K_C_PAGEUP = -69,
    K_C_PAGEDOWN = -70,
    K_M_PAGEUP = -71,
    K_M_PAGEDOWN = -72,

    K_TAB = '\t',
    K_CR = '\n',
    K_SPACE = ' ',
    K_BS = '\x7f',
    K_C_RSQBR = '\x1d',
    K_C_BACKSLASH = '\x1c',
    K_C_SLASH = '\x1f',

    K_M_TAB = -73,
    K_M_CR = -74,
    K_M_SPACE = -75,
    K_M_BS = -76,
    K_M_C_RSQBR = -77,
    K_M_C_BACKSLASH = -78,
    K_M_C_SLASH = -79,
    K_M_ESC = -80,

    K_UNKNOWN = -81,

    K_M_C_A = -200,
};

#define K_CTRL(ch) (ch - 'a')
#define K_M_CTRL(ch) (ch - 'a' + K_M_C_A)

#define convert(T, V)                                                          \
    ((union {                                                                  \
         typeof(V) v;                                                          \
         T t;                                                                  \
     }){.v = V}                                                                \
         .t)

#define isprintable(ch) (isprint(ch) || ch == '\t' || ch == '\n' || ch == ' ')

#endif // UTILS_H
