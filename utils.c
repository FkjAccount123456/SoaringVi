#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <sys/time.h>
#include <wchar.h>
#include <locale.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

char u_obuf[U_OBUF_SIZE];
char u_ibuf = 0;

const char *u_style_map[8] = {
    "", "1;", "3;", "1;3;", "4;", "1;4;", "3;4;", "1;3;4;",
};

struct termios u_orig_termios, u_raw_termios;

trie u_ch2keymap;

void u_init_term() {
    tcgetattr(STDIN_FILENO, &u_orig_termios);

    u_raw_termios = u_orig_termios;
    u_raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    u_raw_termios.c_oflag &= ~(OPOST);
    u_raw_termios.c_cflag |= (CS8);
    u_raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    u_raw_termios.c_cc[VMIN] = 1;
    u_raw_termios.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_raw_termios);
}

void u_fina_term() {
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_orig_termios);
}

void u_init() {
    u_init_term();

#ifdef DEBUG_MODE
    u_init_log();
#endif

    // setvbuf(stdout, u_obuf, _IOFBF, U_OBUF_SIZE);
    // flush();

    u_init_ch2keymap();

    setlocale(LC_ALL, "");
}

void u_fina() {
    trie_free(&u_ch2keymap);

    setvbuf(stdout, u_obuf, _IOFBF, U_OBUF_SIZE);

    u_fina_term();

#ifdef DEBUG_MODE
    u_fina_log();
#endif
}

#ifdef DEBUG_MODE

FILE *u_log = NULL;

void u_init_log() {
    u_log = fopen("log.txt", "w");
    if (!u_log)
        error("u_init_log: fopen");
    setvbuf(u_log, NULL, _IONBF, 0);
}

void u_fina_log() {
    fclose(u_log);
    u_log = NULL;
}

#endif // DEBUG_MODE

void gotoxy(size_t y, size_t x) {
    printf("\x1b[%zu;%zuf", y + 1, x + 1);
}

bool cotext_eq(colortext a, colortext b) {
    return a.bg[0] == b.bg[0] && a.bg[1] == b.bg[1] && a.bg[2] == b.bg[2] &&
           a.fg[0] == b.fg[0] && a.fg[1] == b.fg[1] && a.fg[2] == b.fg[2] &&
           a.style == b.style && a.ch == b.ch;
}

bool color_eq(colortext a, colortext b) {
    return a.bg[0] == b.bg[0] && a.bg[1] == b.bg[1] && a.bg[2] == b.bg[2] &&
           a.fg[0] == b.fg[0] && a.fg[1] == b.fg[1] && a.fg[2] == b.fg[2] &&
           a.style == b.style;
}

void cotext_print(colortext c) {
    printf("\x1b[");
    printf("%s", u_style_map[c.style]);
    printf("38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm", c.fg[0], c.fg[1], c.fg[2],
           c.bg[0], c.bg[1], c.bg[2]);
    printf("%lc", c.ch);
}

char u_ctrl_ch2keymap[26][10];
char u_metactrl_ch2keymap[26][10];

void u_init_ch2keymap() {
    trie_init(&u_ch2keymap);
#define A(K, V) trie_insert(&u_ch2keymap, (unsigned char *)K, V)
    A("\x1b", "<esc>");
    A("\x1b[A", "<up>");
    A("\x1b[B", "<down>");
    A("\x1b[D", "<left>");
    A("\x1b[C", "<right>");
    A("\x1b[H", "<home>");
    A("\x1b[F", "<end>");
    A("\x1bOP", "<F1>");
    A("\x1bOQ", "<F2>");
    A("\x1bOR", "<F3>");
    A("\x1bOS", "<F4>");
    A("\x1b[1;5A", "<C-up>");
    A("\x1b[1;5B", "<C-down>");
    A("\x1b[1;5D", "<C-left>");
    A("\x1b[1;5C", "<C-right>");
    A("\x1b[1;5H", "<C-home>");
    A("\x1b[1;5F", "<C-end>");
    A("\x1b[1;5P", "<C-F1>");
    A("\x1b[1;5Q", "<C-F2>");
    A("\x1b[1;5R", "<C-F3>");
    A("\x1b[1;5S", "<C-F4>");
    A("\x1b[1;3A", "<M-up>");
    A("\x1b[1;3B", "<M-down>");
    A("\x1b[1;3D", "<M-left>");
    A("\x1b[1;3C", "<M-right>");
    A("\x1b[1;3H", "<M-home>");
    A("\x1b[1;3F", "<M-end>");
    A("\x1b[1;3P", "<M-F1>");
    A("\x1b[1;3Q", "<M-F2>");
    A("\x1b[1;3R", "<M-F3>");
    A("\x1b[1;3S", "<M-F4>");
    A("\x1b[15~", "<F5>");
    A("\x1b[17~", "<F6>");
    A("\x1b[18~", "<F7>");
    A("\x1b[19~", "<F8>");
    A("\x1b[15;5~", "<C-F5>");
    A("\x1b[17;5~", "<C-F6>");
    A("\x1b[18;5~", "<C-F7>");
    A("\x1b[19;5~", "<C-F8>");
    A("\x1b[15;3~", "<M-F5>");
    A("\x1b[17;3~", "<M-F6>");
    A("\x1b[18;3~", "<M-F7>");
    A("\x1b[19;3~", "<M-F8>");
    A("\x1b[2~", "<ins>");
    A("\x1b[2;5~", "<C-ins>");
    A("\x1b[2;3~", "<M-ins>");
    A("\x1b[3;5~", "<C-del>");
    A("\x1b[3;3~", "<M-del>");
    A("\x1b[5;5~", "<C-pageup>");
    A("\x1b[5;3~", "<M-pageup>");
    A("\x1b[6;5~", "<C-pagedown>");
    A("\x1b[6;3~", "<M-pagedown>");
    A("\x1b[20~", "<F9>");
    A("\x1b[21~", "<F10>");
    A("\x1b[23~", "<F11>");
    A("\x1b[24~", "<F12>");
    A("\x1b[20;5~", "<C-F9>");
    A("\x1b[21;5~", "<C-F10>");
    A("\x1b[23;5~", "<C-F11>");
    A("\x1b[24;5~", "<C-F12>");
    A("\x1b[20;3~", "<M-F9>");
    A("\x1b[21;3~", "<M-F10>");
    A("\x1b[23;3~", "<M-F11>");
    A("\x1b[24;3~", "<M-F12>");
#undef A
    char tmp[10] = "<C- >";
    for (char i = 0; i < 26; i++) {
        tmp[3] = 'a' + i;
        memcpy(u_ctrl_ch2keymap[i], tmp, 10);
    }
    memcpy(tmp, "<M-C- >", 8);
    for (char i = 0; i < 26; i++) {
        tmp[5] = 'a' + i;
        memcpy(u_metactrl_ch2keymap[i], tmp, 10);
    }
}

long long get_usec() {
    struct timeval t = {0};
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 * 1000 + t.tv_usec;
}

unsigned char u_basic_getch() {
    unsigned char res = u_ibuf;
    if (res)
        u_ibuf = 0;
    else
        read(STDIN_FILENO, &res, 1);
    return res;
}

bool u_kbhit() {
    if (u_ibuf)
        return true;
    u_raw_termios.c_cc[VMIN] = 0;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_raw_termios);
    if (read(STDIN_FILENO, &u_ibuf, 1) == 0)
        u_ibuf = 0;
    u_raw_termios.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &u_raw_termios);
    return u_ibuf;
}

unsigned char u_inputs[10];

byte u_getch(char **res) {
    char i = 0;
    unsigned char ch = u_basic_getch();
    switch (ch) {
    case '\t':
        *res = "<tab>";
        return 0;
    case '\n':
    case '\r':
        *res = "<cr>";
        return 0;
    case ' ':
        *res = "<space>";
        return 0;
    case '\x1d':
        *res = "<C-]>";
        return 0;
    case '\x1c':
        *res = "<C-\\>";
        return 0;
    case '\x7f':
        *res = "<bs>";
        return 0;
    }
    if (isprint(ch)) {
        return ch;
    } else if (1 <= ch && ch <= 26) {
        *res = u_ctrl_ch2keymap[ch - 1];
        return 0;
    } else if (ch == 27) {
        if (!u_kbhit()) {
            *res = "<esc>";
            return 0;
        }
        ch = u_basic_getch();
        switch (ch) {
        case '\t':
            *res = "<M-tab>";
            return 0;
        case '\n':
        case '\r':
            *res = "<M-cr>";
            return 0;
        case ' ':
            *res = "<M-space>";
        case '\x1d':
            *res = "<M-C-]>";
            return 0;
        case '\x1c':
            *res = "<M-C-\\>";
            return 0;
        case '\x7f':
            *res = "<M-bs>";
            return 0;
        case '\x1b':
            *res = "<M-esc>";
            return 0;
        }
        if (0 <= ch && ch <= 26) {
            *res = u_metactrl_ch2keymap[ch - 1];
            return 0;
        }
        trie *t = u_ch2keymap.child['\x1b'];
        if (!t->child[ch]) {
            *res = "<unknown>";
            return 0;
        }
        t = t->child[ch];
        while (t) {
            ch = u_basic_getch();
            if (!t->child[ch]) {
                break;
            }
            t = t->child[ch];
            if (t->is_leaf) {
                *res = t->data;
                return 0;
            }
        }
        *res = "<unknown>";
        return 0;
    } else {
        char nxt = utf8_next(ch);
        if (nxt == -1) {
            *res = "<unknown>";
            return 0;
        }
        u_inputs[i++] = ch;
        for (; i < nxt; i++)
            u_inputs[i] = u_basic_getch();
        byte res;
        utf8_cvt(u_inputs, &res);
        return res;
    }
}

char utf8_next(unsigned char ch) {
    if (ch < 0x80)
        return 1;
    if ((ch & 0xE0) == 0xC0)
        return 2;
    if ((ch & 0xF0) == 0xE0)
        return 3;
    if ((ch & 0xF8) == 0xF0)
        return 4;
    return -1;
}

char utf8_cvt(const unsigned char *code, byte *output) {
    char nbytes;
    wchar_t res = *code;
    if (res < 0x80)
        nbytes = 1, *output = res;
    else if ((res & 0xE0) == 0xC0)
        nbytes = 2, res &= 0x1F;
    else if ((res & 0xF0) == 0xE0)
        nbytes = 3, res &= 0x0F;
    else if ((res & 0xF8) == 0xF0)
        nbytes = 4, res &= 0x07;
    else
        return -1;

    for (char i = 1; i < nbytes; i++) {
        // if ((code[i] & 0xC0) != 0x80)
        //     return -1;
        res = (res << 6) | (code[i] & 0x3F);
    }
    *output = res;

    return nbytes;
}

void trie_insert(trie *t, unsigned char *key, void *data) {
    unsigned char *p = key;
    while (*p) {
        trie *child = t->child[*p];
        if (!child) {
            child = malloc(sizeof(trie));
            trie_init(child);
            t->child[*p] = child;
        }
        t = child;
        p++;
    }
    t->is_leaf = true;
    t->data = data;
}

void **trie_get(trie *t, unsigned char *key) {
    unsigned char *p = key;
    while (*p) {
        trie *child = t->child[*p];
        if (!child)
            return NULL;
        t = child;
        p++;
    }
    if (t->is_leaf)
        return &t->data;
    return NULL;
}

void trie_free(trie *t) {
    for (short i = 0; i < 256; i++) {
        if (t->child[i]) {
            trie_free(t->child[i]);
            free(t->child[i]);
        }
    }
}
