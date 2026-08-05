#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include "utils.h"

// 2026-8-5
// 也算设计缺陷了吧，如果换成short/int renderer_insert就会炸，不过也好改
typedef enum colortype_t : unsigned char {
    CTE_BACKGROUND,
    CTE_FOREGROUND,
    CTE_MODELINE,
    CTE_CMDLINE,
    CTE_BORDER,
    CTE_WARNING,
    CTE_ERROR,
    CTE_CURSOR,
    CTE_LINENUM,
    CTE_CURLINENUM,
    CTE_CURLINE,
    CTE_SELECTED,
    CTE_COMPLETION,
    CTE_COMPLSEL,

    CT_TEXT,
    CT_ERRTEXT,
    CT_OTHER,
    CT_IDENTIFIER,
    CT_OPERATOR,
    CT_DELIMITER,
    CT_KEYWORD,
    CT_STRING,
    CT_ESCAPE,
    CT_COMMENT,
    CT_NUMBER,
    CT_CONST,
    CT_PREPROCESSOR,

    CT_FUNCTION,
    CT_MACRO,
    CT_TYPE,
    CT_MODULE,
    CT_PARAMETER,
    CT_THISPARAM,
    CT_METHOD,
    CT_ATTRIBUTE,

    CT_END,
} colortype_t;

// colortext.ch用于标记指定的字段
// a指bg fg全部指定，0指默认（组合默认的bg和fg），style会将默认和当前求并集
typedef colortext colorscheme[CT_END];

extern colorscheme color_quiet;

void color_init();

colortext color_get(colorscheme *c, char_t ch, colortype_t t, bool sel);

#endif // COLORSCHEME_H
