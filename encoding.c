#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#endif

byte_t c32_forward(char_t ch) {
    if (ch > 0x10FFFF || 0xD800 <= ch && ch <= 0xDFFF)
        return 0;
    else if (ch < 0x80)
        return 1;
    else if (ch < 0x800)
        return 2;
    else if (ch < 0x10000)
        return 3;
    else
        return 4;
}

byte_t u8_forward(unsigned char ch) {
    if ((ch & 0x80) == 0)
        return 1;
    else if ((ch & 0xE0) == 0xC0)
        return 2;
    else if ((ch & 0xF0) == 0xE0)
        return 3;
    else if ((ch & 0xF8) == 0xF0)
        return 4;
    else
        return 0;
}

byte_t u8_to_c32(unsigned char *mb, char_t *ch) {
    unsigned char head = mb[0];
    size_t fd = 0;
    char_t res = 0;

    if ((head & 0x80) == 0) {
        fd = 1;
        res = head;
    } else if ((head & 0xE0) == 0xC0) {
        fd = 2;
        res = head & 0x1F;
    } else if ((head & 0xF0) == 0xE0) {
        fd = 3;
        res = head & 0x0F;
    } else if ((head & 0xF8) == 0xF0) {
        fd = 4;
        res = head & 0x07;
    } else {
        return 0;
    }

    for (size_t i = 1; i < fd; ++i) {
        unsigned char b = mb[i];
        if ((b & 0xC0) != 0x80) {
            return 0;
        }
        res = (res << 6) | (b & 0x3F);
    }

    *ch = res;
    return fd;
}

byte_t u8_from_c32(unsigned char *mb, char_t ch) {
    if (ch > 0x10FFFF || 0xD800 <= ch && ch <= 0xDFFF)
        return 0;

    if (ch < 0x80) {
        mb[0] = ch;
        return 1;
    } else if (ch < 0x800) {
        mb[0] = 0xC0 | (ch >> 6);
        mb[1] = 0x80 | (ch & 0x3F);
        return 2;
    } else if (ch < 0x10000) {
        mb[0] = 0xE0 | (ch >> 12);
        mb[1] = 0x80 | ((ch >> 6) & 0x3F);
        mb[2] = 0x80 | (ch & 0x3F);
        return 3;
    } else {
        mb[0] = 0xF0 | (ch >> 18);
        mb[1] = 0x80 | ((ch >> 12) & 0x3F);
        mb[2] = 0x80 | ((ch >> 6) & 0x3F);
        mb[3] = 0x80 | (ch & 0x3F);
        return 4;
    }
}

wchar_t *wcache = 0;
char *cache = 0;
size_t cache_max = 8, wcache_max = 8;

// 2026-7-25
// Linux实现未经测试
// 这种做法稍微有点浪费空间，不过好处是省脑子
rawstr rawstr_from_mbs(char *mbs, size_t len) {
    if (!len)
        return seq_init(rawstr);
    mbsinit(&u_mbstate);
    if (len > wcache_max) {
        while (wcache_max < len)
            wcache_max <<= 1;
        wcache = realloc(wcache, wcache_max);
    }
#ifndef _WIN32
    const char *mbs_const = mbs;
    size_t str_len = mbsrtowcs(wcache, &mbs_const, len, &u_mbstate);
    if (str_len == (size_t)-1)
        return seq_init(rawstr);
    rawstr s = seq_init_reserved(rawstr, str_len);
    for (size_t i = 0; i < str_len; i++)
        seq_append(s, wcache[i]);
#else
    size_t str_len = MultiByteToWideChar(CP_UTF8, 0, mbs, len, wcache, 0);
    if (str_len == 0)
        return seq_init(rawstr);
    rawstr s = seq_init_reserved(rawstr, str_len);
    for (size_t i = 0; i < str_len; i++) {
        if (u16_ispairh(wcache[i])) {
            if (u16_ispairl(wcache[i + 1])) {
                seq_append(s, u16_to_c32_macro(wcache[i], wcache[i + 1]));
                i++;
            } else {
                seq_append(s, wcache[i]);
            }
        }
        seq_append(s, wcache[i]);
    }
#endif
    return s;
}

rawmbs rawmbs_from_c32(char_t *c32, size_t len) {
    if (!len)
        return seq_init(rawmbs);
    mbsinit(&u_mbstate);
    if (len * 4 > cache_max) {
        while (cache_max < len * 4)
            cache_max <<= 1;
        cache = realloc(cache, cache_max);
    }
#ifndef _WIN32
    const char_t *c32_const = c32;
    size_t str_len = wcsrtombs((char *)cache, (const wchar_t **)&c32_const, len, &u_mbstate);
#else
    if (len * 2 > wcache_max) {
        while (wcache_max < len * 2)
            wcache_max <<= 1;
        wcache = realloc(wcache, wcache_max);
    }
    size_t wcache_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (c32[i] >= 0x10000 && c32[i] <= 0x10FFFF) {
            char_t offset = c32[i] - 0x10000;
            wcache[wcache_len++] = 0xD800 + (offset >> 10);
            wcache[wcache_len++] = 0xDC00 + (offset & 0x3FF);
        } else {
            wcache[wcache_len++] = c32[i];
        }
    }
    size_t str_len = WideCharToMultiByte(CP_ACP, 0, wcache, wcache_len, cache, len, NULL, NULL);
#endif
    rawmbs s = seq_init_reserved(rawmbs, str_len + 1);
    memcpy(s.v, cache, str_len * sizeof(char));
    s.len = str_len;
    s.v[s.len] = 0;
    return s;
}

void encoding_init() {
    cache = malloc(cache_max * sizeof(char));
    wcache = malloc(wcache_max * sizeof(wchar_t));
}

void encoding_fina() {
    free(cache);
    free(wcache);
}

#ifdef _WIN32

byte_t u16_from_c32(char_t c, wchar_t *u16) {
    if (c >= 0x10000 && c <= 0x10FFFF) {
        char_t offset = c - 0x10000;
        u16[0] = 0xD800 + (offset >> 10);
        u16[1] = 0xDC00 + (offset & 0x3FF);
        return 2;
    } else {
        u16[0] = c;
        return 1;
    }
}

byte_t u16_to_c32(char_t *c, wchar_t *u16) {
    if (u16_ispairh(u16[0])) {
        c[0] = u16_to_c32_macro(u16[0], u16[1]);
        return 2;
    } else {
        c[0] = u16[0];
        return 1;
    }
}

rawstr rawstr_from_u16(wchar_t *s, size_t len) {
    rawstr res = seq_init_reserved(rawstr, len);
    for (size_t i = 0; i < len; res.len++)
        i += u16_to_c32(res.v + res.len, s + i);
    return res;
}

#endif
