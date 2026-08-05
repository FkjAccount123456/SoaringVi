#include "colorscheme.h"

colorscheme color_quiet;

void color_init() {
    memset(color_quiet, 0, sizeof(color_quiet));

    color_quiet[CTE_BACKGROUND] = cotext_new(0, 0x101010, 0, 'b');
    color_quiet[CTE_FOREGROUND] = cotext_new(0xC0C0C0, 0, 0, 'f');
    color_quiet[CTE_MODELINE] = cotext_new(0x101010, 0xC0C0C0, 0, 'a');
    color_quiet[CTE_WARNING] = cotext_new(0xA0A0A0, 0, 0, 'f');
    color_quiet[CTE_ERROR] = color_quiet[CTE_WARNING];
    color_quiet[CTE_CURSOR] = color_quiet[CTE_FOREGROUND];
    color_quiet[CTE_SELECTED] = cotext_new(0, 0x606060, 0, 'b');
    color_quiet[CTE_COMPLSEL] = color_quiet[CTE_SELECTED];

    color_quiet[CT_ERRTEXT] = color_quiet[CTE_WARNING];
    color_quiet[CT_DELIMITER] = color_quiet[CTE_WARNING];
    color_quiet[CT_KEYWORD] = cotext_new(0, 0, STYLE_BOLD, 0);
    color_quiet[CT_STRING] = color_quiet[CTE_WARNING];
    color_quiet[CT_COMMENT] = cotext_new(0xA0A0A0, 0, STYLE_ITALIC, 'f');
    color_quiet[CT_NUMBER] = color_quiet[CT_KEYWORD];
    color_quiet[CT_CONST] = color_quiet[CT_KEYWORD];
    color_quiet[CT_PREPROCESSOR] = color_quiet[CTE_WARNING];
}

colortext color_get(colorscheme *c, char_t ch, colortype_t t, bool sel) {
    colortext res = (*c)[t];
    if (res.ch == 'b')
        memcpy(res.fg, (*c)[CTE_FOREGROUND].fg, 3);
    else if (res.ch == 'f')
        memcpy(res.bg, (*c)[CTE_BACKGROUND].bg, 3);
    else if (res.ch == 0)
        memcpy(res.fg, (*c)[CTE_FOREGROUND].fg, 3), memcpy(res.bg, (*c)[CTE_BACKGROUND].bg, 3);
    if (sel)
        memcpy(res.bg, (*c)[CTE_SELECTED].bg, 3);
    res.style |= (*c)[CTE_FOREGROUND].style;
    res.ch = ch;
    return res;
}
