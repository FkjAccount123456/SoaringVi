#ifndef UTILS_H
#define UTILS_H

#include "debug.h"
#include "hashmap.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <wchar.h>

#define error(...)                      \
    do {                                \
        fprintf(stderr, ##__VA_ARGS__); \
        exit(1);                        \
    } while (0)

#define seq(T)                  \
    {                           \
        T *v;                   \
        size_t len, max, Tsize; \
    }

#define seq_init_null(T)                                                        \
    ({                                                                          \
        T name;                                                                 \
        name.v = NULL, name.max = name.len = 0, name.Tsize = sizeof(name.v[0]); \
        name;                                                                   \
    })

#define seq_init(T)                                                 \
    ({                                                              \
        T name;                                                     \
        name.len = 0, name.max = 8, name.Tsize = sizeof(name.v[0]); \
        name.v = malloc(name.Tsize * 8);                            \
        name;                                                       \
    })

#define seq_init_reserved(T, reserved)                                     \
    ({                                                                     \
        T name;                                                            \
        name.len = 0, name.max = reserved, name.Tsize = sizeof(name.v[0]); \
        if (!name.max)                                                     \
            name.max = 1;                                                  \
        name.v = malloc(name.Tsize * name.max);                            \
        name;                                                              \
    })

#define seq_from_slice(T, _dest, _n)                  \
    ({                                                \
        typeof(_n) _S_n = _n;                         \
        T name;                                       \
        if (_S_n) {                                   \
            name = seq_init_reserved(T, _S_n);        \
            memcpy(name.v, _dest, _S_n * name.Tsize); \
            name.len = _S_n;                          \
        } else {                                      \
            name = seq_init_null(T);                  \
        }                                             \
        name;                                         \
    })

#define seq_append(name, val)                                \
    do {                                                     \
        if (name.len == name.max) {                          \
            name.max <<= 1;                                  \
            name.v = realloc(name.v, name.Tsize * name.max); \
        }                                                    \
        name.v[name.len++] = val;                            \
    } while (0)

#define seq_expand_to(name, _n)                              \
    do {                                                     \
        typeof(_n) _S_n = _n;                                \
        if (name.max < _S_n) {                               \
            while (name.max < _S_n)                          \
                name.max <<= 1;                              \
            name.v = realloc(name.v, name.Tsize * name.max); \
        }                                                    \
    } while (0)

#define seq_expand(name, _n)                                 \
    do {                                                     \
        typeof(_n) _S_n = _n;                                \
        if (name.len + _S_n > name.max) {                    \
            while (name.len + _S_n > name.max)               \
                name.max <<= 1;                              \
            name.v = realloc(name.v, name.Tsize * name.max); \
        }                                                    \
    } while (0)

// 2026-6-7
// 竟然又有问题出在这个宏上了
// 只是idx和外部变量重名了而已
#define seq_insert(name, _idx, src, _n)                                                     \
    do {                                                                                    \
        typeof(_idx) _S_idx = _idx;                                                         \
        typeof(_n) _S_n = _n;                                                               \
        if (name.len + _S_n > name.max) {                                                   \
            while (name.len + _S_n > name.max)                                              \
                name.max <<= 1;                                                             \
            name.v = realloc(name.v, name.Tsize * name.max);                                \
        }                                                                                   \
        memmove(name.v + _S_idx + _S_n, name.v + _S_idx, (name.len - _S_idx) * name.Tsize); \
        memcpy(name.v + _S_idx, src, _S_n * name.Tsize);                                    \
        name.len += _S_n;                                                                   \
    } while (0)

#define seq_extend(name, src, _n)                            \
    do {                                                     \
        typeof(_n) _S_n = _n;                                \
        if (name.len + _S_n > name.max) {                    \
            while (name.len + _S_n > name.max)               \
                name.max <<= 1;                              \
            name.v = realloc(name.v, name.Tsize * name.max); \
        }                                                    \
        memcpy(name.v + name.len, src, _S_n * name.Tsize);   \
        name.len += _S_n;                                    \
    } while (0)

#define seq_remove(name, _i)                                            \
    do {                                                                \
        typeof(_i) _S_i = _i;                                           \
        memmove(name.v + _S_i, name.v + _S_i + 1, name.len - 1 - _S_i); \
        name.len--;                                                     \
    } while (0)

#define seq_pop(name) (name.v[--name.len])

#define seq_free(name) (free(name.v))

#define seq_end(name) (name.v[name.len - 1])

#define seq_back(name, n) (name.v[name.len - (n)])

#define swap(a, b)         \
    do {                   \
        typeof(a) tmp = a; \
        a = b;             \
        b = tmp;           \
    } while (0)

// 懒，直接用unicode吧，UTF-8太费劲
typedef char32_t char_t;
typedef char byte_t;

// SoaringVi的字符串不使用0结尾，而是指定长度
// 显然，我将在C语言中使用神似Python的代码风格
typedef struct seq(char_t) rawstr;
typedef struct seq(rawstr) str_list;

typedef struct seq(char) rawmbs;

#define U_OBUF_SIZE 65536

extern mbstate_t u_mbstate;

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
    char_t ch;
} colortext;

bool cotext_eq(colortext a, colortext b);
bool color_eq(colortext a, colortext b);

void cotext_print(colortext c);

typedef struct coord {
    size_t y, x;
} coord;

#define coord_new(y, x) ((coord){y, x})

int coord_cmp(coord a, coord b);

#define min(a, b)          \
    ({                     \
        typeof(a) _a = a;  \
        typeof(b) _b = b;  \
        _a > _b ? _b : _a; \
    })

#define max(a, b)          \
    ({                     \
        typeof(a) _a = a;  \
        typeof(b) _b = b;  \
        _a < _b ? _b : _a; \
    })

void wstrcpy(char_t *dst, char_t *src);

void u_init_ch2keymap();

extern char_t u_cur_keyenum;

// 我显然没有统计全所有的键码
char_t u_add_keyread(unsigned char *keycode);

char_t u_basic_getch();

bool u_kbhit();
// 是正常输入用返回值返回，否则用指针返回
char_t u_getch();

#define wait_until(ev) while (!(ev))

#define get_time(expr)                                              \
    ({                                                              \
        size_t start = get_usec();                                  \
        typeof(expr) res = expr;                                    \
        size_t end = get_usec();                                    \
        log("time: %zu, ret: %lld\n", end - start, (long long)res); \
        res;                                                        \
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

    K_M_C_A = -200,

    K_UNKNOWN = -201,
};

#define K_CTRL(ch) (ch - 'a' + 1)
#define K_M_CTRL(ch) (ch - 'a' + K_M_C_A)

#define convert(T, V) \
    ((union {         \
         typeof(V) v; \
         T t;         \
     }){.v = V}       \
         .t)

#define isprintable(ch) (isprint(ch) || ch == '\t' || ch == '\n' || ch == ' ')

coord get_term_size();

// 搞一些好用的宏吧
// 2026-6-12
// 好用在哪

#define accumulate(restp, tp, i, start, end, val) \
    ({                                            \
        restp _U_res = 0;                         \
        tp _U_start = start, _U_end = end;        \
        for (tp i = _U_start; i < _U_end; i++)    \
            _U_res += val;                        \
        _U_res;                                   \
    })

#define e_sizesum(start, end, list) accumulate(int, int, i, start, end, list)

#define foreach(seq) for (size_t i = 0; i < seq.len; i++)

#define map_res(dest, expr, end)            \
    do {                                    \
        size_t _U_end = end;                \
        for (size_t i = 0; i < _U_end; i++) \
            dest[i] = expr;                 \
    } while (0)

rawstr str_init_by_charp(char *b);

char *get_abspath(char *f);

size_t get_file_updtime(char *f);

#ifndef _WIN32
#define get_msize malloc_usable_size
#else
#define get_msize _msize
#endif

// 不知道有什么用，先留着吧
byte_t c32_forward(char_t ch);
byte_t u8_forward(unsigned char ch);
byte_t u8_to_c32(unsigned char *mb, char_t *ch);
byte_t u8_from_c32(unsigned char *mb, char_t ch);

rawstr rawstr_from_mbs(char *mbs, size_t len);
rawmbs rawmbs_from_c32(char_t *c32, size_t len);

void encoding_init();
void encoding_fina();

#ifdef _WIN32 // Windows我cnm

#define u16_ispairh(c) (0xD800 <= (c) && (c) <= 0xDBFF)
#define u16_ispairl(c) (0xDC00 <= (c) && (c) <= 0xDFFF)
#define u16_to_c32_macro(h, l) (0x10000 + (((h) - 0xD800) << 10) + ((l) - 0xDC00))
byte_t u16_from_c32(char_t c, wchar_t *u16);
byte_t u16_to_c32(char_t *c, wchar_t *u16);
rawstr rawstr_from_u16(wchar_t *s, size_t len);

#endif

void putchar_c32(char_t ch);

#ifdef _WIN32
#define flockfile(f) _lock_file(f)
#define funlockfile(f) _unlock_file(f)
#define getc_unlocked(f) _getc_nolock(f)
#define putc_unlocked(c, f) _putc_nolock(c, f)
#endif

#define do_if_exist(f, x) \
    if (x)                \
    f(x)

#endif // UTILS_H
