#include "utils.h"

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

rawstr rawstr_from_u8(unsigned char *u8, size_t len) {
    rawstr s = seq_init_reserved(rawstr, (len + 2) / 2);
    for (size_t i = 0; i < len;) {
        char_t ch;
        byte_t fd = u8_to_c32(u8 + i, &ch);
        if (fd == 0)
            i++;
        else
            seq_append(s, ch);
        i += fd;
    }
    return s;
}

rawmbs rawmbs_from_c32(char_t *c32, size_t len) {
    rawmbs s = seq_init_reserved(rawmbs, len);
    for (size_t i = 0; i < len; i++) {
        seq_expand(s, 4);
        byte_t fd = u8_from_c32(s.v + s.len, c32[i]);
        s.len += fd;
    }
    return s;
}
